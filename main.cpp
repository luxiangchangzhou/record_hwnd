#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <Devicetopology.h>
#include <Endpointvolume.h>


#include "LX_VideoEncoder.h"
#include "LX_AudioEncoder.h"
#include "LX_Make_MP4.h"
#include "LX_VideoScale.h"
#include "LX_Resample.h"
#include "LX_Timer.h"

#include <windows.h>
#include <iostream>
#include <thread>
#include <list>
#include <mutex>
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
}


using namespace std;



// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);



struct LX_AVFrame
{
	AVFrame*pframe;
	unsigned long long micro;
};


string mp4_name;
BOOL is_mp4_name_valid = FALSE;


LX_Timer timer;//这个timer用于记录截图时的瞬时时间,或者获取音频的时间

BOOL need_quit = FALSE;

LX_VideoEncoder video_encoder;
LONG is_Video_Encoder_Initialized = FALSE;
LX_AudioEncoder audio_encoder;
LONG is_Audio_Encoder_Initialized = FALSE;

LX_Resample resample;

LX_Make_MP4 make_mp4;
mutex mutex_make_mp4;

LX_VideoScale video_scale;

list<LX_AVFrame> video_frame;
mutex mux_video_frame;

list<LX_AVFrame> audio_frame;
mutex mux_audio_frame;


list<AVPacket*> video_packet_list;
mutex mux_video_pkts;



void thread_proc_window_capture(HWND hWnd);
void thread_proc_video_encode();
void thread_proc_audio_capture();
void thread_proc_audio_encode();

void thread_proc_video_make_mp4();


int main(int argc,char**argv)
{
	
	HWND hWnd = (HWND)strtol(argv[1], 0, 0);
	if (hWnd == 0 || !IsWindow(hWnd))
	{
		cout << "错误的窗口句柄!!!!!!" << endl;
		return 0;
	}

	mp4_name = argv[2];
	if (string::npos == mp4_name.find("mp4"))
	{
		cout << "错误的文件类型!!!!!!" << endl;
		return 0;
	}

	FILE * fp = fopen(argv[2], "wb+");
	if (fp == 0)
	{
		cout << "错误的文件路径!!!!!!" << endl;
		return 0;
	}
	fclose(fp);
	



	thread *thread_window_capture;
	thread *thread_video_encode;
	thread *thread_audio_capture;
	thread *thread_audio_encode;
	thread *thread_video_make_mp4;

	thread_window_capture = new thread{ thread_proc_window_capture ,hWnd };
	while (is_Video_Encoder_Initialized != 0);


	
	cout << "spin get" << endl;
	thread_audio_capture = new thread{ thread_proc_audio_capture };
	while (is_Audio_Encoder_Initialized != 0);



	thread_video_encode = new thread{ thread_proc_video_encode };
	thread_video_make_mp4 = new thread{ thread_proc_video_make_mp4 };
	thread_audio_encode = new thread{ thread_proc_audio_encode };



	cin.get();
	need_quit = TRUE;
	cout <<endl<< "begin clear" << endl;



	thread_window_capture->join();
	thread_audio_capture->join();
	thread_audio_encode->join();
	thread_video_encode->join();
	thread_video_make_mp4->join();

	make_mp4.SendEnd();
	delete thread_window_capture;
	delete thread_video_encode;
	delete thread_audio_capture;
	delete thread_audio_encode;

	cin.get();




	return 0;
}


void thread_proc_audio_encode()
{

	for (;;)
	{
		mux_audio_frame.lock();
		if (audio_frame.empty())
		{
			mux_audio_frame.unlock();
			Sleep(0);
		}
		else
		{
			LX_AVFrame lxframe = audio_frame.front();
			AVFrame*pcmframe = lxframe.pframe;
			audio_frame.pop_front();
			mux_audio_frame.unlock();
			vector<AVPacket*>pkts = audio_encoder.EncodeAudio(pcmframe, make_mp4.indexOfAudio, lxframe.micro, 1000000);
			freeFrame(pcmframe);
			if (!pkts.empty())
			{
				mutex_make_mp4.lock();
				for (int i = 0; i < pkts.size(); i++)
				{
					if (pkts[i] != 0)
					{
						make_mp4.SendPacket(pkts[i]);
					}
					else
					{
						mutex_make_mp4.unlock();
						cout << endl << "find audio flush packet!!!!!!!!!!!! thread_proc_audio_encode  return" << endl;
						return;
					}
				}
				mutex_make_mp4.unlock();
			}

		}
	}





}

void thread_proc_audio_capture()
{

	//LX_Timer timer;//这个timer用于记录截图时的瞬时时间,或者获取音频的时间

	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	WAVEFORMATEXTENSIBLE * pwfxex = NULL;
	UINT32 packetLength = 0;
	BOOL bDone = FALSE;
	BYTE *pData;
	DWORD flags;


	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	EXIT_ON_ERROR(hr)

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr)

	hr = pEnumerator->GetDefaultAudioEndpoint(
		eRender, eConsole, &pDevice);
	EXIT_ON_ERROR(hr)

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr)

	hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr);

	//luxiang+
	//这里的处理还是有很多模糊的地方,现在基本确定的是,样本格式windows始终使用float,
	//文档里说可以改,但我懒得改了,毕竟aac用的就是float
	cout << "pwfx->wFormatTag = " << pwfx->wFormatTag << endl;//#define WAVE_FORMAT_PCM     1   #define  WAVE_FORMAT_EXTENSIBLE                 0xFFFE
	cout << "pwfx->nChannels = " << pwfx->nChannels << endl;
	cout << "pwfx->nSamplesPerSec = " << pwfx->nSamplesPerSec << endl;
	cout << "pwfx->nAvgBytesPerSec = " << pwfx->nAvgBytesPerSec << endl;
	cout << "pwfx->nBlockAlign = " << pwfx->nBlockAlign << endl;
	cout << "pwfx->wBitsPerSample = " << pwfx->wBitsPerSample << endl;//the container size, not necessarily the sample size
	cout << "pwfx->cbSize = " << pwfx->cbSize << endl;

	if (pwfx->wFormatTag == WAVE_FORMAT_PCM)
	{
	}
	else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		pwfxex = (WAVEFORMATEXTENSIBLE *)pwfx;
		cout << "waoooo-----------------using WAVE_FORMAT_EXTENSIBLE" << endl;
		cout << "wValidBitsPerSample = " << pwfxex->Samples.wValidBitsPerSample << endl;
		cout << "wSamplesPerBlock = " << pwfxex->Samples.wSamplesPerBlock << endl;
		cout << "dwChannelMask = " << pwfxex->dwChannelMask << endl;

	}
	////////////////////////////////////////////////////////////////////////


	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL);
	EXIT_ON_ERROR(hr)

	// Get the size of the allocated buffer.
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr)

	hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
	EXIT_ON_ERROR(hr)

	// Notify the audio sink which format to use.
	//hr = pMySink->SetFormat(pwfx);
	EXIT_ON_ERROR(hr)

	// Calculate the actual duration of the allocated buffer.
	hnsActualDuration = (double)REFTIMES_PER_SEC *
	bufferFrameCount / pwfx->nSamplesPerSec;

	hr = pAudioClient->Start();  // Start recording.
	EXIT_ON_ERROR(hr)




	//初始化音频编码器
	audio_encoder.InitAudioCodecAAC(pwfx->nChannels, pwfx->nSamplesPerSec);

	//初始化重采样上下文
	resample.Init_FromFLTToFLTP(pwfx->nChannels, pwfx->nSamplesPerSec);

	bool ret = make_mp4.AddStream(audio_encoder.ac);
	if (ret)
	{
		cout << "make_mp4.AddStream(audio_encoder.ac) 成功" << endl;
	}

	ret = make_mp4.open();
	if (ret)
	{
		cout << "make_mp4.open()成功" << endl;
	}
	ret = make_mp4.SendHead();
	if (ret)
	{
		cout << "make_mp4.SendHead()成功" << endl;
	}

	InterlockedAdd(&is_Audio_Encoder_Initialized, 1);





	//现在的问题在于如何去处理留白的声音
	//目前的思路是给全为0的数据(通过测试发现只要数据都一样没啥问题),惭愧啊,对pcm数据还是理解不深~~~
	float * pf = new float[pwfx->nChannels * pwfx->nSamplesPerSec * 10];
	memset(pf, 0, pwfx->nChannels * pwfx->nSamplesPerSec * 10 * sizeof(float));
	int float_data_count_persecond = pwfx->nChannels * pwfx->nSamplesPerSec;
	int float_data_count = float_data_count_persecond * hnsActualDuration / REFTIMES_PER_MILLISEC / 2 / 1000;
	int samples_per_channel = float_data_count / pwfx->nChannels;


	//timer.start_micro(0);

	int codec_frame_size = 1024;//samples per channel
	unsigned long long micro_time_per_frame = 1000000 * codec_frame_size / pwfx->nSamplesPerSec;
	float * pf_frame = new float[pwfx->nChannels*codec_frame_size];
	int pf_frame_byte_size = 0;
	int pf_frame_rest_byte_size = pwfx->nChannels*codec_frame_size * sizeof(float);
	int one_frame_byte_size = pwfx->nChannels*codec_frame_size * sizeof(float);

	// Each loop fills about half of the shared buffer.
	while (need_quit == false)
	{

		unsigned long long time_cap_audio = timer.get_micro();
		unsigned long long time_last_cap_audio = timer.get_micro();

		// Sleep for half the buffer duration.
		Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);


		hr = pCaptureClient->GetNextPacketSize(&packetLength);
		EXIT_ON_ERROR(hr)

		if (packetLength == 0)
		{
			cout << "packetLength == 0 ************************************" << endl;
			//这个时候就需要写留白音频数据进去了,应当写入的音频数据的量就是  (hnsActualDuration / REFTIMES_PER_MILLISEC / 2) 时间内的量
			//fwrite(pf, sizeof(float), float_data_count, fp);
			int cnt = samples_per_channel / codec_frame_size;

			mux_audio_frame.lock();
			for (int i = 0; i < cnt; i++)
			{
				AVFrame *pcm = allocFLTP_PCMFrame(pwfx->nChannels, codec_frame_size);
				resample.Resample_FromFLTToFLTP((char*)pf, pcm);
				audio_frame.push_back(LX_AVFrame{ pcm,time_cap_audio + i*micro_time_per_frame });
			}
			mux_audio_frame.unlock();
		}

		bool need_refresh_time = true;
		while (packetLength != 0)
		{
			if (need_quit)
			{
				break;
			}

			if (need_refresh_time)
			{
				time_cap_audio = timer.get_micro();
			}

			//cout << "packetLength != 0 ---------------------------------------" << endl;
			// Get the available data in the shared buffer.
		GETBUF:
			hr = pCaptureClient->GetBuffer(
				&pData,
				&numFramesAvailable,
				&flags, NULL, NULL);
			EXIT_ON_ERROR(hr)

			//cout << "packetLength = " << packetLength << endl;
			//cout << "numFramesAvailable = " << numFramesAvailable << endl;
			cout << " bytedata_len = " << numFramesAvailable*pwfx->nBlockAlign << endl;
			//cout << "samplesperchannel" << numFramesAvailable*pwfx->nBlockAlign / pwfx->nChannels / sizeof(float) << endl;


			/*
			if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
			{
				cout << "AUDCLNT_BUFFERFLAGS_SILENT" << endl;
				pData = NULL;  // Tell CopyData to write silence.
			}
			*/

			// Copy the available capture data to the audio sink.
			//hr = pMySink->CopyData(pData, numFramesAvailable, &bDone);

			//fwrite(pData, numFramesAvailable*pwfx->nBlockAlign, 1, fp);
			//write_count++;

			int data_len = numFramesAvailable*pwfx->nBlockAlign;
			int data_rest_len = data_len;//还没被拷贝的数据大小

WRITE_FRAME:
			if (pf_frame_byte_size < one_frame_byte_size)//表明需要填充
			{
				if (pf_frame_rest_byte_size > data_rest_len)//刚收到的数据可以塞进去,并且pf_frame还有余量
				{
					memcpy(((char*)pf_frame) + pf_frame_byte_size, pData + (data_len - data_rest_len), data_rest_len);
					pf_frame_byte_size += data_rest_len;
					pf_frame_rest_byte_size = one_frame_byte_size - pf_frame_byte_size;
					need_refresh_time = false;
				}
				else if (pf_frame_rest_byte_size < data_rest_len)//刚收到的数据只能塞一部分进去
				{
					memcpy(((char*)pf_frame) + pf_frame_byte_size, pData + (data_len - data_rest_len), pf_frame_rest_byte_size);
					data_rest_len -= pf_frame_rest_byte_size;



					//到这里就可以封装帧了
					AVFrame *pcm = allocFLTP_PCMFrame(pwfx->nChannels, 1024);
					resample.Resample_FromFLTToFLTP((char*)pf_frame, pcm);
					mux_audio_frame.lock();
					audio_frame.push_back(LX_AVFrame{ pcm,time_cap_audio});
					mux_audio_frame.unlock();
					pf_frame_byte_size = 0;
					pf_frame_rest_byte_size = pwfx->nChannels*codec_frame_size * sizeof(float);



					time_cap_audio = timer.get_micro();
					goto WRITE_FRAME;

				}
				else if (pf_frame_rest_byte_size == data_rest_len)
				{
					memcpy(((char*)pf_frame) + pf_frame_byte_size, pData + (data_len - data_rest_len), data_rest_len);
					pf_frame_byte_size += data_rest_len;
					pf_frame_rest_byte_size = one_frame_byte_size - pf_frame_byte_size;




					//到这里就可以封装帧了
					AVFrame *pcm = allocFLTP_PCMFrame(pwfx->nChannels, 1024);
					resample.Resample_FromFLTToFLTP((char*)pf_frame, pcm);
					mux_audio_frame.lock();
					audio_frame.push_back(LX_AVFrame{ pcm,time_cap_audio });
					mux_audio_frame.unlock();

					


					pf_frame_byte_size = 0;
					pf_frame_rest_byte_size = pwfx->nChannels*codec_frame_size * sizeof(float);
					need_refresh_time = true;
				}
			}

			hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
			EXIT_ON_ERROR(hr)

			//time_cap_audio = timer.get_micro();

			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			EXIT_ON_ERROR(hr)



		}
	}

	mux_audio_frame.lock();
	audio_frame.push_back(LX_AVFrame{ 0,0 });
	mux_audio_frame.unlock();

	delete[] pf;

	cout << "audio capture over" << endl;

	hr = pAudioClient->Stop();  // Stop recording.
	EXIT_ON_ERROR(hr)

Exit:
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
	SAFE_RELEASE(pDevice)
	SAFE_RELEASE(pAudioClient)
	SAFE_RELEASE(pCaptureClient)
	CoUninitialize();


	cout << "audio capture thread exit"<<endl;
}




void thread_proc_window_capture(HWND hWnd)
{
	
	LX_Timer timer_fps_control;
	HDC hdcWindow;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;
	unsigned char* lpbitmap = NULL;
	DWORD dwBmpSize = 0;
	int w = 0;
	int h = 0;

	hdcWindow = GetDC(hWnd);
	// Create a compatible DC, which is used in a BitBlt from the window DC.
	hdcMemDC = CreateCompatibleDC(hdcWindow);

	if (!hdcMemDC)
	{
		MessageBox(0, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
		goto done;
	}

	// Get the client area for size calculation.
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);


	// Create a compatible bitmap from the Window DC.
	hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

	if (!hbmScreen)
	{
		MessageBox(0, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
		goto done;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmScreen);


	w = rcClient.right - rcClient.left;
	h = rcClient.bottom - rcClient.top;
	//初始化视频编码器
	bool ret = video_encoder.InitVideoCodecH264(w, h, 30);//我他妈真的是笑了,帧率这种东西还不能给0,给了0,前几帧会尼玛花屏...
	if (ret)
	{
		cout << "编码器初始化成功" << endl;
	}
	else
	{
		cout << "编码器初始化失败" << endl;
	}
	//初始化图像格式转化上下文
	ret = video_scale.InitScaleFromBGR24ToYUV420P(w, h, w, h);
	if (ret)
	{
		cout << "图像格式转化上下文初始化成功" << endl;
	}
	
	ret = make_mp4.Init(mp4_name.c_str());
	if (ret)
	{
		cout << "mp4封装器初始化成功" << endl;
	}
	else
	{
		cout <<endl<< "mp4封装器初始化失败 exit process" << endl;
		ExitProcess(0);
	}
	
	ret = make_mp4.AddStream(video_encoder.vc);
	if (ret )
	{
		cout << "make_mp4.AddStream(video_encoder.vc) 成功" << endl;
	}
	





	InterlockedAdd(&is_Video_Encoder_Initialized,1);


	timer.start_micro(0);
	for (;need_quit == FALSE;)
	{
		timer_fps_control.start_ms(0);
		// Bit block transfer into our compatible memory DC.
		if (!BitBlt(hdcMemDC,
			0, 0,
			rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
			hdcWindow,
			0, 0,
			SRCCOPY))
		{

			cout << "BitBlt has failed" << endl;
			//MessageBox(0, L"BitBlt has failed", L"Failed", MB_OK);
			goto done;
		}
		unsigned long long picture_time = timer.get_micro();

		// Get the BITMAP from the HBITMAP.
		GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

		BITMAPFILEHEADER   bmfHeader;
		BITMAPINFOHEADER   bi;

		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = bmpScreen.bmWidth;
		bi.biHeight = -1*bmpScreen.bmHeight;//巧妙解决图片翻转问题,哈哈
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		//扫描行必须四字节对齐
		int x = ((bmpScreen.bmWidth * 3) % 4);
		if (x == 0)
		{
			dwBmpSize = bmpScreen.bmWidth * 3 * bmpScreen.bmHeight;
		}
		else
		{
			dwBmpSize = (bmpScreen.bmWidth * 3 + 4 - x) * bmpScreen.bmHeight;
		}

		if (lpbitmap == NULL)
		{
			lpbitmap = new unsigned char[dwBmpSize];
		}
		

		GetDIBits(hdcWindow, hbmScreen, 0,
			(UINT)bmpScreen.bmHeight,
			lpbitmap,
			(BITMAPINFO*)&bi, DIB_RGB_COLORS);

		//至此已将设备无关位图数据复制到准备的缓存中 lpbitmap
		//接下来就是将RGB图像数据做像素格式转换至YUV420P
		AVFrame * yuvframe = allocYUV420pFrame(w,h);
		
		bool ret = video_scale.ScaleFromBGR24ToYUV420P(lpbitmap,dwBmpSize / bmpScreen.bmHeight, yuvframe);
		if (ret == false)
		{
			//cout << "ScaleFromBGR24ToYUV420P failed" << endl;
		}
		else
		{
			//cout << "ScaleFromBGR24ToYUV420P success" << endl;
		}

		mux_video_frame.lock();
		video_frame.push_back(LX_AVFrame{ yuvframe,picture_time });
		mux_video_frame.unlock();

		//freeFrame(yuvframe);
		//cout << "cost time = " << timer.get_ms() << "ms" << endl;
		int elapsed_time_ms = timer_fps_control.get_ms();
		if (elapsed_time_ms < 30)
		{
			Sleep(30 - elapsed_time_ms);
		}

		
	}
	
	mux_video_frame.lock();
	video_frame.push_back(LX_AVFrame{ 0,0 });
	mux_video_frame.unlock();


done:
	delete[] lpbitmap;
	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(hWnd, hdcWindow);
	cout << "window capture thread return" << endl;

}


void thread_proc_video_encode()
{
	for (;;)
	{
		mux_video_frame.lock();
		if (video_frame.empty())
		{
			mux_video_frame.unlock();
			Sleep(0);
		}
		else
		{
			LX_AVFrame lxframe = video_frame.front();
			AVFrame*yuvframe = lxframe.pframe;
			video_frame.pop_front();
			mux_video_frame.unlock();


			//LX_Timer timer_encode;
			//timer_encode.start_ms(0);

			vector<AVPacket*>pkts = video_encoder.EncodeVideo(yuvframe, make_mp4.indexOfVideo, lxframe.micro, 1000000);

			//cout <<endl<< "time_ecode = " << timer_encode.get_ms() << " ms" << endl;;


			freeFrame(yuvframe);
			if (!pkts.empty())
			{

				mux_video_pkts.lock();
				for (int i = 0;i < pkts.size();i++)
				{
					video_packet_list.push_back(pkts[i]);
					if (pkts[i] == 0)
					{
						mux_video_pkts.unlock();
						return;
					}
				}
				mux_video_pkts.unlock();




				
			}

		}
	}

	
	
}




void thread_proc_video_make_mp4()
{


	for (;;)
	{
		mux_video_pkts.lock();
		if (video_packet_list.empty())
		{
			mux_video_pkts.unlock();
			Sleep(0);
		}
		else
		{
			AVPacket * pkt = video_packet_list.front();
			video_packet_list.pop_front();
			mux_video_pkts.unlock();

			mutex_make_mp4.lock();
			if (pkt != 0)
			{
				make_mp4.SendPacket(pkt);
			}
			else
			{
				mutex_make_mp4.unlock();
				cout << endl << "find video flush packet!!!!!!!!!!!! thread_proc_video_encode  return" << endl;
				return;
			}
			mutex_make_mp4.unlock();


		}
		

		
	}




}