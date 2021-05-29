#pragma once
struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;
struct AVStream;
#include <string>

class LX_Make_MP4
{
public:
	LX_Make_MP4();
	~LX_Make_MP4();
	bool Init(const char *url);
	void Close();
	bool AddStream(AVCodecContext *c);
	bool SendHead();
	bool SendPacket(AVPacket *pack);
	bool open();
	bool SendEnd();
	//rtmp flv ·â×°Æ÷
	AVFormatContext *ic = 0;
	std::string filename = "";
	//ÊÓÆµ±àÂëÆ÷Á÷
	AVCodecContext *vc = 0;
	AVStream *vs = 0;
	AVCodecContext *ac = 0;
	AVStream *as = 0;

	int indexOfVideo = -1;
	int indexOfAudio = -1;
};

