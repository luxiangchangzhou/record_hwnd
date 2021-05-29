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
	//ע�����еı������
	avcodec_register_all();
	//ע�����еķ�װ��
	av_register_all();
	//ע����������Э��
	avformat_network_init();
}

AVFrame*allocFLTP_PCMFrame(int channels, int samplesPerChannel)
{
	AVFrame *pcm = NULL;
	///3 ��Ƶ�ز�������ռ����
	pcm = av_frame_alloc();
	pcm->format = AV_SAMPLE_FMT_FLTP;
	pcm->channels = channels;
	pcm->channel_layout = av_get_default_channel_layout(channels);
	pcm->nb_samples = samplesPerChannel; //һ֡��Ƶһͨ���Ĳ�������
	int ret = av_frame_get_buffer(pcm, 0); // ��pcm����洢�ռ�
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
	///2 ��Ƶ�ز��� �����ĳ�ʼ��
	actx = NULL;
	actx = swr_alloc_set_opts(actx,
		av_get_default_channel_layout(channels), (AVSampleFormat)AV_SAMPLE_FMT_FLTP, sampleRate,//�����ʽ
		av_get_default_channel_layout(channels), (AVSampleFormat)AV_SAMPLE_FMT_S16, sampleRate, 0, 0);//�����ʽ
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
	cout << "��Ƶ�ز��� �����ĳ�ʼ���ɹ�!" << endl;
	return true;
}

bool LX_Resample::Init_FromFLTToFLTP(int channels, int sampleRate)
{
	this->channels = channels;
	this->sampleRate = sampleRate;
	///2 ��Ƶ�ز��� �����ĳ�ʼ��
	actx = NULL;
	actx = swr_alloc_set_opts(actx,
		av_get_default_channel_layout(channels), (AVSampleFormat)AV_SAMPLE_FMT_FLTP, sampleRate,//�����ʽ
		av_get_default_channel_layout(channels), (AVSampleFormat)AV_SAMPLE_FMT_FLT, sampleRate, 0, 0);//�����ʽ
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
	cout << "��Ƶ�ز��� �����ĳ�ʼ���ɹ�!" << endl;
	return true;
}
bool LX_Resample::Resample_FromFLTToFLTP(char *data, AVFrame *dst)
{
	const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
	indata[0] = (uint8_t *)data;
	int len = swr_convert(actx, dst->data, dst->nb_samples, //�������������洢��ַ����������
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
	int len = swr_convert(actx, dst->data, dst->nb_samples, //�������������洢��ַ����������
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
