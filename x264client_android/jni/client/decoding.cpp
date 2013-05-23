#include "decoding.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <../3dvlog.h>
#include "runnable.h"
#include <pthread.h>
#include "full_cache.h"
#include "httppost.h"

#define max(a,b) ((a)>(b) ? (a) : (b))

// test runnable
class runner : public Irunnable
{
public:
	runner()
	{
		locked = true;
	}
	virtual void run()
	{
		LOGE("run %08x\n", this);
		while(locked)
			usleep(100);
		LOGE("run %08x OK\n", this);
	}
	virtual void signal_quit()
	{
		LOGE("signal_quit %08x\n", this);
		locked = false;
	}
	virtual void join()
	{
		while(locked)
			usleep(10000);
	}
	~runner()
	{
		LOGE("~runner()\n");
	}
protected:
	bool locked;
};

int test_thread_pool()
{
	LOGE("test_thread_pool");
	thread_pool pool(10);
	pool.submmit(new runner);
	pool.submmit(new runner);

	for(int i=0; i<1000; i++)
		usleep(50000);

	return 0;
	
	httppost post(L"http://192.168.1.199:8080/flv.flv");
	int response_code = post.send_request();

	LOGE("http size: %d code=%d", (int)post.get_content_length(), response_code);
	FILE *f = fopen("/sdcard/flv.flv", "wb");
	int got = 0;
	int total = 0;
	char buf[4096];
	while ((got=post.read_content(buf, 4096))>0)
	{
		fwrite(buf, 1, got, f);
		total += got;
	}

	fclose(f);

	return 0;
}


void test_cache()
{
	disk_manager *d = new disk_manager(L"flv.flv.config");
	d->setURL(L"http://192.168.1.199:8080/flv.flv");

	FILE * f = fopen("/sdcard/flv.flv", "rb");

	srand(123456);

	int l = GetTickCount();
	int max_response = 0;
	int last_tick = l;
	const int block_size = 99;
	for(int i=0; i<500; i++)
	{
		LOGE("test %d", i);
		int pos = __int64(21008892-block_size) * rand() / RAND_MAX;

		char block[block_size] = {0};
		char block2[block_size] = {0};
		char ref_block[block_size] = {0};
		fragment frag = {pos, pos+block_size};
		d->get(block, frag);

		fseek(f, pos, SEEK_SET);
		fread(ref_block, 1, sizeof(ref_block), f);

		int c = memcmp(block, ref_block, sizeof(block)-1);

		if (c != 0)
		{
			int j;
			for(j=0; j<sizeof(block); j++)
				if (block[j] != ref_block[j])
					break;
				

			d->get(block2, frag);
			d->get(block2, frag);
			int c2 = memcmp(block2, block, sizeof(block));
			LOGE("error");

			FILE *e = fopen("/sdcard/t.bin", "wb");
			fwrite(block, 1, block_size, e);
			fwrite(block2, 1, block_size, e);
			fclose(e);
			break;
		}

		max_response = max(max_response, GetTickCount() - last_tick);
		last_tick = GetTickCount();
	}

	LOGE("avg speed: %d KB/s\n", __int64(50000)*block_size / (GetTickCount()-l));
	LOGE("avg response time: %d ms, total %dms, max %dms\n\n", (GetTickCount()-l)/50000, GetTickCount()-l, max_response);

	l = GetTickCount();
	LOGE("exiting cache");
	delete d;
	LOGE("done exiting cache, %dms\n", GetTickCount()-l);
}


#define MAX_CACHE 5

int sockfd;

bool utf8fromwcs(const wchar_t* wcs, size_t length, char* outbuf);
void UTF2Uni(const char *src, wchar_t *des, int count_d);
client::client()
{
    pthread_mutex_init(&m_queue_mutex, NULL);
	LOGE("client::client()");
	wchar_t test[2000] = L"";
	char testo[2000]={0};
	sprintf(testo, "Hold %lld", __int64(123456789));
	UTF2Uni(testo, test, 200);
	utf8fromwcs(test, wcslen(test), testo);

	LOGE("testo:%s, %d", testo, wcslen(test));

	test_thread_pool();
}

client::~client()
{
    pthread_mutex_destroy(&m_queue_mutex);
}

int client::start_decoding()
{
    m_stop_decoding = false;
    pthread_create(&m_decoding_thread, NULL, decoding_thread_entry, this);
    pthread_create(&m_network_thread, NULL, network_thread_entry, this);

    return 0;
}

int client::stop_decoding()
{
    m_stop_decoding = true;

    pthread_join(m_decoding_thread, NULL);
    pthread_join(m_network_thread, NULL);

    while(!m_queue.empty())
        m_queue.pop();

    return 0;
}

int client::connect(const char *host_address, int port)
{
	LOGE("connect(%s, %d)", host_address, port);
	struct hostent *host;
	if((host=gethostbyname(host_address))==NULL)
	{
		LOGE("Gethostname error, %s\n", strerror(errno));
		return -1;
	}       
	sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sockfd==-1)
	{
		LOGE("Socket Error:%s\a\n",strerror(errno));
		return -2;
	}
	struct sockaddr_in server_addr = {0};   
	server_addr.sin_family=AF_INET; 
	server_addr.sin_port=htons(port); 
	server_addr.sin_addr=*((struct in_addr *)host->h_addr);

	if(::connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
	{               
		LOGE("Connect Error:%s\a\n",strerror(errno));
		return -3;
	}

	LOGI("recv");
	char test[4096] = {0};
	recv(sockfd, test, 4096, 0);
	if(strstr(test,"\n") == NULL)
		recv(sockfd, test+strlen(test), 4096, 0);
	LOGI(test);

	const char *p = "auth|TestCode\r\n";
	send(sockfd, p, strlen(p), 0);
	memset(test, 0, 4096);
	int o = recv(sockfd, test, 4096, 0);
	if(strstr(test,"\n") == NULL)
		recv(sockfd, test+strlen(test), 4096, 0);
	LOGI(test);

	p = "x264_init\r\n";
	send(sockfd, p, strlen(p), 0);
	memset(test, 0, 4096);
	o = recv(sockfd, test, 4096, 0);
	if(strstr(test,"\n") == NULL)
		recv(sockfd, test+strlen(test), 4096, 0);
	m_x264_handle = atoi(strstr(test, ",")+1);
	LOGI("%s, x264_handle = %d", test, m_x264_handle);
}

int client::disconnect()
{
	char p[1024];
	char test[4096];
	sprintf(p, "x264_destroy|%d\r\n", m_x264_handle);
	send(sockfd, p, strlen(p), 0);
	memset(test, 0, 4096);
	int o = recv(sockfd, test, 4096, 0);
	if(strstr(test,"\n") == NULL)
		recv(sockfd, test+strlen(test), 4096, 0);

	LOGI(test);

    close(sockfd);
}

int client::get_h264_frame(void *buf)
{
	char p[1024];
	sprintf(p, "x264_shot|%d\r\n", m_x264_handle);
	send(sockfd, p, strlen(p), 0);

	int frame_size = 0;
	if (recv(sockfd, (char*)&frame_size, 4, NULL) != 4)
		return -1;

	if (frame_size> 1024*1024)
		return -1;

	char *dst = (char*)buf;
	int recieved = 0;
	while (recieved < frame_size)
	{
		int got = recv(sockfd, dst, frame_size - recieved, NULL);
		if (got < 0)
			return 0;

		dst += got;
		recieved += got;
	}

	LOGI("frame_size:%d\n", frame_size);

	return recieved;
}

int client::get_a_frame(void *buf)
{
retry:
    pthread_mutex_lock(&m_queue_mutex);

    if (m_queue.empty())
    {
        pthread_mutex_unlock(&m_queue_mutex);

        if (m_stop_decoding)
            return -1;

        usleep(1000);
        goto retry;
    }

    packet p = m_queue.front();
    m_queue.pop();

    pthread_mutex_unlock(&m_queue_mutex);

    memcpy(buf, p.data, p.size);

    return p.size;
}

void client::network_thread()
{
	return;
    for(;!m_stop_decoding;)
    {
        packet p;
        p.size = get_h264_frame(p.data);

        if (p.size<0)
            return;

        retry:

        pthread_mutex_lock(&m_queue_mutex);
        if (m_queue.size() > MAX_CACHE)
        {
            pthread_mutex_unlock(&m_queue_mutex);

            if (m_stop_decoding)
                return;

            usleep(1000);
            goto retry;
        }

        m_queue.push(p);

        pthread_mutex_unlock(&m_queue_mutex);
    }
}

void client::decoding_thread()
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int frame, got_picture, len;
    AVFrame *picture;
    uint8_t inbuf[INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;

	avcodec_register_all();
    av_init_packet(&avpkt);

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
    memset(inbuf + INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    LOGE("Video decoding\n");

    /* find the mpeg1 video decoder */
    codec = avcodec_find_decoder(CODEC_ID_H264);
    if (!codec) {
        LOGE("codec not found\n");
        return;
    }

    c = avcodec_alloc_context3(codec);
    picture= avcodec_alloc_frame();

    if(codec->capabilities&CODEC_CAP_TRUNCATED)
        c->flags|= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        LOGE("could not open codec\n");
        return;
    }

    /* the codec gives us the frame size, in samples */


    frame = 0;
	int frame_read = 0;
    for(;!m_stop_decoding;) {
		LOGI("loop");
        avpkt.size = get_h264_frame(inbuf);
        if (avpkt.size < 0)
            break;
		frame_read ++;
		if (avpkt.size == 0)
			continue;

        /* NOTE1: some codecs are stream based (mpegvideo, mpegaudio)
           and this is the only method to use them because you cannot
           know the compressed data size before analysing it.

           BUT some other codecs (msmpeg4, mpeg4) are inherently frame
           based, so you must call them with all the data for one
           frame exactly. You must also initialize 'width' and
           'height' before initializing them. */

        /* NOTE2: some codecs allow the raw parameters (frame size,
           sample rate) to be changed at any frame. We handle this, so
           you should also take care of it */

        /* here, we use a stream based decoder (mpeg1video), so we
           feed decoder and see if it could decode a frame */
        avpkt.data = inbuf;
        while (avpkt.size > 0) {
            len = avcodec_decode_video2(c, picture, &got_picture, &avpkt);
            if (len < 0) {
                LOGE("Error while decoding frame %d\n", frame);
                return;
            }
            if (got_picture) {
                LOGI("saving frame %3d/%3d\n", frame, frame_read);
                fflush(stdout);

                /* the picture is allocated by the decoder. no need to
                   free it */
				show_picture(picture);
				frame++;
            }
            avpkt.size -= len;
            avpkt.data += len;
        }
    }

    /* some codecs, such as MPEG, transmit the I and P frame with a
       latency of one frame. You must do the following to have a
       chance to get the last frame of the video */
    avpkt.data = NULL;
    avpkt.size = 0;
    len = avcodec_decode_video2(c, picture, &got_picture, &avpkt);
    if (got_picture) {
        LOGE("saving last frame %3d\n", frame);
        fflush(stdout);

        /* the picture is allocated by the decoder. no need to
           free it */
		show_picture(picture);
        frame++;
    }

    avcodec_close(c);
    av_free(c);
    av_free(picture);
}
