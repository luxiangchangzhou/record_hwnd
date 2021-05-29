#include "LX_VideoEncoder.h"
extern "C"
{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#include <iostream>
#include <vector>
using namespace std;

LX_VideoEncoder::LX_VideoEncoder()
{
	//ע�����еı������
	avcodec_register_all();
	//ע�����еķ�װ��
	av_register_all();
	//ע����������Э��
	avformat_network_init();
}
bool LX_VideoEncoder::InitVideoCodecH264(int w, int h,int fps)
{
	///4 ��ʼ������������
	//a �ҵ�������
	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
	{
		cout << "Can`t find h264 encoder!" << endl;;
		return false;
	}
	//b ����������������
	vc = avcodec_alloc_context3(codec);
	if (!vc)
	{
		cout << "avcodec_alloc_context3 failed!" << endl;
	}
	//c ���ñ���������
	vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //ȫ�ֲ���
	vc->codec_id = codec->id;
	vc->thread_count = 8;


	vc->bit_rate = 50 * 1024 * 8*2;//ѹ����ÿ����Ƶ��bitλ��С 50kB
	vc->width = w;
	vc->height = h;
	vc->time_base = { 1,1000000 };//ʱ��������õ�΢��
	vc->framerate = { fps,1 };

	//������Ĵ�С������֡һ���ؼ�֡
	vc->gop_size = 100;
	vc->max_b_frames = 0;
	vc->pix_fmt = AV_PIX_FMT_YUV420P;
	//d �򿪱�����������
	int ret = avcodec_open2(vc, 0, 0);
	if (ret != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << buf << endl;
		return false;
	}
	cout << "avcodec_open2 success!" << endl;
	return true;
}
vector<AVPacket*> LX_VideoEncoder::EncodeVideo(AVFrame* frame, int streamIndex, long long pts, long long base)
{
	if (frame != NULL)
	{
		pts = av_rescale_q(pts, AVRational{ 1,(int)base }, vc->time_base);
		frame->pts = pts;
	}
	vector<AVPacket*> vp;
	int ret = avcodec_send_frame(vc, frame);
	if (ret < 0)
	{
		cout << "avcodec_send_frame failed" << endl;
		return vp;
	}

	
	for (;ret>=0;)
	{
		AVPacket *pack = av_packet_alloc();
		ret = avcodec_receive_packet(vc, pack);
		
		if (ret != 0 || pack->size <= 0)
		{
			av_packet_free(&pack);
			if (ret == AVERROR_EOF)
			{
				pack = 0;
				vp.push_back(pack);
			}
			return vp;
		}
		pack->stream_index = streamIndex;
		vp.push_back(pack);
	}
	
	return vp;
}
void LX_VideoEncoder::Close()
{
	if (vc)
	{
		avcodec_free_context(&vc);
	}
	vpts = 0;
}
LX_VideoEncoder::~LX_VideoEncoder()
{
}
