extern "C"
{
#include <stdint.h>
#include <inttypes.h>
#include <libavcodec/avcodec.h>	
#include <pthread.h>
}
#include <queue>
class client
{
public:
	client();
	virtual ~client();

	int connect(const char *host, int port);
	int disconnect();
	virtual void show_picture(AVFrame *frame){}			// default : do nothing
	int start_decoding();
	int stop_decoding();

protected:

	static void *decoding_thread_entry(void *para){((client*)para)->decoding_thread(); return NULL;}
	void decoding_thread();
	static void *network_thread_entry(void *para){((client*)para)->network_thread(); return NULL;}
	void network_thread();

	int get_h264_frame(void *buf);
	int get_a_frame(void *buf);

	int sockfd;
	bool m_stop_decoding;
	pthread_t m_decoding_thread;
	pthread_t m_network_thread;

	pthread_mutex_t m_queue_mutex;

	static const int INBUF_SIZE = 409600;
	typedef struct
	{
		char data[INBUF_SIZE];
		int size;
	} packet;
	std::queue<packet> m_queue;
};