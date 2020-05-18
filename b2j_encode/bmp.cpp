#include"main.h"

//BMP文件头
typedef struct {
	WORD   bfType;  //必须为"BM"
	DWORD  bfSize;  //整个BMP文件的大小
	WORD   bfReserved1;  
	WORD   bfReserved2;
	DWORD  bfOffBits;  //文件起始位置到图像像素数据的字节偏移量
	DWORD  biSize;   // BMPFILEHEADER结构体大小
	DWORD  biWidth;  //图像宽度
	DWORD  biHeight; //图像高度
	WORD   biPlanes; //图像数据平面 RGB数据总为1
	WORD   biBitCount; //图像像素的位数
	DWORD  biCompression;  //压缩方式
	DWORD  biSizeImage;  //4字节对齐的图像数据大小
	DWORD  biXPelsPerMeter; //像素/米表示的水平分辨率
	DWORD  biYPelsPerMeter; //水平分辨率
	DWORD  biClrUsed;  //实际使用的调色板索引数
	DWORD  biClrImportant;  //重要的调色板索引数
} BMPFILEHEADER;

//bmp�ļ���ȡ��
class Bmp {
	//BMP图像的主要数据
	int width;
	int height;
	int stride;
	void *pdata;

	// 字节对齐
	static int ALIGN(int x, int y){
		return (x+y-1)&~(y-1);
	}
	//BMP文件加载
	int bmp_load(BMP *pb, char *file)
	{
		BMPFILEHEADER header = {0};  // 初始化一个BMP文件头
		FILE *fp = NULL;  // 文件流指针
		BYTE *pdata = NULL;
		int i;

		fp = fopen(file, "rb");  // 打开文件流
		if(!fp) return -1;

		fread(&header, sizeof(header), 1, fp);  // 从文件流中读取文件头
		pb->width = header.biWidth;
		pb->height = header.biHeight;
		pb->stride = ALIGN(header.biWidth * 3, 4);
		pb->pdata = malloc(pb->stride * pb->height);
		if(pb->pdata){
			pdata = (BYTE*)pb->pdata + pb->stride * pb->height;
			//先读的数据放在下面
			for(int i=0;i<pb->height;i++){
				pdata -= pb->stride;
				fread(pdata, pb->stride, 1 ,fp);
			}
		}
		fclose(fp);
		return pb->pdata?0:-1;  //返回0说明成功
	}

	int bmp_create(BMP *pb, int w, int h)
	{
		pb->width = w;
		pb->height = h;
		pb->stride = ALIGN(w*3, 4);
		pb->data = malloc(pb->stride*h);
		return pb->pdata?0:-1;
	}

	int bmp_save(BMP *pb, char *file)
	{
		//分配空间 定义参数
		BMPFILEHEADER header = {0};
		FILE *fp = NULL;
		BYTE *pdata;
		int i;
		// 填入信息
		header.bfType = ('B' << 0) | ('M' << 8);  //BMP文件类型
		header.bfSize = sizeof(header) + pb->stride * pb->height;
		header.bfOffBits = sizeof(header); //文件头后即为图片起始位置
		header.biSize = 40;
		header.biWidth = pb->width;
		header.biHeight = pb->height;
		header.biPlanes = 1;
		header.biBitCount = 24;  //像素位数24位
		header.biSizeImage = pb->stride * pb->height;
		// 写入文件
		fp = fopen(file, "wb");
		if(fp){
			fwrite(&header, sizeof(header), 1, fp);
			pdata = (BYTE*)pb->data + pb->stride * pb->height;
			for(i=0;i<pb->height;i++){
				pdata -= pb->stride;
				fwrite(pdata, pb->stride, 1, fp)
			}
			fclose(fp);
		}
		return fp?0:-1;
	}

	void bmp_free(BMP *pb)
	{
    	if (pb->pdata) {
        	free(pb->pdata);
        	pb->pdata = NULL;
    	}
    	pb->width  = 0;
    	pb->height = 0;
    	pb->stride = 0;
	}
};