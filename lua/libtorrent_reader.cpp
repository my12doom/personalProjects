#include "libtorrent_reader.h"

using namespace libtorrent;

reader::reader(torrent_handle& h, session &s)
: m_handle(h)
, m_ses(s)
, m_info(h.get_torrent_info())
, m_last_piece_size()
, m_data_index(-1)
{
	m_last_piece_size = m_info.piece_size(m_info.num_pieces()-1);
	m_piece_buffer = new char[m_info.piece_length()];
}

reader::~reader()
{
	delete m_piece_buffer;
}

void reader::on_read(char* data, size_type offset, size_type size)
{
	boost::mutex::scoped_lock lock(m_notify_mutex);

	m_datasize = size;
	memcpy(m_piece_buffer, data, size);
	m_notify.notify_one();
}

int reader::read(char* data, size_type offset, size_type size, bool *cancel/* = NULL*/)
{
	size_type block_size = m_info.piece_length();
	size_type end = offset + size;
	size_type got = 0;
	end = std::min(end, block_size*(m_info.num_pieces()-1)+m_last_piece_size);
	size = end - offset;

	if (size<=0)
		return 0;

	// head
	if (offset % block_size)
	{
		int index = offset / block_size;
		int result;
		while (!(result = read_piece(index)))
		{
			if (cancel && *cancel)
				return -1;

			Sleep(1);
		}
		
		if (result == -1)
			return -1;

		int size_to_copy = std::min(block_size - (offset % block_size), size);
		memcpy(data, m_piece_buffer+(offset % block_size), size_to_copy);
		data += size_to_copy;
		size -= size_to_copy;
		offset += size_to_copy;
		got += size_to_copy;
	}

	// main body
	while (size > block_size)
	{
		int index = offset / block_size;
		int result;
		while (!(result = read_piece(index)))
		{
			if (cancel && *cancel)
				return -1;

			Sleep(1);
		}

		if (result == -1)
			return -1;
		
		memcpy(data, m_piece_buffer, m_datasize);
		data += m_datasize;
		size -= m_datasize;
		offset += m_datasize;
		got += m_datasize;
	}

	// tail
	if (size > 0)
	{
		int index = offset / block_size;
		int result;
		while (!(result = read_piece(index)))
		{
			if (cancel && *cancel)
				return -1;

			Sleep(1);
		}

		if (result == -1)
			return -1;

		size_type bytes_to_copy = std::min(size, (size_type)m_datasize);
		memcpy(data, m_piece_buffer, bytes_to_copy);
		data += bytes_to_copy;
		size -= bytes_to_copy;
		offset += bytes_to_copy;
		got += bytes_to_copy;
	}

	return got;
}

int reader::read_piece(int index)
{
	boost::mutex::scoped_lock lock(m_notify_mutex);
	int ret = 0;

	if (index >= m_info.num_pieces() || index < 0)
		return -1;
	if (index == m_data_index && m_datasize > 0)
		return 1;

	torrent_status status = m_handle.status();
	if (!status.pieces.empty())
	{
		if (status.state != torrent_status::downloading &&
			status.state != torrent_status::finished &&
			status.state != torrent_status::seeding)
			return ret;

		// 设置下载位置.
		std::vector<int> pieces;
		pieces = m_handle.piece_priorities();
		std::for_each(pieces.begin(), pieces.end(), boost::lambda::_1 = 1);
		for(int i=index; i<index+50 && i<m_info.num_pieces(); i++)
			pieces[i] = 7;
		m_handle.prioritize_pieces(pieces);

		if (status.pieces.get_bit(index))
		{
			// 提交请求.
			m_handle.read_piece(index, boost::bind(&reader::on_read, this, _1, _2, _3));

			// 等待请求返回.
			m_notify.wait(lock);

			ret = m_datasize == m_info.piece_size(index);
		}
	}
	else
	{
		// 直接读取文件.
		if (m_file_path.string() == "" || !m_file.is_open())
		{
			boost::filesystem::path path = m_handle.save_path();
			const file_storage& stor = m_info.files();
			std::string name = stor.name();
			m_file_path = path / name;
			name = convert_to_native(m_file_path.string());

			// 打开文件, 直接从文件读取数据.
			if (!m_file.is_open())
				m_file.open(name.c_str(), std::ios::in | std::ios::binary);
		}

		if (!m_file.is_open())
			return ret;

		m_file.clear();
		m_file.seekg(index * m_info.piece_length(), std::ios::beg);
		m_file.read(m_piece_buffer, m_info.piece_size(index));
		m_datasize = m_file.gcount();

		if (m_datasize != -1)
			ret = 1;
	}

	if (ret == 1)
		m_data_index = index;
	return ret;
}