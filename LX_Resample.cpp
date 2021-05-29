#include "LX_Resample.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
}
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#include <iostream>
using namespace std;
LX_Resample::LX_Resample()
{
	//注册所有的编解码器
	avcodec_register_all();
	//注册所有的封装器
	av_register_all();
	//注册所有网络协议
	avformat_network_init();
}

AVFrame*allocFLTP_PCMFrame(int channels, int samplesPerChannel)
{
	AVFrame *pcm = NULL;
	///3 音频重采样输出空间分配
	pcm = av_frame_alloc();
	pcm->format = AV_SAMPLE_FMT_FLTP;
	pcm->channels = channels;
	pcm->channel_layout = av_get_default_channel_layout(channels);
	pcm->nb_samples = samplesPerChannel; //一帧音频一通道的采用数量
	int ret = av_frame_get_buffer(pcm, 0); // 给pcm分配存储空间
	if (ret != 0)
	{
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		return NULL;
	}
	return pcm;

}

bool LX_Resample::Init_FromS16ToFLTP(int channels, int sampleRate)
{
	this->channels = channels;
	this->sampleRate = sampleRate;
	///2 音频重采样 上下文初始化
	actx = NULL;
	actx = swr_alloc_set_opts(actx,
		av_get_default_channel_layout(channels), (AVSampleFormat)AV_SAMPLE_FMT_FLTP, sampleRate,//输出格式
		av_get_default_channel_layout(channels), (AVSampleFormat)AV_SAMPLE_FMT_S16, sampleRate, 0, 0);//输入格式
	if (!actx)
	{
		cout << "swr_alloc_set_opts failed!";
		return false;
	}
	int ret = swr_init(actx);
	if (ret != 0)
	{
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		return false;
	}
	cout << "音频重采样 上下文初始化成功!" << endl;
	return true;
}

bool LX_Resample::Init_FromFLTToFLTP(int channels, int sampleRate)
{
	this->channels = channels;
	this->sampleRate = sampleRate;
	///2 音频重采样 上下文初始化
	actx = NULL;
	actx = swr_alloc_set_opts(actx,
		av_get_default_channel_layout(channels), (AVSampleFormat)AV_SAMPLE_FMT_FLTP, sampleRate,//输出格式
		av_get_default_channel_layout(channels), (AVSampleFormat)AV_SAMPLE_FMT_FLT, sampleRate, 0, 0);//输入格式
	if (!actx)
	{
		cout << "swr_alloc_set_opts failed!";
		return false;
	}
	int ret = swr_init(actx);
	if (ret != 0)
	{
		char err[1024] = { 0 };
		av_strerror(ret, err, sizeof(err) - 1);
		cout << err << endl;
		return false;
	}
	cout << "音频重采样 上下文初始化成功!" << endl;
	return true;
}
bool LX_Resample::Resample_FromFLTToFLTP(char *data, AVFrame *dst)
{
	const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
	indata[0] = (uint8_t *)data;
	int len = swr_convert(actx, dst->data, dst->nb_samples, //输出参数，输出存储地址和样本数量
		indata, dst->nb_samples
	);
	if (len <= 0)
	{
		return false;
	}
	return true;
}
bool LX_Resample::Resample_FromS16ToFLTP(char *data, AVFrame *dst)
{
	const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
	indata[0] = (uint8_t *)data;
	int len = swr_convert(actx, dst->data, dst->nb_samples, //输出参数，输出存储地址和样本数量
		indata, dst->nb_samples
	);
	if (len <= 0)
	{
		return false;
	}
	return true;
}
 
void LX_Resample::close()
{
	if (actx)
		swr_free(&actx);
}

LX_Resample::~LX_Resample()
{
}
