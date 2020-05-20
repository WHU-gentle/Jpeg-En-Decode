#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>

using namespace std;

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

//++ for fixed point
#define FIXQ          11
#define FLOAT2FIX(f)  ((int)((f) * (1 << 11)))
//-- for fixed point

//bitStream¶¨Òå
enum {
    BITSTR_MEM = 0,
    BITSTR_FILE
};
// file bitstr
typedef struct {
    int type;
    DWORD bitbuf;
    int bitnum;
    FILE* fp;
}FBITSTR;

typedef struct {
    int type;
    DWORD bitbuf;
    int bitnum;
    BYTE* membuf;
    int memlen;  // memory len
    int curpos;  // current position
}MBITSTR;

