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

	bmp.bmp_load(&b, argv[1]);
	jfif = b2j.jfif_encode(&b);
	bmp.bmp_free(&b);
	b2j.jfif_save(jfif, argv[2]);
	b2j.jfif_free(jfif);

	return 0;
}