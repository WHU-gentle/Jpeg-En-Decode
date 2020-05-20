#pragma once
#include"main.h"
class Quant {
public:
	void quant_encode(int du[64], int qtab[64]);
	void quant_decode(int du[64], int qtab[64]);
};