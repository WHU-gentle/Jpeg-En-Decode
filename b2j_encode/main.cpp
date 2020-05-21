#include"main.h"
#include"bmp.h"
#include"b2j.h"

int main(int argc, char* argv[]) {
	if (argc < 4)
		cout << "用法："<<endl<<"b2j_encode -e name.bmp target.jpg"<<
		endl<< "b2j_encode -d name.jpg target.bmp";
	//调用类
	Bmp bmp;
	//结构变量
	BMP b; B2J b2j;
	//变量
	void* jfif = NULL;

	if (strcmp(argv[1],"-e")==0) {
		bmp.bmp_load(&b, argv[2]);
		jfif = b2j.jfif_encode(&b);
		bmp.bmp_free(&b);
		b2j.jfif_save(jfif, argv[3]);
		b2j.jfif_free(jfif);
	}
	else if (strcmp(argv[1],"-d")==0) {
		jfif = b2j.jfif_load(argv[2]);
		b2j.jfif_decode(jfif, &b);
		b2j.jfif_free(jfif);
		bmp.bmp_save(&b,argv[3]);
		bmp.bmp_free(&b);
	}

	return 0;
}