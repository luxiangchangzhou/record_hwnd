#pragma once
#include <mutex>
struct AVCodecParameters;
struct AVFrame;
struct SwrContext;


AVFrame*allocFLTP_PCMFrame(int channels,int samplesPerChannel);
void freeFrame(AVFrame *);

class LX_Resample
{
public:
	LX_Resample();
	~LX_Resample();
	bool Init_FromS16ToFLTP(int channels, int sampleRate);
	bool Resample_FromS16ToFLTP(char *data, AVFrame *dst);
	bool Init_FromFLTToFLTP(int channels, int sampleRate);
	bool Resample_FromFLTToFLTP(char *data, AVFrame *dst);
	void close();
	int channels = 2;
	int sampleRate = 44100;
protected:
	SwrContext *actx = 0;

};

