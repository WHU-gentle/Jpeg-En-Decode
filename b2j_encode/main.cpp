#include"main.h"

//全局结构
typedef struct
{
	int   width;   /* 宽度 */
	int   height;  /* 高度 */
	int   stride;  /* 行字节数 */
	void* pdata;   /* 指向数据 */
} BMP;

int main(int argc,char *argv[]) {
	if(argc<3)
		cout << "用法：b2j_encode name.bmp target.jpg";
	//调用类
	Bmp bmp;
	//结构变量
	BMP b; B2J b2j;
	//变量
	void* jfif = NULL;

	bmp.bmp_load(&b, argv[1]);
	jfif = b2j.jfif_encode(&b);
	bmp.bmp_free(&b);
	b2j.jfif_save(jfif, argv[2]);
	b2j.jfif_free(jfif);

	return 0;
}