#pragma once
#include"main.h"
#define DCTSIZE 8  // DCT�任�д���һ��ͼ����ά��Ϊ8��8

//dct�任������
class Dct {
private:
	int g_inited = 0; // ��־�Ƿ�����˳�ʼ��
	int g_fdctfactor[64] = { 0 };
	int g_idctfactor[64] = { 0 };
	//AA&N�Ż��㷨  ������
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
