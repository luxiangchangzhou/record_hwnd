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
	///3 初始化输出的数据结构
	AVFrame * yuv = av_frame_alloc();
	yuv->format = AV_PIX_FMT_YUV420P;
	yuv->width = w;
	yuv->height = h;
	yuv->pts = 0;
	//分配yuv空间
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
	//注册所有的编解码器
	avcodec_register_all();
	//注册所有的封装器
	av_register_all();
	//注册所有网络协议
	avformat_network_init();
	
}
bool LX_VideoScale::InitScaleFromBGR24ToYUV420P(int inw,int inh,int outw,int outh)
{
	inWidth = inw;
	inHeight = inh;
	outWidth = outw;
	outHeight = outh;
	///2 初始化格式转换上下文
	vsc = sws_getCachedContext(vsc,
		inWidth, inHeight, AV_PIX_FMT_BGR24,	 //源宽、高、像素格式
		outWidth, outHeight, AV_PIX_FMT_YUV420P,//目标宽、高、像素格式
		SWS_BICUBIC,  // 尺寸变化使用算法
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
	//输入的数据结构
	uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
	//indata[0] bgrbgrbgr
	//plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr 
	indata[0] = (uint8_t*)rgb;
	int insize[AV_NUM_DATA_POINTERS] = { 0 };
	//一行（宽）数据的字节数
	insize[0] = linesize;

	int h = sws_scale(vsc, indata, insize, 0, inHeight, //源数据
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
