#pragma once
#include <vector>
struct AVFrame;
struct AVPacket;
struct AVCodecContext;



class LX_AudioEncoder
{
public:
	LX_AudioEncoder();
	~LX_AudioEncoder();

	bool InitAudioCodecAAC(int channels, int sampleRate);
	std::vector<AVPacket*> EncodeAudio(AVFrame* frame,int streamIndex, long long pts,long long base);
	void Close();
	int channels = 2;
	int sampleRate = 44100;
	int apts = 0;
	AVCodecContext *ac = 0; //ÒôÆµ±àÂëÆ÷ÉÏÏÂÎÄ
};

