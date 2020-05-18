#pragma once

typedef struct
{
    int   width;   /* 宽度 */
    int   height;  /* 高度 */
    int   stride;  /* 行字节数 */
    void* pdata;   /* 指向数据 */
} BMP;
//bmp文件读取类声明
class Bmp {
public:
    int  bmp_load(BMP* pb, char* file);
    int  bmp_create(BMP* pb, int w, int h);

    int  bmp_save(BMP* pb, char* file);
    void bmp_free(BMP* pb);
};