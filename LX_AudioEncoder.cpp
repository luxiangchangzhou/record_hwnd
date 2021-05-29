#include "LX_AudioEncoder.h"

extern "C"
{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")

#include <iostream>
#include <vector>
using namespace std;







LX_AudioEncoder::LX_AudioEncoder()
{
	//ע�����еı������
	avcodec_register_all();
	//ע�����еķ�װ��
	av_register_all();
	//ע����������Э��
	avformat_network_init();
}

bool LX_AudioEncoder::InitAudioCodecAAC(int channels,int sampleRate)
{
	this->channels = channels;
	this->sampleRate = sampleRate;
	///4 ��ʼ����Ƶ������
	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!codec)
	{
		cout << "avcodec_find_encoder AV_CODEC_ID_AAC failed!" << endl;
		getchar();
		return false;
	}
	//��Ƶ������������
	ac = avcodec_alloc_context3(codec);
	if (!ac)
	{
		cout << "avcodec_alloc_context3 AV_CODEC_ID_AAC failed!" << endl;
		getchar();
		return false;
	}
	cout << "avcodec_alloc_context3 success!" << endl;

	ac->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	ac->thread_count = 8;
	ac->bit_rate = 40000*2;
	ac->sample_rate = sampleRate;
	ac->sample_fmt = AV_SAMPLE_FMT_FLTP;
	ac->channels = channels;
	ac->channel_layout = av_get_default_channel_layout(channels);

	//����Ƶ������
	int ret = avcodec_open2(ac, 0, 0);
	if (ret != 0)
	{
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		getchar();
		return false;
	}
	cout << "avcodec_open2 success!" << endl;
	return true;
}

void LX_AudioEncoder::Close()
{
	apts = 0;
	if (ac)
	{
		avcodec_free_context(&ac);
	}
}

vector<AVPacket*> LX_AudioEncoder::EncodeAudio(AVFrame* frame, int streamIndex, long long pts, long long base)
{
	vector<AVPacket*> vp;
	if (frame!=NULL)
	{
		//��ptsԭʼʱ�����ת��Ϊ��������ʱ�����
		pts = av_rescale_q(pts, AVRational{ 1,(int)base }, ac->time_base);
		//��Ƶ��ʱ������������ʾ���,Ҳ����{1,sampleRate}
		frame->pts = pts;
	}
	
	int ret = avcodec_send_frame(ac, frame);
	if (ret != 0)
		return vp;

	for (;ret >= 0;)
	{
		AVPacket *apack = av_packet_alloc();
		ret = avcodec_receive_packet(ac, apack);
		if (ret != 0 || apack->size <= 0)
		{
			av_packet_free(&apack);
			if (ret == AVERROR_EOF)
			{
				apack = 0;
				vp.push_back(apack);
			}
			return vp;
		}
		apack->stream_index = streamIndex;
		vp.push_back(apack);
	}

	return vp;
	
}

LX_AudioEncoder::~LX_AudioEncoder()
{
}
