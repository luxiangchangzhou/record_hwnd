#pragma once

#include <windows.h>

struct Image_Data
{
	char *data = 0;
	int size = 0;
	long long pts = 0;
	void Drop();
	Image_Data();
	//创建空间，并复制data内容
	Image_Data(char *data, int size, long long p = 0);
};