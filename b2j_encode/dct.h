#pragma once
#include"main.h"
#define DCTSIZE 8  // DCT变换中处理一个图像块的维度为8×8

//dct变换类声明
class Dct {
private:
	int g_inited = 0; // 标志是否进行了初始化
	int g_fdctfactor[64] = { 0 };
	int g_idctfactor[64] = { 0 };
	//AA&N优化算法  ？？？
	const float AAN_DCT_FACTOR[DCTSIZE] = {
		1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
		1.0f, 0.785694958f, 0.541196100f, 0.275899379f,
	};

	void init_fdct_ftab(int* ftab, int* qtab);
public:
	void init_dct_module(void);
	void fdct2d8x8(int* data, int* ftab);
	void idct2d8x8(int* data, int* ftab);
	void init_idct_ftab(int* ftab, int* qtab);
};
