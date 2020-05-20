#pragma once
#include"main.h"
class Color {
public:
	void yuv_to_rgb(int y, int u, int v, BYTE* r, BYTE* g, BYTE* b);
	void rgb_to_yuv(BYTE r, BYTE g, BYTE b, int* y, int* u, int* v);
};