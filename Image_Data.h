#pragma once

#include <windows.h>

struct Image_Data
{
	char *data = 0;
	int size = 0;
	long long pts = 0;
	void Drop();
	Image_Data();
	//�����ռ䣬������data����
	Image_Data(char *data, int size, long long p = 0);
};