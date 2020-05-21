#include"b2j.h"
//��������
int ALIGN_b(int x, int y) {//����16�ı���
	// y must be a power of 2.
	return (x + y - 1) & ~(y - 1);
}

//z���б���
void zigzag_encode(int* data)
{
	int buf[64], i;
	for (i = 0; i < 64; i++) buf[i] = data[ZIGZAG[i]];
	for (i = 0; i < 64; i++) data[i] = buf[i];
}
void zigzag_decode(int* data)
{
	int buf[64], i;
	for (i = 0; i < 64; i++) buf[ZIGZAG[i]] = data[i];
	for (i = 0; i < 64; i++) data[i] = buf[i];
}

//��ֱ��������
void category_encode(int* code, int* size)
{
	unsigned absc = abs(*code);
	unsigned mask = (1 << 15);
	int i = 15;
	if (absc == 0) { *size = 0; return; }
	while (i && !(absc & mask)) { mask >>= 1; i--; }//����ż�size
	*size = i + 1;
	if (*code < 0) *code = (1 << *size) - absc - 1;
}
int category_decode(int code, int  size)
{
	return code >= (1 << (size - 1)) ? code : code - (1 << size) + 1;
}


typedef struct {
	unsigned runlen : 4;
	unsigned codesize : 4;
	unsigned codedata : 16;
} RLEITEM;
//���������
void B2J::jfif_encode_du(JFIF* jfif, int type, int du[64], int* dc)
{
	HUFCODEC* hfcac = jfif->phcac[type];//AC����
	HUFCODEC* hfcdc = jfif->phcdc[type];//DC����
	int* pqtab = jfif->pqtab[type];//����������
	void* bs = hfcac->output;//�����
	int       diff, code, size;
	RLEITEM   rlelist[63];
	int       i, j, n, eob;

	// fdct����1
	dct.fdct2d8x8(du, NULL);//F(u,v)

	//����2
	quant.quant_encode(du, pqtab);//~F(u,v)

	// zigzagɨ��
	zigzag_encode(du);

	// dc����ͱ���
	diff = du[0] - *dc;
	*dc = du[0];

	//dc��ֱ���
	code = diff;
	category_encode(&code, &size);

	// ���ݵõ��ĳ��ȣ�������д��һ��
	huffman.huffman_encode_step(hfcdc, size);
	bitStream.bitstr_put_bits(bs, code, size);

	//ac�ı���
	for (i = 1, j = 0, n = 0, eob = 0; i < 64 && j < 63; i++) {
		if (du[i] == 0 && n < 15) {
			n++;
		}
		else {
			code = du[i]; size = 0;
			category_encode(&code, &size);
			rlelist[j].runlen = n;
			rlelist[j].codesize = size;
			rlelist[j].codedata = code;
			n = 0;
			j++;
			if (size != 0) eob = j;
		}
	}

	// set eob
	if (du[63] == 0) {
		rlelist[eob].runlen = 0;
		rlelist[eob].codesize = 0;
		rlelist[eob].codedata = 0;
		j = eob + 1;
	}

	// huffman encode for ac
	for (i = 0; i < j; i++) {
		huffman.huffman_encode_step(hfcac, (rlelist[i].runlen << 4) | (rlelist[i].codesize << 0));
		bitStream.bitstr_put_bits(bs, rlelist[i].codedata, rlelist[i].codesize);
	}
}

//�������
int B2J::huffman_decode_step(HUFCODEC* phc)
{
	int bit;
	int code = 0;
	int len = 0;
	int idx = 0;

	/* ��������������������Ч�� */
	if (!phc || !phc->input) return EOF;

	/* ����������ȡ���� */
	while (1) {
		bit = bitStream.bitstr_getb(phc->input);
		if (bit == EOF) return EOF;
		//      printf("%d, first = %d, len = %d\n", bit ? 1 : 0, phc->first[len], len);
		code <<= 1; code |= bit;
		if (code - phc->first[len] < phc->huftab[len]) break;
		if (++len == MAX_HUFFMAN_CODE_LEN) return EOF;
	}

	idx = phc->index[len] + (code - phc->first[len]);
	//  printf("get code:%c len:%d, idx:%d\n\n", phc->huftab[idx], len, idx);
	return idx < MAX_HUFFMAN_CODE_LEN + 256 ? phc->huftab[idx] : EOF;
}

//�������
void* B2J::jfif_encode(BMP* pb) {

	//�ֲ�����
	JFIF* jfif = NULL;//�ṹ�彨��
	void* bs = NULL;//output��
	int   jw, jh;//��,��(����)
	int* yuv_datbuf[3] = { 0 };//yuv����
	int* ydst, * udst, * vdst;//��
	int* isrc, * idst;
	BYTE* bsrc;
	int   du[64] = { 0 };//8*8�����ؾ���
	int   dc[4] = { 0 };//��¼DC�Ŀ�ʼϵ��
	int   i, j, m, n;
	int   failed = 1;//ʧ�ܱ�־
	char temp[3] = { 'm','e','m' };


	// �������
	if (!pb) {
		printf("invalid input params !\n");
		return NULL;
	}

	// allocate jfif context
	jfif = (JFIF*)calloc(1, sizeof(JFIF));
	if (!jfif) return NULL;

	// init dct module
	dct.init_dct_module();

	//�ռ����
	jfif->width = pb->width;
	jfif->height = pb->height;
	jfif->pqtab[0] = (int*)malloc(64 * sizeof(int));//������1
	jfif->pqtab[1] = (int*)malloc(64 * sizeof(int));//������2
	jfif->phcac[0] = (HUFCODEC*)calloc(1, sizeof(HUFCODEC));//����AC
	jfif->phcac[1] = (HUFCODEC*)calloc(1, sizeof(HUFCODEC));//ɫ��AC
	jfif->phcdc[0] = (HUFCODEC*)calloc(1, sizeof(HUFCODEC));//����DC
	jfif->phcdc[1] = (HUFCODEC*)calloc(1, sizeof(HUFCODEC));//ɫ��DC
	jfif->datalen = jfif->width * jfif->height * 2;
	jfif->databuf = (BYTE*)malloc(jfif->datalen);
	if (!jfif->pqtab[0] || !jfif->pqtab[1]
		|| !jfif->phcac[0] || !jfif->phcac[1]
		|| !jfif->phcdc[0] || !jfif->phcdc[1]
		|| !jfif->databuf) {
		goto done;
	}

	// init qtab��ֱ�Ӿ�����Ԥ����õ�������ɫ�Ⱥ����ȵ�
	memcpy(jfif->pqtab[0], STD_QUANT_TAB_LUMIN, 64 * sizeof(int));
	memcpy(jfif->pqtab[1], STD_QUANT_TAB_CHROM, 64 * sizeof(int));

	// open bit stream
	bs = bitStream.bitstr_open(jfif->databuf, temp, jfif->datalen);
	if (!bs) {
		printf("failed to open bitstr for jfif_decode !");
		goto done;
	}

	//�����������ʼ��д��
	memcpy(jfif->phcac[0]->huftab, STD_HUFTAB_LUMIN_AC, MAX_HUFFMAN_CODE_LEN + 256);
	memcpy(jfif->phcac[1]->huftab, STD_HUFTAB_CHROM_AC, MAX_HUFFMAN_CODE_LEN + 256);
	memcpy(jfif->phcdc[0]->huftab, STD_HUFTAB_LUMIN_DC, MAX_HUFFMAN_CODE_LEN + 256);
	memcpy(jfif->phcdc[1]->huftab, STD_HUFTAB_CHROM_DC, MAX_HUFFMAN_CODE_LEN + 256);
	jfif->phcac[0]->output = bs; huffman.huffman_encode_init(jfif->phcac[0], 1);
	jfif->phcac[1]->output = bs; huffman.huffman_encode_init(jfif->phcac[1], 1);
	jfif->phcdc[0]->output = bs; huffman.huffman_encode_init(jfif->phcdc[0], 1);
	jfif->phcdc[1]->output = bs; huffman.huffman_encode_init(jfif->phcdc[1], 1);

	//ͷ��Ϣд��
	jfif->comp_num = 3;
	jfif->comp_info[0].id = 1;
	jfif->comp_info[0].samp_factor_v = 2;
	jfif->comp_info[0].samp_factor_h = 2;
	jfif->comp_info[0].qtab_idx = 0;
	jfif->comp_info[0].htab_idx_ac = 0;
	jfif->comp_info[0].htab_idx_dc = 0;
	jfif->comp_info[1].id = 2;
	jfif->comp_info[1].samp_factor_v = 1;
	jfif->comp_info[1].samp_factor_h = 1;
	jfif->comp_info[1].qtab_idx = 1;
	jfif->comp_info[1].htab_idx_ac = 1;
	jfif->comp_info[1].htab_idx_dc = 1;
	jfif->comp_info[2].id = 3;
	jfif->comp_info[2].samp_factor_v = 1;
	jfif->comp_info[2].samp_factor_h = 1;
	jfif->comp_info[2].qtab_idx = 1;
	jfif->comp_info[2].htab_idx_ac = 1;
	jfif->comp_info[2].htab_idx_dc = 1;

	// init jw & jw, init yuv data buffer
	jw = ALIGN_b(pb->width, 16);//����16������100�����أ���Ϊ112��
	jh = ALIGN_b(pb->height, 16);
	yuv_datbuf[0] = (int*)calloc(1, jw * jh / 1 * sizeof(int));
	yuv_datbuf[1] = (int*)calloc(1, jw * jh / 4 * sizeof(int));
	yuv_datbuf[2] = (int*)calloc(1, jw * jh / 4 * sizeof(int));
	if (!yuv_datbuf[0] || !yuv_datbuf[1] || !yuv_datbuf[2]) {
		goto done;
	}

	// ����rgb���ݷֱ����yuv���ݲ�д����������:4:2:0
	bsrc = (BYTE*)pb->pdata;//ԭʼ���ݣ�R0G0B0,R1G1B1...����ת��Ϊ��������yuv
	ydst = yuv_datbuf[0];
	udst = yuv_datbuf[1];
	vdst = yuv_datbuf[2];
	for (i = 0; i < pb->height; i++) {
		for (j = 0; j < pb->width; j++) {
			color.rgb_to_yuv(bsrc[2], bsrc[1], bsrc[0], ydst, udst, vdst);
			bsrc += 3;
			ydst += 1;
			if (j & 1) {
				udst += 1;
				vdst += 1;
			}
		}
		bsrc -= pb->width * 3; bsrc += pb->stride;
		ydst -= pb->width * 1; ydst += jw;
		udst -= pb->width / 2;
		vdst -= pb->width / 2;
		if (i & 1) {
			udst += jw / 2;
			vdst += jw / 2;
		}
	}

	//�ֿ鿪ʼ����
	for (m = 0; m < jh / 16; m++) {
		for (n = 0; n < jw / 16; n++) {
			//++ encode mcu, yuv 4:2:0
			//+ y du0���Ͽ�
			isrc = yuv_datbuf[0] + (m * 16 + 0) * jw + n * 16 + 0;//ÿһ��ÿ�ζ�16��
			idst = du;//int 64����
			for (i = 0; i < 8; i++) {
				memcpy(idst, isrc, 8 * sizeof(int));//һ��ȡ8��
				isrc += jw; idst += 8;//8��
			}
			jfif_encode_du(jfif, DU_TYPE_LUMIN, du, &(dc[0]));
			//- y du0

			//+ y du1���Ͽ�
			isrc = yuv_datbuf[0] + (m * 16 + 0) * jw + n * 16 + 8;
			idst = du;
			for (i = 0; i < 8; i++) {
				memcpy(idst, isrc, 8 * sizeof(int));
				isrc += jw; idst += 8;
			}
			jfif_encode_du(jfif, DU_TYPE_LUMIN, du, &(dc[0]));
			//- y du1

			//+ y du2���¿�
			isrc = yuv_datbuf[0] + (m * 16 + 8) * jw + n * 16 + 0;
			idst = du;
			for (i = 0; i < 8; i++) {
				memcpy(idst, isrc, 8 * sizeof(int));
				isrc += jw; idst += 8;
			}
			jfif_encode_du(jfif, DU_TYPE_LUMIN, du, &(dc[0]));
			//- y du2

			//+ y du3���¿�
			isrc = yuv_datbuf[0] + (m * 16 + 8) * jw + n * 16 + 8;
			idst = du;
			for (i = 0; i < 8; i++) {
				memcpy(idst, isrc, 8 * sizeof(int));
				isrc += jw; idst += 8;
			}
			jfif_encode_du(jfif, DU_TYPE_LUMIN, du, &(dc[0]));
			//- y du3

			//+ u du
			isrc = yuv_datbuf[1] + m * 8 * (jw / 2) + n * 8;
			idst = du;
			for (i = 0; i < 8; i++) {
				memcpy(idst, isrc, 8 * sizeof(int));
				isrc += jw / 2; idst += 8;
			}
			jfif_encode_du(jfif, DU_TYPE_CHROM, du, &(dc[1]));
			//- u du

			//+ v du
			isrc = yuv_datbuf[2] + m * 8 * (jw / 2) + n * 8;
			idst = du;
			for (i = 0; i < 8; i++) {
				memcpy(idst, isrc, 8 * sizeof(int));
				isrc += jw / 2; idst += 8;
			}
			jfif_encode_du(jfif, DU_TYPE_CHROM, du, &(dc[2]));
			//- v du
			//-- encode mcu, yuv 4:2:0
		}
	}
	failed = 0;

done:
	// free yuv data buffer
	if (yuv_datbuf[0]) free(yuv_datbuf[0]);
	if (yuv_datbuf[1]) free(yuv_datbuf[1]);
	if (yuv_datbuf[2]) free(yuv_datbuf[2]);

	// close huffman codec
	huffman.huffman_encode_done(jfif->phcac[0]);
	huffman.huffman_encode_done(jfif->phcac[1]);
	huffman.huffman_encode_done(jfif->phcdc[0]);
	huffman.huffman_encode_done(jfif->phcdc[1]);
	jfif->datalen = bitStream.bitstr_tell(bs);

	// close bit stream
	bitStream.bitstr_close(bs);

	// if failed free context
	if (failed) {
		jfif_free(jfif);
		jfif = NULL;
	}

	// return context
	return jfif;
}

//��ȡjpg�ļ���Ϣ
void* B2J::jfif_load(char* file)
{
	JFIF* jfif = NULL;
	FILE* fp = NULL;
	int   header = 0;
	int   type = 0;
	WORD  size = 0;
	BYTE* buf = NULL;
	BYTE* end = NULL;
	BYTE* dqt, * dht;
	int   ret = -1;
	long  offset = 0;
	int   i;

	jfif = (JFIF*)calloc(1, sizeof(JFIF));//ͷ
	buf = (BYTE*)calloc(1, 0x10000);//������
	end = buf + 0x10000;//ĩβ��
	if (!jfif || !buf) goto done;

	fp = fopen(file, "rb");
	if (!fp) goto done;

	while (1) {
		do { header = fgetc(fp); } while (header != EOF && header != 0xff); // get header������EOF�ж��ļ���β����0xff�жϱ�־
		do { type = fgetc(fp); } while (type != EOF && type == 0xff); // get type������0xff֮�������ͱ�־
		if (header == EOF || type == EOF) {
			printf("file eof !\n");
			break;
		}

		if ((type == 0xd8) || (type == 0xd9) || (type == 0x01) || (type >= 0xd0 && type <= 0xd7)) {
			size = 0;//��ʼ��־��������־��������ֵ��RSTn��־
		}
		else {
			size = fgetc(fp) << 8;//��8λ
			size |= fgetc(fp) << 0;//�Ͱ�λ
			size -= 2;//��ȥ���ֶγ���
		}

		size = fread(buf, 1, size, fp);//����һ��
		switch (type) {
		case 0xc0: // SOF0
			jfif->width = (buf[3] << 8) | (buf[4] << 0);
			jfif->height = (buf[1] << 8) | (buf[2] << 0);
			jfif->comp_num = buf[5] < 4 ? buf[5] : 4;
			for (i = 0; i < jfif->comp_num; i++) {
				jfif->comp_info[i].id = buf[6 + i * 3];
				jfif->comp_info[i].samp_factor_v = (buf[7 + i * 3] >> 0) & 0x0f;
				jfif->comp_info[i].samp_factor_h = (buf[7 + i * 3] >> 4) & 0x0f;
				jfif->comp_info[i].qtab_idx = buf[8 + i * 3] & 0x0f;
			}
			break;

		case 0xda: // SOS
			jfif->comp_num = buf[0] < 4 ? buf[0] : 4;
			for (i = 0; i < jfif->comp_num; i++) {
				jfif->comp_info[i].id = buf[1 + i * 2];
				jfif->comp_info[i].htab_idx_ac = (buf[2 + i * 2] >> 0) & 0x0f;
				jfif->comp_info[i].htab_idx_dc = (buf[2 + i * 2] >> 4) & 0x0f;
			}
			offset = ftell(fp);
			ret = 0;
			goto read_data;

		case 0xdb: // DQT
			dqt = buf;
			while (size > 0 && dqt < end) {
				int idx = dqt[0] & 0x0f;
				int f16 = dqt[0] & 0xf0;
				if (!jfif->pqtab[idx]) jfif->pqtab[idx] = (int*)malloc(64 * sizeof(int));
				if (!jfif->pqtab[idx]) break;
				if (dqt + 1 + 64 + (f16 ? 64 : 0) < end) {
					for (i = 0; i < 64; i++) {
						jfif->pqtab[idx][ZIGZAG[i]] = f16 ? ((dqt[1 + i * 2] << 8) | (dqt[2 + i * 2] << 0)) : dqt[1 + i];
					}
				}
				dqt += 1 + 64 + (f16 ? 64 : 0);
				size -= 1 + 64 + (f16 ? 64 : 0);
			}
			break;

		case 0xc4: // DHT
			dht = buf;
			while (size > 0 && dht + 17 < end) {
				int idx = dht[0] & 0x0f;
				int fac = dht[0] & 0xf0;
				int len = 0;
				for (i = 1; i < 1 + 16; i++) len += dht[i];
				if (len > end - dht - 17) len = end - dht - 17;
				if (len > 256) len = 256;
				if (fac) {
					if (!jfif->phcac[idx]) jfif->phcac[idx] = (HUFCODEC*)calloc(1, sizeof(HUFCODEC));
					if (jfif->phcac[idx]) memcpy(jfif->phcac[idx]->huftab, &dht[1], 16 + len);
				}
				else {
					if (!jfif->phcdc[idx]) jfif->phcdc[idx] = (HUFCODEC*)calloc(1, sizeof(HUFCODEC));
					if (jfif->phcdc[idx]) memcpy(jfif->phcdc[idx]->huftab, &dht[1], 16 + len);
				}
				dht += 17 + len;
				size -= 17 + len;
			}
			break;
		}
	}

read_data:
	fseek(fp, 0, SEEK_END);//�ļ�β
	jfif->datalen = ftell(fp) - offset;//offset������Ϊ���ݿ�ʼλ�ã�SOS
	jfif->databuf = (BYTE*)malloc(jfif->datalen);
	if (jfif->databuf) {//��������
		fseek(fp, offset, SEEK_SET);
		fread(jfif->databuf, 1, jfif->datalen, fp);
	}

done:
	if (buf) free(buf);
	if (fp) fclose(fp);
	if (ret == -1) {
		jfif_free(jfif);
		jfif = NULL;
	}
	return jfif;//��������
}

//����
int B2J::jfif_decode(void* ctxt, BMP* pb)
{
	JFIF* jfif = (JFIF*)ctxt;//���ݵ�������
	void* bs = NULL;
	int* ftab[16] = { 0 };//������
	int   dc[4] = { 0 };
	int   mcuw, mcuh, mcuc, mcur, mcui, jw, jh;//����
	int   i, j, c, h, v, x, y;
	int   sfh_max = 0;//�����������ֵ
	int   sfv_max = 0;
	int   yuv_stride[3] = { 0 };//yuv������
	int   yuv_height[3] = { 0 };
	int* yuv_datbuf[3] = { 0 };
	int* idst, * isrc;
	int* ysrc, * usrc, * vsrc;
	BYTE* bdst;
	int   ret = -1;
	char temp[3] = { 'm','e','m' };

	if (!ctxt || !pb) {
		printf("invalid input params !\n");
		return -1;
	}

	//dct��ʼ��
	dct.init_dct_module();

	//++Ϊ���������ռ�ftab������ʼ��
	for (i = 0; i < 16; i++) {
		if (jfif->pqtab[i]) {
			ftab[i] = (int*)malloc(64 * sizeof(int));
			if (ftab[i]) {
				dct.init_idct_ftab(ftab[i], jfif->pqtab[i]);//dct����
			}
			else {
				goto done;
			}
		}
	}
	//-- init ftab

	//++ calculate mcu info�����������
	for (c = 0; c < jfif->comp_num; c++) {
		if (sfh_max < jfif->comp_info[c].samp_factor_h) {
			sfh_max = jfif->comp_info[c].samp_factor_h;
		}
		if (sfv_max < jfif->comp_info[c].samp_factor_v) {
			sfv_max = jfif->comp_info[c].samp_factor_v;
		}
	}
	if (!sfh_max) sfh_max = 1;
	if (!sfv_max) sfv_max = 1;
	mcuw = sfh_max * 8;//�������ӣ�ˮƽ��16
	mcuh = sfv_max * 8;//�������ӣ���ֱ��16
	jw = ALIGN_b(jfif->width, mcuw);
	jh = ALIGN_b(jfif->height, mcuh);
	mcuc = jw / mcuw;
	mcur = jh / mcuh;
	//-- calculate mcu info

	// create yuv buffer for decoding
	yuv_stride[0] = jw;
	yuv_stride[1] = jw * jfif->comp_info[1].samp_factor_h / sfh_max;
	yuv_stride[2] = jw * jfif->comp_info[2].samp_factor_h / sfh_max;
	yuv_height[0] = jh;
	yuv_height[1] = jh * jfif->comp_info[1].samp_factor_v / sfv_max;
	yuv_height[2] = jh * jfif->comp_info[2].samp_factor_v / sfv_max;
	yuv_datbuf[0] = (int*)malloc(yuv_stride[0] * yuv_height[0] * sizeof(int));
	yuv_datbuf[1] = (int*)malloc(yuv_stride[1] * yuv_height[1] * sizeof(int));
	yuv_datbuf[2] = (int*)malloc(yuv_stride[2] * yuv_height[2] * sizeof(int));
	if (!yuv_datbuf[0] || !yuv_datbuf[1] || !yuv_datbuf[2]) {
		goto done;
	}

	// open bit stream
	bs = bitStream.bitstr_open(jfif->databuf, temp, jfif->datalen);
	if (!bs) {
		printf("failed to open bitstr for jfif_decode !");
		return -1;
	}

	// �ҹ�����ʹ�õĲ��ұ�
	for (i = 0; i < 16; i++) {
		if (jfif->phcac[i]) {
			jfif->phcac[i]->input = bs;
			huffman.huffman_decode_init(jfif->phcac[i]);
		}
		if (jfif->phcdc[i]) {
			jfif->phcdc[i]->input = bs;
			huffman.huffman_decode_init(jfif->phcdc[i]);
		}
	}

	//ÿ�����������
	for (mcui = 0; mcui < mcuc * mcur; mcui++) {
		for (c = 0; c < jfif->comp_num; c++) {//y,u,v
			for (v = 0; v < jfif->comp_info[c].samp_factor_v; v++) {
				for (h = 0; h < jfif->comp_info[c].samp_factor_h; h++) {
					HUFCODEC* hcac = jfif->phcac[jfif->comp_info[c].htab_idx_ac];
					HUFCODEC* hcdc = jfif->phcdc[jfif->comp_info[c].htab_idx_dc];
					int       fidx = jfif->comp_info[c].qtab_idx;
					int size, znum, code;
					int du[64] = { 0 };

					//+ decode dc
					size = huffman_decode_step(hcdc) & 0xf;
					if (size) {
						code = bitStream.bitstr_get_bits(bs, size);
						code = category_decode(code, size);//�������
					}
					else {
						code = 0;
					}
					dc[c] += code;
					du[0] = dc[c];
					//- decode dc

					//+ decode ac
					for (i = 1; i < 64; ) {
						code = huffman_decode_step(hcac);
						if (code <= 0) break;
						size = (code >> 0) & 0xf;
						znum = (code >> 4) & 0xf;
						i += znum;
						code = bitStream.bitstr_get_bits(bs, size);
						code = category_decode(code, size);
						if (i < 64) du[i++] = code;
					}
					//- decode ac

					// de-zigzag
					zigzag_decode(du);

					// idct
					dct.idct2d8x8(du, ftab[fidx]);

					// copy du to yuv buffer
					x = ((mcui % mcuc) * mcuw + h * 8) * jfif->comp_info[c].samp_factor_h / sfh_max;
					y = ((mcui / mcuc) * mcuh + v * 8) * jfif->comp_info[c].samp_factor_v / sfv_max;
					idst = yuv_datbuf[c] + y * yuv_stride[c] + x;
					isrc = du;
					for (i = 0; i < 8; i++) {
						memcpy(idst, isrc, 8 * sizeof(int));
						idst += yuv_stride[c];
						isrc += 8;
					}
				}
			}
		}
	}

	// close huffman codec
	for (i = 0; i < 16; i++) {
		if (jfif->phcac[i]) huffman.huffman_decode_done(jfif->phcac[i]);
		if (jfif->phcdc[i]) huffman.huffman_decode_done(jfif->phcdc[i]);
	}

	// close bit stream
	bitStream.bitstr_close(bs);

	// create bitmap, and convert yuv to rgb
	bmp.bmp_create(pb, jfif->width, jfif->height);
	bdst = (BYTE*)pb->pdata;
	ysrc = yuv_datbuf[0];
	for (i = 0; i < jfif->height; i++) {
		int uy = i * jfif->comp_info[1].samp_factor_v / sfv_max;
		int vy = i * jfif->comp_info[2].samp_factor_v / sfv_max;
		for (j = 0; j < jfif->width; j++) {
			int ux = j * jfif->comp_info[1].samp_factor_h / sfh_max;
			int vx = j * jfif->comp_info[2].samp_factor_h / sfh_max;
			usrc = yuv_datbuf[2] + uy * yuv_stride[2] + ux;
			vsrc = yuv_datbuf[1] + vy * yuv_stride[1] + vx;
			color.yuv_to_rgb(*ysrc, *vsrc, *usrc, bdst + 2, bdst + 1, bdst + 0);
			bdst += 3;
			ysrc += 1;
		}
		bdst -= jfif->width * 3;
		bdst += pb->stride;
		ysrc -= jfif->width * 1;
		ysrc += yuv_stride[0];
	}

	// success
	ret = 0;

done:
	if (yuv_datbuf[0]) free(yuv_datbuf[0]);
	if (yuv_datbuf[1]) free(yuv_datbuf[1]);
	if (yuv_datbuf[2]) free(yuv_datbuf[2]);
	//++ free ftab
	for (i = 0; i < 16; i++) {
		if (ftab[i]) {
			free(ftab[i]);
		}
	}
	//-- free ftab
	return ret;
}

//�ռ��ͷ�
void B2J::jfif_free(void* ctxt)
{
	JFIF* jfif = (JFIF*)ctxt;
	int   i;
	if (!jfif) return;
	for (i = 0; i < 16; i++) {
		if (jfif->pqtab[i]) free(jfif->pqtab[i]);
		if (jfif->phcac[i]) free(jfif->phcac[i]);
		if (jfif->phcdc[i]) free(jfif->phcdc[i]);
	}
	if (jfif->databuf) free(jfif->databuf);
	free(jfif);
}

//������д��jpg�ļ�
int B2J::jfif_save(void* ctxt, char* file)
{
	JFIF* jfif = (JFIF*)ctxt;
	FILE* fp = NULL;
	int   len = 0;
	int   i, j;
	int   ret = -1;

	//��д���ļ���
	fp = fopen(file, "wb");
	if (!fp) goto done;

	// output SOI��ʼ���
	fputc(0xff, fp);
	fputc(0xd8, fp);

	// output DQTд��������
	for (i = 0; i < 16; i++) {
		if (!jfif->pqtab[i]) continue;
		len = 2 + 1 + 64;//����
		fputc(0xff, fp);
		fputc(0xdb, fp);
		fputc(len >> 8, fp);//����0��8λ
		fputc(len >> 0, fp);//���ȣ�64+2+1
		fputc(i, fp);//ID
		for (j = 0; j < 64; j++) {
			fputc(jfif->pqtab[i][ZIGZAG[j]], fp);
		}
	}

	// output SOF0ͼ������
	len = 2 + 1 + 2 + 2 + 1 + 3 * jfif->comp_num;
	fputc(0xff, fp);
	fputc(0xc0, fp);
	fputc(len >> 8, fp);
	fputc(len >> 0, fp);
	fputc(8, fp); // ���� 8bit
	fputc(jfif->height >> 8, fp); // height
	fputc(jfif->height >> 0, fp); // height
	fputc(jfif->width >> 8, fp); // width
	fputc(jfif->width >> 0, fp); // width
	fputc(jfif->comp_num, fp);
	for (i = 0; i < jfif->comp_num; i++) {
		fputc(jfif->comp_info[i].id, fp);
		fputc((jfif->comp_info[i].samp_factor_v << 0) | (jfif->comp_info[i].samp_factor_h << 4), fp);
		fputc(jfif->comp_info[i].qtab_idx, fp);
	}

	// output DHT AC��������
	for (i = 0; i < 16; i++) {
		if (!jfif->phcac[i]) continue;
		fputc(0xff, fp);
		fputc(0xc4, fp);
		len = 2 + 1 + 16;
		for (j = 0; j < 16; j++) len += jfif->phcac[i]->huftab[j];
		fputc(len >> 8, fp);
		fputc(len >> 0, fp);
		fputc(i + 0x10, fp);
		fwrite(jfif->phcac[i]->huftab, len - 3, 1, fp);
	}

	// output DHT DC��������
	for (i = 0; i < 16; i++) {
		if (!jfif->phcdc[i]) continue;
		fputc(0xff, fp);
		fputc(0xc4, fp);
		len = 2 + 1 + 16;
		for (j = 0; j < 16; j++) len += jfif->phcdc[i]->huftab[j];
		fputc(len >> 8, fp);
		fputc(len >> 0, fp);
		fputc(i + 0x00, fp);
		fwrite(jfif->phcdc[i]->huftab, len - 3, 1, fp);
	}

	// output SOSɨ�迪ʼ12�ֽ�
	len = 2 + 1 + 2 * jfif->comp_num + 3;
	fputc(0xff, fp);
	fputc(0xda, fp);
	fputc(len >> 8, fp);
	fputc(len >> 0, fp);//����
	fputc(jfif->comp_num, fp);//������
	for (i = 0; i < jfif->comp_num; i++) {
		fputc(jfif->comp_info[i].id, fp);
		fputc((jfif->comp_info[i].htab_idx_ac << 0) | (jfif->comp_info[i].htab_idx_dc << 4), fp);
	}
	fputc(0x00, fp);
	fputc(0x00, fp);
	fputc(0x00, fp);

	// output dataд������
	if (jfif->databuf) {
		fwrite(jfif->databuf, jfif->datalen, 1, fp);
	}
	ret = 0;

done:
	if (fp) fclose(fp);
	return ret;
}