#include"main.h"

//ȫ�ֽṹ
typedef struct
{
	int   width;   /* ��� */
	int   height;  /* �߶� */
	int   stride;  /* ���ֽ��� */
	void* pdata;   /* ָ������ */
} BMP;

int main(int argc,char *argv[]) {
	if(argc<3)
		cout << "�÷���b2j_encode name.bmp target.jpg";
	//������
	Bmp bmp;
	//�ṹ����
	BMP b; B2J b2j;
	//����
	void* jfif = NULL;

	bmp.load(&b, argv[1]);
	jfif = b2j.encod(&b);
	bmp.free(&b);
	b2j.save(jfif, argv[2]);
	b2j.free(jfif);

	return 0;
}