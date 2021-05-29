#pragma once
#include <vector>
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
class LX_VideoEncoder
{
public:
	LX_VideoEncoder();
	~LX_VideoEncoder();
	bool InitVideoCodecH264(int w, int h, int fps);
	std::vector<AVPacket*> EncodeVideo(AVFrame* frame,int streamIndex, long long pts,long long base);
	void Close();
	AVCodecContext *vc = 0;	//╠ЮбКфВиообнд
	int vpts = 0;
};

