#pragma once
#include"main.h"
//dct�任������
class Dct {
public:
	void init_dct_module(void);
	void fdct2d8x8(int* data, int* ftab);
};
