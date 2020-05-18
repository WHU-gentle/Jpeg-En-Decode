#include"main.h"

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
	FILE *fp;
}FBITSTR;

typedef struct {
	int type;
	DWORD bitbuf;
	int bitnum;
	BYTE *membuf;
	int memlen;  // memory len
	int curpos;  // current position
}MBITSTR;

class BitStream {
	/*从文件打开bit流*/
	static void* fbitstr_open(char *file, char *mode)
	{
		//分配空间
		FBITSTR *context = calloc(1, sizeof(FBITSTR));
		if(!context) return null;  // 分配失败返回空

		context->type = BITSTR_FILE;
		context->fp = fopen(file, mode);
		// 文件流不为空就返回相应的FBITSTR结构
		if(!context->fp){
			free(context);
			return null;
		}
		else return context;
	}
	// 关闭文件字节流
	static int fbitstr_close(void *stream)
	{
		FBITSTR *context = (FBITSTR*)stream;
		if(!context || !context->fp) return EOF;
		fclose(context->fp);
		free(context);
		return 0;
	}

	static int fbitstr_getc(void *stream)
	{
		FBITSTR *context = (FBITSTR*)stream;
		if(!context || !context->fp) return EOF;
		return fgetc(context->fp);
	}

	static int fbitstr_putc(int c, void *stream)
	{
		FBITSTR *context = (FBITSTR*)stream;
		if(!context || !context->fp) return EOF;
		return fputc(c, context->fp);
	}

	static int fbitstr_seek(void *stream, long offset, int origin)
	{
		FBITSTR *context = (FBITSTR*)stream;
		if(!context || !context->fp) return EOF;
		context->bitbuf = 0;
		context->bitnum = 0;
		return fseek(context->fp, offset, origin);
	}

	static long fbitstr_tell(void *stream)
	{
		FBITSTR *context = (FBITSTR*)stream;
		if(!context || !context->fp) return EOF;
		return ftell(context->fp);
	}

	static int fbitstr_flush(void *stream)
	{
    	FBITSTR *context = (FBITSTR*)stream;
   		if (!context || !context->fp) return EOF;

    	if (context->bitnum != 0) {
        	if (EOF == fputc(context->bitbuf & 0xff, context->fp))
            	return EOF;
        	context->bitbuf = 0;
        	context->bitnum = 0;
    	}
    	return fflush(context->fp);
	}

	/*
	memory bit str
	*/
	static void* mbitstr_open(void *buf, int len)
	{
		MBITSTR *context = calloc(1, sizeof(MBITSTR)); // 分配空间
		if(!context) return NULL;
		context->type = BITSTR_MEM;
		context->membuf = buf;
    	context->memlen = len;
    	return context;
	}

	static int mbitstr_close(void *stream)
	{
		MBITSTR *context = (MBITSTR*)stream;
    	if (!context) return EOF;
    	free(context);
    	return 0;
	}

	static int mbitstr_getc(void *stream)
	{
    	MBITSTR *context = (MBITSTR*)stream;
    	if (!context || context->curpos >= context->memlen) return EOF;
    	return context->membuf[context->curpos++];
	}

	static int mbitstr_putc(int c, void *stream)
	{
    	MBITSTR *context = (MBITSTR*)stream;
    	if (!context || context->curpos >= context->memlen) return EOF;
    	return (context->membuf[context->curpos++] = c);
	}

	static int mbitstr_seek(void *stream, long offset, int origin)
	{
		MBITSTR *context = (MBITSTR*)stream;
    	int newpos  = 0;
    	if (!context) return EOF;
		// 根据origin计算offset的位置
    	switch (origin) {
    	case SEEK_SET: newpos = offset; break;
    	case SEEK_CUR: newpos = context->curpos + offset; break;
    	case SEEK_END: newpos = context->memlen + offset; break;
    	}
    	if (newpos < 0 || newpos > context->memlen) return EOF;
    	context->curpos = newpos;
    	context->bitbuf = 0;
    	context->bitnum = 0;
    	return 0;
	}

	static long mbitstr_tell(void *stream)
	{
    	MBITSTR *context = (MBITSTR*)stream;
    	if (!context) return EOF;
    	return context->curpos > context->memlen ? EOF : context->curpos;
	}

	static int mbitstr_flush(void *stream) 
	{ 
		return stream ? 0 : EOF; 
	}

	/*
	bit str
	*/
	void* bitstr_open(void *fnamebuf, char *fmode, int bufsize)
	{
		// 选择从文件中读取还是从内存中读取
		if(strcmp(fmode, "mem") == 0)
			return mbitstr_open((void*)fnamebuf, bufsize);
		else
			return fbitstr_open((char*)fnamebuf, fmode);
	}
	// 根据bitstr类型关闭字节流
	int bitstr_close(void *stream)
	{
		int type = *(int*)stream;
		switch(type){
			case BITSTR_MEM: return mbitstr_close(stream);
			case BITSTR_FILE: return fbitstr_close(stream);
		}
		return EOF;
	}

	int bitstr_getc(void *stream)
	{
		int type = *(int*)stream;
		switch(type){
			case BITSTR_MEM: return mbitstr_getc(stream);
			case BITSTR_FILE: return fbitstr_getc(stream);
		}
		return EOF;
	}

	int bitstr_putc(int c, void *stream)
	{
		int type = *(int*)stream;
		switch(type){
			case BITSTR_MEM: return mbitstr_putc(c, stream);
			case BITSTR_FILE: return fbitstr_putc(c, stream);
		}
		return EOF;
	}

	int bitstr_seek(void *stream, long offset, int origin)
	{
    	int type = *(int*)stream;
    	switch (type) {
    	case BITSTR_MEM : return mbitstr_seek(stream, offset, origin);
    	case BITSTR_FILE: return fbitstr_seek(stream, offset, origin);
    	}
    	return EOF;
	}

	long bitstr_tell(void *stream)
	{
    	int type = *(int*)stream;
    	if (!stream) return EOF;
    	switch (type) {
    	case BITSTR_MEM : return mbitstr_tell(stream);
    	case BITSTR_FILE: return fbitstr_tell(stream);
    	}
    	return EOF;
	}

	int bitstr_getb(void *stream)
	{
    	int bit, flag = 0;
    	FBITSTR *context = (FBITSTR*)stream;
    	if (!context) return EOF;

    		if (context->bitnum == 0) {
	#if USE_JPEG_BITSTR
        	do {
            	context->bitbuf = bitstr_getc(stream);
            	if (context->bitbuf == 0xff) flag = 1;
        	} while (context->bitbuf != EOF && context->bitbuf == 0xff);
        	if (flag && context->bitbuf == 0) context->bitbuf = 0xff;
	#else
        	context->bitbuf = bitstr_getc(stream);
	#endif
        	context->bitnum = 8;
        	if (context->bitbuf == EOF) {
            	return EOF;
        	}
    	}
		// 从bitbuf的最高位取一个bit
    	bit = (context->bitbuf >> 7) & (1 << 0);
    	context->bitbuf <<= 1;
    	context->bitnum--;
    	return bit;
	}

	int bitstr_putb(int b, void *stream)
	{
    	FBITSTR *context = (FBITSTR*)stream;
    	if (!context) return EOF;
		// bitbuf左移一位,b填到bitbuf的最低位
    	context->bitbuf <<= 1;
    	context->bitbuf  |= b;
    	context->bitnum++;

    	if (context->bitnum == 8) {
        	if (bitstr_putc(context->bitbuf & 0xff, stream) == EOF) {
            	return EOF;
        	}

	#if USE_JPEG_BITSTR
			// Jpeg文件格式以0xff开头
        	if (context->bitbuf == 0xff) {
            	if (EOF == bitstr_putc(0x00, stream)) return EOF;
        	}
	#endif
        	context->bitbuf = 0;
        	context->bitnum = 0;
    	}
    	return b;
	}
	// n次循环 取到的bit从高到底排列到buf中
	int bitstr_get_bits(void *stream, int n)
	{
    	int buf = 0;
    	while (n--) {
        	buf <<= 1;
        	buf  |= bitstr_getb(stream);
    	}
    	return buf;
	}

	int bitstr_put_bits(void *stream, int bits, int n)
	{
    	unsigned buf = bits << (32 - n);
    	while (n--) {
        	if (EOF == bitstr_putb(buf >> 31, stream)) {
            	return EOF;
        	}
        	buf <<= 1;
    	}
    	return bits;
	}

	int bitstr_flush(void *stream, int flag)
	{
    	FBITSTR *context = (FBITSTR*)stream;
    	if (!context) return EOF;

    	// output
    	bitstr_put_bits(stream, flag ? -1 : 0, context->bitnum ? 8 - context->bitnum : 0);

    	// flush
    	switch (context->type) {
    	case BITSTR_MEM : return mbitstr_flush(stream);
    	case BITSTR_FILE: return fbitstr_flush(stream);
    	}
    	return EOF;
	}
}