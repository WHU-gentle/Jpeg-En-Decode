#pragma once
#include<iostream>
#include"bmp.h"
#include"b2j.h"
#include"bitstream.h"
#include"dct.h"
#include"huffman.h"
#include"quant.h"
#include"color.h"

using namespace std;

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

//++ for fixed point
#define FIXQ          11
#define FLOAT2FIX(f)  ((int)((f) * (1 << 11)))
//-- for fixed point