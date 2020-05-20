#pragma once
#include"main.h"
#include"bitstream.h"
#define MAX_HUFFMAN_CODE_LEN 16

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

//huffman定义
/* 编码表项类型定义 */
typedef struct
{
    int symbol; /* 符号 */
    int freq;   /* 频率 */
    int group;  /* 分组 */
    int depth;  /* 码长 */
    int code;   /* 码字 */
} HUFCODEITEM;

/* 编码器类型定义 */
typedef struct
{
    BYTE         huftab[MAX_HUFFMAN_CODE_LEN + 256]; /* 哈夫曼表 */
    int          first[MAX_HUFFMAN_CODE_LEN];       /* first   */
    int          index[MAX_HUFFMAN_CODE_LEN];       /* index   */
    HUFCODEITEM  codelist[256];/* 编码表 */
    void* input;        /* input bit stream  */
    void* output;       /* output bit stream */
} HUFCODEC;

//哈夫曼编码类
class Huffman {
private:
    BitStream bitStream;
    void huffman_stat_freq(HUFCODEITEM codelist[256], void* stream);
public:
    // flag == 0, init from code freq list
    // flag == 1, init from huffman table
    void huffman_encode_init(HUFCODEC* phc, int flag);
    void huffman_encode_done(HUFCODEC* phc);

    /* 一次只编码一个符号 */
    bool huffman_encode_step(HUFCODEC* phc, int symbol);

    //解码
    void huffman_decode_init(HUFCODEC* phc);
    void huffman_decode_done(HUFCODEC* phc);

    /* 一次只解码一个符号 */
    int  huffman_decode_step(HUFCODEC* phc);
};