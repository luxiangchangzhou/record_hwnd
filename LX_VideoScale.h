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

	//��ʼ�����ظ�ʽת���������ĳ�ʼ��
	bool InitScaleFromBGR24ToYUV420P(int inw, int inh, int outw, int outh);
	void Close();
	bool ScaleFromBGR24ToYUV420P(unsigned char * rgb, int linesize, AVFrame*yuv420p_frame);

	SwsContext *vsc = 0;		//���ظ�ʽת��������
								///�������
	int inWidth;
	int inHeight;
	int outWidth;
	int outHeight;

};

