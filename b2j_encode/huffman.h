#pragma once
#include"main.h"
#include"bitstream.h"
#define MAX_HUFFMAN_CODE_LEN 16

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

//huffman����
/* ����������Ͷ��� */
typedef struct
{
    int symbol; /* ���� */
    int freq;   /* Ƶ�� */
    int group;  /* ���� */
    int depth;  /* �볤 */
    int code;   /* ���� */
} HUFCODEITEM;

/* ���������Ͷ��� */
typedef struct
{
    BYTE         huftab[MAX_HUFFMAN_CODE_LEN + 256]; /* �������� */
    int          first[MAX_HUFFMAN_CODE_LEN];       /* first   */
    int          index[MAX_HUFFMAN_CODE_LEN];       /* index   */
    HUFCODEITEM  codelist[256];/* ����� */
    void* input;        /* input bit stream  */
    void* output;       /* output bit stream */
} HUFCODEC;

//������������
class Huffman {
private:
    BitStream bitStream;
    void huffman_stat_freq(HUFCODEITEM codelist[256], void* stream);
public:
    // flag == 0, init from code freq list
    // flag == 1, init from huffman table
    void huffman_encode_init(HUFCODEC* phc, int flag);
    void huffman_encode_done(HUFCODEC* phc);

    /* һ��ֻ����һ������ */
    bool huffman_encode_step(HUFCODEC* phc, int symbol);

    //����
    void huffman_decode_init(HUFCODEC* phc);
    void huffman_decode_done(HUFCODEC* phc);

    /* һ��ֻ����һ������ */
    int  huffman_decode_step(HUFCODEC* phc);
};