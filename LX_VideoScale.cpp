#include "LX_VideoScale.h"

extern "C"
{
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#include <iostream>
using namespace std;

AVFrame * allocYUV420pFrame(int w, int h)
{
	///3 ��ʼ����������ݽṹ
	AVFrame * yuv = av_frame_alloc();
	yuv->format = AV_PIX_FMT_YUV420P;
	yuv->width = w;
	yuv->height = h;
	yuv->pts = 0;
	//����yuv�ռ�
	int ret = av_frame_get_buffer(yuv, 0);
	if (ret != 0)
	{
		return 0;
	}
	
	return yuv;
}
void freeFrame(AVFrame *p)
{
	if (p!=0) av_frame_free(&p);
}

LX_VideoScale::LX_VideoScale()
{
	//ע�����еı������
	avcodec_register_all();
	//ע�����еķ�װ��
	av_register_all();
	//ע����������Э��
	avformat_network_init();
	
}
bool LX_VideoScale::InitScaleFromBGR24ToYUV420P(int inw,int inh,int outw,int outh)
{
	inWidth = inw;
	inHeight = inh;
	outWidth = outw;
	outHeight = outh;
	///2 ��ʼ����ʽת��������
	vsc = sws_getCachedContext(vsc,
		inWidth, inHeight, AV_PIX_FMT_BGR24,	 //Դ���ߡ����ظ�ʽ
		outWidth, outHeight, AV_PIX_FMT_YUV420P,//Ŀ����ߡ����ظ�ʽ
		SWS_BICUBIC,  // �ߴ�仯ʹ���㷨
		0, 0, 0
	);
	if (!vsc)
	{
		cout << "sws_getCachedContext failed!";
		return false;
	}
	return true;
}
void LX_VideoScale::Close()
{
	if (vsc)
	{
		sws_freeContext(vsc);
		vsc = NULL;
	}
}

bool LX_VideoScale::ScaleFromBGR24ToYUV420P(unsigned char * rgb,int linesize, AVFrame*yuv420p_frame)
{
	///rgb to yuv
	//��������ݽṹ
	uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
	//indata[0] bgrbgrbgr
	//plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr 
	indata[0] = (uint8_t*)rgb;
	int insize[AV_NUM_DATA_POINTERS] = { 0 };
	//һ�У������ݵ��ֽ���
	insize[0] = linesize;

	int h = sws_scale(vsc, indata, insize, 0, inHeight, //Դ����
		yuv420p_frame->data, yuv420p_frame->linesize);
	if (h <= 0)
	{
		return false;
	}
	return true;
}

LX_VideoScale::~LX_VideoScale()
{
}
