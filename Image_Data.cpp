#include "Image_Data.h"

void Image_Data::Drop()
{
	if (data)
		delete data;
	data = 0;
	size = 0;
}

Image_Data::Image_Data(char *data, int size, long long p)
{
	this->data = new char[size];
	memcpy(this->data, data, size);
	this->size = size;
	this->pts = p;
}
Image_Data::Image_Data()
{
}