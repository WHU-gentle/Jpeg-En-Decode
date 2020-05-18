#pragma once

typedef struct
{
    int   width;   /* ��� */
    int   height;  /* �߶� */
    int   stride;  /* ���ֽ��� */
    void* pdata;   /* ָ������ */
} BMP;
//bmp�ļ���ȡ������
class Bmp {
public:
    int  bmp_load(BMP* pb, char* file);
    int  bmp_create(BMP* pb, int w, int h);

    int  bmp_save(BMP* pb, char* file);
    void bmp_free(BMP* pb);
};