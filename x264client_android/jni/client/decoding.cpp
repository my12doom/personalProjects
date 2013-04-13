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

#define MAX_CACHE 5

int sockfd;

client::client()
{
    pthread_mutex_init(&m_queue_mutex, NULL);
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
}

int client::disconnect()
{
    close(sockfd);
}

int client::get_h264_frame(void *buf)
{
	send(sockfd, "H", 1, 0);

	int frame_size = 0;
	if (recv(sockfd, (char*)&frame_size, 4, NULL) != 4)
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
        avpkt.size = get_a_frame(inbuf);
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
