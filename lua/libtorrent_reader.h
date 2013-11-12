#pragma once


#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/escape_string.hpp"
#include "libtorrent/peer_info.hpp"
#include <boost/lambda/lambda.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/filesystem.hpp>

using namespace libtorrent;
class TORRENT_EXPORT reader : public boost::noncopyable
{
public:
	reader(torrent_handle& h, session &s);
	~reader();

public:
	int read(char* data, size_type offset, size_type size, bool *cancel = NULL);
protected:
	int read_piece(int index);
	void on_read(char* data, size_type offset, size_type size);

private:
	boost::mutex m_notify_mutex;
	boost::condition m_notify;
	torrent_handle &m_handle;
	session &m_ses;
	boost::filesystem::path m_file_path;
	std::fstream m_file;
	char *m_piece_buffer;
	int m_datasize;
	int m_data_index;
	const torrent_info& m_info;
	int m_last_piece_size;
};