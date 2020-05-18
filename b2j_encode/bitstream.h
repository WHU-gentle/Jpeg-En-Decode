#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define USE_JPEG_BITSTR 1  // 是否使用JPEG格式的编码
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


	class BitStream {
	public:
		void* bitstr_open(void* fnamebuf, char* fmode, int bufsize);
		int   bitstr_close(void* stream);
		int   bitstr_getc(void* stream);
		int   bitstr_putc(int c, void* stream);
		int   bitstr_seek(void* stream, long offset, int origin);
		long  bitstr_tell(void* stream);
		int   bitstr_getb(void* stream);
		int   bitstr_putb(int b, void* stream);
		int   bitstr_get_bits(void* stream, int n);
		int   bitstr_put_bits(void* stream, int bits, int n);
		int   bitstr_flush(void* stream, int flag);
	};
}