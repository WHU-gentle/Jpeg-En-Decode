#include"dct.h"

//dct变换类
	// DCT 模块初始化
void Dct::init_dct_module(void)
{
	int i, j;
	float factor[64];
	// 初始化检测
	if (g_inited) return;
	else g_inited = 1;
	// encode阶段的fdct
	for (i = 0; i < DCTSIZE; i++) {
		for (j = 0; j < DCTSIZE; j++)
			factor[i * 8 + j] = 1.0f / (AAN_DCT_FACTOR[i] * AAN_DCT_FACTOR[j] * 8);
	}
	for (i = 0; i < 64; i++)
		g_fdctfactor[i] = FLOAT2FIX(factor[i]);  // *2048
	//decode阶段idct变化
	for (i = 0; i < DCTSIZE; i++) {
		for (j = 0; j < DCTSIZE; j++)
			factor[i * 8 + j] = 1.0f * (AAN_DCT_FACTOR[i] * AAN_DCT_FACTOR[j] / 8);
	}
	for (i = 0; i < 64; i++)
		g_idctfactor[i] = FLOAT2FIX(factor[i]);
}


// 初始化fdct的factor table
void Dct::init_fdct_ftab(int* ftab, int* qtab)
{
	int i, j;
	float factor[64];
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++)
			factor[i * 8 + j] = 1.0f / (AAN_DCT_FACTOR[i] * AAN_DCT_FACTOR[j] * 8);
	}
	// 量化
	for (i = 0; i < 64; i++) {
		ftab[i] = FLOAT2FIX(factor[i] / qtab[i]);
	}
}

void Dct::init_idct_ftab(int* ftab, int* qtab)
{
	int i, j;
	float factor[64];
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++)
			factor[i * 8 + j] = 1.0f * (AAN_DCT_FACTOR[i] * AAN_DCT_FACTOR[j] / 8);
	}
	for (i = 0; i < 64; i++)
		ftab[i] = FLOAT2FIX(factor[i] * qtab[i]);
}


//  跟AA&N相关的数据变换
void Dct::fdct2d8x8(int* data, int* ftab)
{
	int tmp0, tmp1, tmp2, tmp3;
	int tmp4, tmp5, tmp6, tmp7;
	int tmp10, tmp11, tmp12, tmp13;
	int z1, z2, z3, z4, z5, z11, z13;
	int* dataptr;
	int ctr;

	/* Pass 1: process rows. */
	dataptr = data;
	for (ctr = 0; ctr < DCTSIZE; ctr++)
	{
		tmp0 = dataptr[0] + dataptr[7];
		tmp7 = dataptr[0] - dataptr[7];
		tmp1 = dataptr[1] + dataptr[6];
		tmp6 = dataptr[1] - dataptr[6];
		tmp2 = dataptr[2] + dataptr[5];
		tmp5 = dataptr[2] - dataptr[5];
		tmp3 = dataptr[3] + dataptr[4];
		tmp4 = dataptr[3] - dataptr[4];

		/* Even part */
		tmp10 = tmp0 + tmp3;  /* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[0] = tmp10 + tmp11;  /* phase 3 */
		dataptr[4] = tmp10 - tmp11;

		z1 = (tmp12 + tmp13) * FLOAT2FIX(0.707106781f) >> FIXQ; /* c4 */
		dataptr[2] = tmp13 + z1;  /* phase 5 */
		dataptr[6] = tmp13 - z1;

		/* Odd part */
		tmp10 = tmp4 + tmp5;  /* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * FLOAT2FIX(0.382683433f) >> FIXQ;  /* c6 */
		z2 = (FLOAT2FIX(0.541196100f) * tmp10 >> FIXQ) + z5;     /* c2-c6 */
		z4 = (FLOAT2FIX(1.306562965f) * tmp12 >> FIXQ) + z5;     /* c2+c6 */
		z3 = tmp11 * FLOAT2FIX(0.707106781f) >> FIXQ;            /* c4 */

		z11 = tmp7 + z3;        /* phase 5 */
		z13 = tmp7 - z3;

		dataptr[5] = z13 + z2;  /* phase 6 */
		dataptr[3] = z13 - z2;
		dataptr[1] = z11 + z4;
		dataptr[7] = z11 - z4;

		dataptr += DCTSIZE;     /* advance pointer to next row */
	}

	/* Pass 2: process columns. */
	dataptr = data;
	for (ctr = 0; ctr < DCTSIZE; ctr++)
	{
		tmp0 = dataptr[DCTSIZE * 0] + dataptr[DCTSIZE * 7];
		tmp7 = dataptr[DCTSIZE * 0] - dataptr[DCTSIZE * 7];
		tmp1 = dataptr[DCTSIZE * 1] + dataptr[DCTSIZE * 6];
		tmp6 = dataptr[DCTSIZE * 1] - dataptr[DCTSIZE * 6];
		tmp2 = dataptr[DCTSIZE * 2] + dataptr[DCTSIZE * 5];
		tmp5 = dataptr[DCTSIZE * 2] - dataptr[DCTSIZE * 5];
		tmp3 = dataptr[DCTSIZE * 3] + dataptr[DCTSIZE * 4];
		tmp4 = dataptr[DCTSIZE * 3] - dataptr[DCTSIZE * 4];

		/* Even part */
		tmp10 = tmp0 + tmp3;  /* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[DCTSIZE * 0] = tmp10 + tmp11;  /* phase 3 */
		dataptr[DCTSIZE * 4] = tmp10 - tmp11;

		z1 = (tmp12 + tmp13) * FLOAT2FIX(0.707106781f) >> FIXQ; /* c4 */
		dataptr[DCTSIZE * 2] = tmp13 + z1;  /* phase 5 */
		dataptr[DCTSIZE * 6] = tmp13 - z1;

		/* Odd part */
		tmp10 = tmp4 + tmp5;  /* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * FLOAT2FIX(0.382683433f) >> FIXQ;  /* c6 */
		z2 = (FLOAT2FIX(0.541196100f) * tmp10 >> FIXQ) + z5;     /* c2-c6 */
		z4 = (FLOAT2FIX(1.306562965f) * tmp12 >> FIXQ) + z5;     /* c2+c6 */
		z3 = tmp11 * FLOAT2FIX(0.707106781f) >> FIXQ;            /* c4 */

		z11 = tmp7 + z3;  /* phase 5 */
		z13 = tmp7 - z3;

		dataptr[DCTSIZE * 5] = z13 + z2; /* phase 6 */
		dataptr[DCTSIZE * 3] = z13 - z2;
		dataptr[DCTSIZE * 1] = z11 + z4;
		dataptr[DCTSIZE * 7] = z11 - z4;

		dataptr++;  /* advance pointer to next column */
	}

	ftab = ftab ? ftab : g_fdctfactor;
	for (ctr = 0; ctr < 64; ctr++) {
		data[ctr] *= ftab[ctr];
		data[ctr] >>= FIXQ + 2;
	}
}

//解码过程
void Dct::idct2d8x8(int* data, int* ftab)
{
	int  tmp0, tmp1, tmp2, tmp3;
	int  tmp4, tmp5, tmp6, tmp7;
	int  tmp10, tmp11, tmp12, tmp13;
	int  z5, z10, z11, z12, z13;
	int* dataptr;
	int  ctr;

	ftab = ftab ? ftab : g_idctfactor;
	for (ctr = 0; ctr < 64; ctr++) {
		data[ctr] *= ftab[ctr];
	}

	/* Pass 1: process rows. */
	dataptr = data;
	for (ctr = 0; ctr < DCTSIZE; ctr++)
	{
		if (dataptr[1] + dataptr[2] + dataptr[3] + dataptr[4]
			+ dataptr[5] + dataptr[6] + dataptr[7] == 0)
		{
			dataptr[1] = dataptr[0];
			dataptr[2] = dataptr[0];
			dataptr[3] = dataptr[0];
			dataptr[4] = dataptr[0];
			dataptr[5] = dataptr[0];
			dataptr[6] = dataptr[0];
			dataptr[7] = dataptr[0];

			dataptr += DCTSIZE;
			continue;
		}

		/* Even part */
		tmp0 = dataptr[0];
		tmp1 = dataptr[2];
		tmp2 = dataptr[4];
		tmp3 = dataptr[6];

		tmp10 = tmp0 + tmp2;    /* phase 3 */
		tmp11 = tmp0 - tmp2;

		tmp13 = tmp1 + tmp3;   /* phases 5-3 */
		tmp12 = tmp1 - tmp3;   /* 2 * c4 */
		tmp12 *= FLOAT2FIX(1.414213562f); tmp12 >>= FIXQ;
		tmp12 -= tmp13;

		tmp0 = tmp10 + tmp13;   /* phase 2 */
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;

		/* Odd part */
		tmp4 = dataptr[1];
		tmp5 = dataptr[3];
		tmp6 = dataptr[5];
		tmp7 = dataptr[7];

		z13 = tmp6 + tmp5;    /* phase 6 */
		z10 = tmp6 - tmp5;
		z11 = tmp4 + tmp7;
		z12 = tmp4 - tmp7;

		tmp7 = z11 + z13;   /* phase 5 */
		tmp11 = z11 - z13;   /* 2 * c4 */
		tmp11 *= FLOAT2FIX(1.414213562f); tmp11 >>= FIXQ;

		z5 = (z10 + z12) * FLOAT2FIX(1.847759065f) >> FIXQ;  /*  2 * c2 */
		tmp10 = (FLOAT2FIX(1.082392200f) * z12 >> FIXQ) - z5; /*  2 * (c2 - c6) */
		tmp12 = (FLOAT2FIX(-2.613125930f) * z10 >> FIXQ) + z5; /* -2 * (c2 + c6) */

		tmp6 = tmp12 - tmp7;    /* phase 2 */
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		dataptr[0] = tmp0 + tmp7;
		dataptr[7] = tmp0 - tmp7;
		dataptr[1] = tmp1 + tmp6;
		dataptr[6] = tmp1 - tmp6;
		dataptr[2] = tmp2 + tmp5;
		dataptr[5] = tmp2 - tmp5;
		dataptr[4] = tmp3 + tmp4;
		dataptr[3] = tmp3 - tmp4;

		dataptr += DCTSIZE;
	}

	/* Pass 2: process columns. */
	dataptr = data;
	for (ctr = 0; ctr < DCTSIZE; ctr++)
	{
		/* Even part */
		tmp0 = dataptr[DCTSIZE * 0];
		tmp1 = dataptr[DCTSIZE * 2];
		tmp2 = dataptr[DCTSIZE * 4];
		tmp3 = dataptr[DCTSIZE * 6];

		tmp10 = tmp0 + tmp2;    /* phase 3 */
		tmp11 = tmp0 - tmp2;

		tmp13 = tmp1 + tmp3;   /* phases 5-3 */
		tmp12 = tmp1 - tmp3;   /* 2 * c4 */
		tmp12 *= FLOAT2FIX(1.414213562f); tmp12 >>= FIXQ;
		tmp12 -= tmp13;

		tmp0 = tmp10 + tmp13;   /* phase 2 */
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;

		/* Odd part */
		tmp4 = dataptr[DCTSIZE * 1];
		tmp5 = dataptr[DCTSIZE * 3];
		tmp6 = dataptr[DCTSIZE * 5];
		tmp7 = dataptr[DCTSIZE * 7];

		z13 = tmp6 + tmp5;    /* phase 6 */
		z10 = tmp6 - tmp5;
		z11 = tmp4 + tmp7;
		z12 = tmp4 - tmp7;

		tmp7 = z11 + z13;   /* phase 5 */
		tmp11 = z11 - z13;   /* 2 * c4 */
		tmp11 *= FLOAT2FIX(1.414213562f); tmp11 >>= FIXQ;

		z5 = (z10 + z12) * FLOAT2FIX(1.847759065f) >> FIXQ;  /*  2 * c2 */
		tmp10 = (FLOAT2FIX(1.082392200f) * z12 >> FIXQ) - z5; /*  2 * (c2 - c6) */
		tmp12 = (FLOAT2FIX(-2.613125930f) * z10 >> FIXQ) + z5; /* -2 * (c2 + c6) */

		tmp6 = tmp12 - tmp7;    /* phase 2 */
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		dataptr[DCTSIZE * 0] = tmp0 + tmp7;
		dataptr[DCTSIZE * 7] = tmp0 - tmp7;
		dataptr[DCTSIZE * 1] = tmp1 + tmp6;
		dataptr[DCTSIZE * 6] = tmp1 - tmp6;
		dataptr[DCTSIZE * 2] = tmp2 + tmp5;
		dataptr[DCTSIZE * 5] = tmp2 - tmp5;
		dataptr[DCTSIZE * 4] = tmp3 + tmp4;
		dataptr[DCTSIZE * 3] = tmp3 - tmp4;

		dataptr++; /* advance pointers to next column */
	}
}