#pragma once
struct SwsContext;
struct AVFrame;

AVFrame * allocYUV420pFrame(int w,int h);
void freeFrame(AVFrame *);
class LX_VideoScale
{
public:
	LX_VideoScale();
	~LX_VideoScale();

	//初始化像素格式转换的上下文初始化
	bool InitScaleFromBGR24ToYUV420P(int inw, int inh, int outw, int outh);
	void Close();
	bool ScaleFromBGR24ToYUV420P(unsigned char * rgb, int linesize, AVFrame*yuv420p_frame);

	SwsContext *vsc = 0;		//像素格式转换上下文
								///输入参数
	int inWidth;
	int inHeight;
	int outWidth;
	int outHeight;

};

