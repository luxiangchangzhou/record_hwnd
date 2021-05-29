#include "LX_Make_MP4.h"
#include <iostream>
#include <string>
using namespace std;
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")

LX_Make_MP4::LX_Make_MP4()
{
	//注册所有的编解码器
	avcodec_register_all();
	//注册所有的封装器
	av_register_all();
	//注册所有网络协议
	avformat_network_init();
}


LX_Make_MP4::~LX_Make_MP4()
{
}


bool LX_Make_MP4::Init(const char *filename)
{
	///5 输出封装器和视频流配置
	//a 创建输出封装器上下文
	int ret = avformat_alloc_output_context2(&ic, 0, 0, filename);
	if (ret != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << buf;
		return false;
	}
	this->filename = filename;
	return true;
}
bool LX_Make_MP4::open()
{
	///打开rtmp 的网络输出IO
	int ret = avio_open(&ic->pb, filename.c_str(), AVIO_FLAG_WRITE);
	if (ret != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << buf << endl;
		return false;
	}
	return true;
}
void LX_Make_MP4::Close()
{
	if (ic)
	{
		avformat_close_input(&ic);
		vs = NULL;
	}
	vc = NULL;
	filename = "";
}
bool LX_Make_MP4::AddStream(AVCodecContext *c)
{
	if (!c)return false;

	//b 添加视频流 
	AVStream *st = avformat_new_stream(ic, NULL);
	if (!st)
	{
		cout << "avformat_new_stream failed" << endl;
		return false;
	}
	st->codecpar->codec_tag = 0;
	//从编码器复制参数
	avcodec_parameters_from_context(st->codecpar, c);
	av_dump_format(ic, 0, filename.c_str(), 1);

	if (c->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		vc = c;
		vs = st;
		indexOfVideo = vs->index;
	}
	else if (c->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		ac = c;
		as = st;
		indexOfAudio = as->index;
	}
	return true;
}


bool LX_Make_MP4::SendHead()
{
	//写入封装头
	int ret = avformat_write_header(ic, NULL);
	if (ret != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << buf << endl;
		return false;
	}
	return true;
}

bool LX_Make_MP4::SendEnd()
{
	//写入尾部信息索引
	if (av_write_trailer(ic) != 0)
	{
		cerr << "av_write_trailer failed!" << endl;
		return false;
	}
	cout << "WriteEnd success!" << endl;
	return true;
}

bool LX_Make_MP4::SendPacket(AVPacket *pack)
{
	int index = pack->stream_index;
	//这个函数内部做了一个时间基数的转换
	//将packet的时间基数转换为ic的videostream的时间基数
	//packet是由videoencoder编码来的,他的time_base由videoencoder指定(我指定为vc->time_base = { 1,fps };)
	//这只是一种表述,实际上AVPacket结构体中并没有time_base
	//我们这里所做的推流,是基于ic变量的,也就是AVFormatContext *ic
	//在AddStream的时候用到了编码器的参数,不过参数里面并没有time_base
	//由ffmpeg的手册可知,在调用avformat_write_header之后time_base才会被确定(但是到底是多少,并没有明确说明),
	//所以这一步的转换是必要的
	if (!pack || pack->size <= 0 || !pack->data)return false;

	AVRational src_time_base;
	AVRational dst_time_base;
	//判断是音频还是视频
	if (vs && vc&& pack->stream_index == vs->index)
	{
		src_time_base = vc->time_base;
		dst_time_base = vs->time_base;
	}
	else if (as && ac&&pack->stream_index == as->index)
	{
		src_time_base = ac->time_base;
		dst_time_base = as->time_base;
	}

	//推流
	pack->pts = av_rescale_q(pack->pts, src_time_base, dst_time_base);
	pack->dts = av_rescale_q(pack->dts, src_time_base, dst_time_base);
	pack->duration = av_rescale_q(pack->duration, src_time_base, dst_time_base);
	int ret = av_interleaved_write_frame(ic, pack);
	//int ret = av_write_frame(ic, pack);
	//av_packet_free(&pack);
	if (ret == 0)
	{
		if (index == indexOfAudio)
		{
			cout << "#" << flush;
		}
		if (index == indexOfVideo)
		{
			cout << "$" << flush;
		}
		return true;
	}
	return false;
}

