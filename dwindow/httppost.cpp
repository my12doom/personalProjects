#include "httppost.h"
#include <assert.h>
#include <Windows.h>
#include <wininet.h>
#include <atlbase.h>
#pragma  comment(lib, "ws2_32.lib")

#define safe_delete(x) if(x){delete x;x=NULL;}
#define TIMEOUT 30000
#define MAXRESPONSESIZE 10240
#define MAXREQUESTSIZE 1024000
#define MAXRESPONSE_DATASIZE 2048000
static int wcstrim(wchar_t *str, wchar_t char_  = L' ');

#define log printf
#define LINE(...) {sprintf(tmp, __VA_ARGS__); strcat(requestheader, tmp); strcat(requestheader, "\r\n");}
#define SEND_FMT(...) {sprintf(tmp, __VA_ARGS__); send(m_sock, tmp, strlen(tmp), 0); log(tmp); m_request_content_sent += strlen(tmp);}

class W2UTF8
{
public:
	W2UTF8(const wchar_t *in);
	~W2UTF8();
	operator char*();
	char *p;
};

class UTF82W
{
public:
	UTF82W(const char *in);
	~UTF82W();
	operator wchar_t*();
	wchar_t *p;
};

httppost::httppost(const wchar_t *url)
:m_object(NULL)
,m_server(NULL)
,m_sock(0)
,m_content_length(0)
,m_content_read(0)
,m_request_content_size(0)
,m_request_content_sent(0)
,m_cb(NULL)
{
	setURL(url);
};

httppost::~httppost()
{
	for(std::list<form_item>::iterator i = m_form_items.begin(); i != m_form_items.end(); ++i)
		delete_form_item(*i);
	for(std::map<wchar_t*, wchar_t*>::iterator i = m_headers.begin(); i!= m_headers.end(); ++i)
		delete i->first;
	for(std::map<wchar_t*, wchar_t*>::iterator i = m_request_headers.begin(); i!= m_request_headers.end(); ++i)
		delete i->first;

	safe_delete(m_server);
	safe_delete(m_object);
	close_connection();
	if (m_sock)
		closesocket(m_sock);
}

int httppost::close_connection()
{
	if (m_sock)
		closesocket(m_sock);
	m_sock = 0;
	return 0;
}

int httppost::setURL(const wchar_t *url)
{
	safe_delete(m_server);
	safe_delete(m_object);

	if (wcslen(url) < 7 || wcschr(url+7, L'/') == NULL)
		return -1;

	m_server = new wchar_t[wcslen(url)+1];
	m_object = new wchar_t[wcslen(url)+1];

	wcscpy(m_object, wcschr(url+7, L'/'));
	wcscpy(m_server, url+7);
	*((wchar_t*)wcschr(m_server, L'/')) = NULL;

	m_port = 80;
	wchar_t *p = (wchar_t*)wcschr(m_server, L':');
	if (p)
	{
		p[0] = NULL;
		m_port = _wtoi(p+1);
	}
	return 0;
}

int httppost::addFile(const wchar_t *name, const wchar_t *local_filename)
{
	FILE *f = _wfopen(local_filename, L"rb");
	if (!f)
		return -1;

	fclose(f);

	form_item item;
	item.type = form_item::form_item_file_name;
	item.name = new wchar_t[wcslen(name)+1];
	wcscpy(item.name, name);
	item.file_name = new wchar_t[wcslen(local_filename)+1];
	wcscpy(item.file_name, local_filename);

	m_form_items.push_back(item);

	return 0;
}
int httppost::addFile(const wchar_t *name, const void *data, int size)
{
	form_item item;
	item.type = form_item::form_item_file_data;
	item.name = new wchar_t[wcslen(name)+1];
	wcscpy(item.name, name);
	item.file_data = new char[size];
	item.data_size = size;
	memcpy(item.file_data, data, size);

	m_form_items.push_back(item);

	return 0;
}
int httppost::addFormItem(const wchar_t *name, const wchar_t *value)
{
	form_item item;
	item.type = form_item::form_item_data;
	item.name = new wchar_t[wcslen(name)+1];
	wcscpy(item.name, name);
	item.form_data = new wchar_t[wcslen(value)+1];
	wcscpy(item.form_data, value);

	m_form_items.push_back(item);

	return 0;
}
int httppost::addHeader(const wchar_t *name, const wchar_t *value)
{
	wchar_t *name2 = new wchar_t[wcslen(name)+wcslen(value)+2];
	wchar_t *value2 = name2+wcslen(name)+1;
	wcscpy(name2, name);
	wcscpy(value2, value);
	m_request_headers[name2] = value2;

	return 0;
}
void httppost::delete_form_item(form_item &item)
{
	switch (item.type)
	{
	case form_item::form_item_data:
		delete [] item.form_data;
		break;
	case form_item::form_item_file_data:
		delete [] item.file_data;
		break;
	case form_item::form_item_file_name:
		delete [] item.file_name;
		break;
	default:
		assert(0);
	}

	delete item.name;
}

// calculate the data size it self, no leading or tailing symbols
int httppost::calculate_item_size(form_item &item)
{
	char tmp[MAXREQUESTSIZE/10];
	USES_CONVERSION;
	switch (item.type)
	{
	case form_item::form_item_data:
		sprintf(tmp, "Content-Disposition: form-data; name=\"%s\"\r\n\r\n", W2UTF8(item.name));
		return strlen(W2UTF8(item.form_data))+strlen(tmp);
		break;
	case form_item::form_item_file_data:
		sprintf(tmp, "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
			"Content-Type: application/octet-stream\r\n\r\n", W2UTF8(item.name), "RawData.dat");
		return item.data_size+strlen(tmp);
		break;
	case form_item::form_item_file_name:
		{
			FILE *f = _wfopen(item.file_name, L"rb");
			if (!f)
				return -1;
			fseek(f, 0, SEEK_END);
			int file_size = ftell(f);
			fclose(f);
			sprintf(tmp, "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
				"Content-Type: application/octet-stream\r\n\r\n", W2UTF8(item.name), W2UTF8(item.file_name));
			return file_size+strlen(tmp);
		}
		break;
	default:
		assert(0);
		return -1;
	}

	return -1;
}


// send the content itself, no \r\n symbols
int httppost::send_item(int sock, form_item &item)
{
	char tmp[MAXREQUESTSIZE/10];
	USES_CONVERSION;
	switch (item.type)
	{
	case form_item::form_item_data:
		SEND_FMT("Content-Disposition: form-data; name=\"%s\"\r\n\r\n", W2UTF8(item.name));
		send(sock, W2UTF8(item.form_data), strlen(W2UTF8(item.form_data)), 0);
		log(W2UTF8(item.form_data));
		m_request_content_sent += strlen(W2UTF8(item.form_data));
		callback();
		return 0;
		break;
	case form_item::form_item_file_data:
		{
			SEND_FMT("Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
				"Content-Type: application/octet-stream\r\n\r\n", W2UTF8(item.name), "RawData.dat");
			log("<<<binary data>>>");
			int left = item.data_size;
			char *p = (char*)item.file_data;
			while (left > 0)
			{
				int this_block_size = min(left, 2048);
				send(sock, p, this_block_size, 0);
				m_request_content_sent += this_block_size;
				p += this_block_size;
				left -= this_block_size;
				callback();
			}
		}
		return 0;
	case form_item::form_item_file_name:
		{
			FILE *f = _wfopen(item.file_name, L"rb");
			if (!f)
				return -1;

			SEND_FMT("Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
				"Content-Type: application/octet-stream\r\n\r\n", W2UTF8(item.name), W2UTF8(item.file_name));

			log("<<<binary data>>>");
			char block[2048];
			int read;
			while (read = fread(block, 1, sizeof(block), f))
			{
				send(m_sock, block, read, 0);
				m_request_content_sent += read;
				callback();
			}

			fclose(f);
			return 0;
		}
		break;
	default:
		assert(0);
		return -1;
	}

	return 0;
}

int httppost::send_request()
{	
	USES_CONVERSION;
	WSADATA WSAData;
	if (0 != WSAStartup(MAKEWORD(1, 1), &WSAData))
		return -1;

	struct hostent *host = gethostbyname(W2UTF8(m_server));
	if(NULL == host)
		return -2;

	m_sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (m_sock<0)
		return -3;

	struct sockaddr_in server_addr = {0};   
	server_addr.sin_family=AF_INET; 
	server_addr.sin_port=htons(m_port); 
	server_addr.sin_addr=*((struct in_addr *)host->h_addr);         
	if(connect(m_sock,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
		return -4;

	char *requestheader = new char[MAXREQUESTSIZE];
	requestheader[0] = NULL;
	char *tmp = new char[MAXREQUESTSIZE];

	char boundary[] = "----7b4a6d158c9";

	LINE("%s %s HTTP/1.1", m_form_items.size()>0 ? "POST" : "GET", W2UTF8(m_object));
	LINE("HOST: %s", W2UTF8(m_server));
	LINE("Accept: */*");
	LINE("Content-Type: multipart/form-data; charset=UTF-8; boundary=%s", boundary);
	LINE("Accept-Encoding: gzip, deflate");
	LINE("User-Agent: Mozilla/4.0 (compatible; windows 5.1)");
	LINE("Connection: Keep-Alive");
	LINE("Cache-Control: no-cache");
	for(std::map<wchar_t*, wchar_t*>::iterator i = m_request_headers.begin(); i!= m_request_headers.end(); ++i)
		LINE("%s: %s", W2UTF8(i->first), W2UTF8(i->second));

	m_request_content_size = 0;
	for(std::list<form_item>::iterator i = m_form_items.begin(); i != m_form_items.end(); ++i)
	{
		m_request_content_size += strlen(boundary) + 4;
		m_request_content_size += calculate_item_size(*i);
		m_request_content_size += strlen(boundary) + 8;
	}

	if (m_request_content_size >0)
		LINE("Content-Length: %d", m_request_content_size);
	LINE("");

	send(m_sock, requestheader, strlen(requestheader), 0);
	log(requestheader);
	for(std::list<form_item>::iterator i = m_form_items.begin(); i != m_form_items.end(); ++i)
	{
		SEND_FMT("--%s\r\n", boundary);
		send_item(m_sock, *i);
		SEND_FMT("\r\n");
		SEND_FMT("--%s--\r\n", boundary);
		callback();
	}

	delete [] requestheader;
	delete [] tmp;


	char *response = new char[MAXRESPONSESIZE];
	int got = 0;
	int response_code = 400;
	while(recv(m_sock, &response[got], 1, 0) >0)
	{
		//log("%c", response[got]);
		got++;
		if (got >=2 && response[got-1] == '\n' && response[got-2] == '\r')
		{
			response[got] = NULL;
			if (got == 2)		// end of headers
				break;
			else if (strstr(response, "HTTP") == response)
			{
				if (strstr(response, " "))
					response_code = atoi(strstr(response, " ")+1);
			}
			else
			{
				wchar_t *line = new wchar_t[got+1];
				wcscpy(line, UTF82W(response));

				wchar_t *value = (wchar_t*)wcschr(line, L':');

				if (value)
				{
					value[0] = NULL;
					value ++;
					wcstrim(value, L' ');
					wcstrim(value, L'\r');
					wcstrim(value, L'\n');

					m_headers [line] = value;

					if (wcscmp(line, L"Content-Length") == 0)
						swscanf(value, L"%I64d", &m_content_length);
				}
			}

			got = 0;
		}
	}

	return response_code;
}

std::map<wchar_t*,wchar_t*> httppost::get_response_headers()
{
	return m_headers;
}

int httppost::read_content(void *buf, int size)
{
	if (m_content_length>0)
		size = (int)min(size, m_content_length - m_content_read);

	if (size <= 0)
		return 0;

	int o = recv(m_sock, (char*)buf, size, 0);
	m_content_read += o;

	return o;
}

int httppost::callback()
{
	if (m_cb)
		m_cb->HttpSendingCallback(m_request_content_sent, m_request_content_size);
	return 0;
}

int httppost::set_callback(IHttpSendingCallback *cb)
{
	m_cb = cb;
	return 0;
}

static int wcstrim(wchar_t *str, wchar_t char_ )
{
	int len = (int)wcslen(str);
	//LEADING:
	int lead = 0;
	for(int i=0; i<len; i++)
		if (str[i] != char_)
		{
			lead = i;
			break;
		}

	//ENDING:
	int end = 0;
	for (int i=len-1; i>=0; i--)
		if (str[i] != char_)
		{
			end = len - 1 - i;
			break;
		}
	//TRIMMING:
	memmove(str, str+lead, (len-lead-end)*sizeof(wchar_t));
	str[len-lead-end] = NULL;

	return len - lead - end;
}