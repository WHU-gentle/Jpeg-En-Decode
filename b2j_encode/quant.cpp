#include "quant.h"

class Quant {
public:
	void quant_encode(int du[64], int qtab[64])
	{
		for (int i = 0; i < 64; i++)
			du[i] /= qtab[i];
	}
	void quant_decode(int du[64], int qtab[64])
	{
		for (int i = 0; i < 64; i++)
			du[i] *= qtab[i];
	}
};
