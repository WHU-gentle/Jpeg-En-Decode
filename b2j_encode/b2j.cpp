#include"main.h"

//��������
class B2J {
private:
    //���õ���
    Dct dct; BitStream bitStream; Huffman huffman; Color color; Quant quant;

    int ALIGN(int x, int y) {//����16�ı���
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

    //��ֱ���
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
    void jfif_encode_du(JFIF* jfif, int type, int du[64], int* dc)
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
public:
	void* jfif_encode(BMP* pb) {

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
        char* temp = new char[3];

        // �������
        if (!pb) {
            printf("invalid input params !\n");
            return NULL;
        }

        //�ռ����
        jfif->width = pb->width;
        jfif->height = pb->height;
        jfif->pqtab[0] = (int*)malloc(64 * sizeof(int));//������1
        jfif->pqtab[1] = (int*)malloc(64 * sizeof(int));//������2
        jfif->phcac[0] = calloc(1, sizeof(HUFCODEC));//����AC
        jfif->phcac[1] = calloc(1, sizeof(HUFCODEC));//ɫ��AC
        jfif->phcdc[0] = calloc(1, sizeof(HUFCODEC));//����DC
        jfif->phcdc[1] = calloc(1, sizeof(HUFCODEC));//ɫ��DC
        jfif->datalen = jfif->width * jfif->height * 2;
        jfif->databuf = malloc(jfif->datalen);
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
        strcmp(temp, "mem");
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
        jw = ALIGN(pb->width, 16);//����16������100�����أ���Ϊ112��
        jh = ALIGN(pb->height, 16);
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
                //+ y du0
                isrc = yuv_datbuf[0] + (m * 16 + 0) * jw + n * 16 + 0;//ÿһ��ÿ�ζ�16��
                idst = du;//int 64����
                for (i = 0; i < 8; i++) {
                    memcpy(idst, isrc, 8 * sizeof(int));//һ��ȡ8��
                    isrc += jw; idst += 8;//8��
                }
                jfif_encode_du(jfif, DU_TYPE_LUMIN, du, &(dc[0]));
                //- y du0

                //+ y du1
                isrc = yuv_datbuf[0] + (m * 16 + 0) * jw + n * 16 + 8;
                idst = du;
                for (i = 0; i < 8; i++) {
                    memcpy(idst, isrc, 8 * sizeof(int));
                    isrc += jw; idst += 8;
                }
                jfif_encode_du(jfif, DU_TYPE_LUMIN, du, &(dc[0]));
                //- y du1

                //+ y du2
                isrc = yuv_datbuf[0] + (m * 16 + 8) * jw + n * 16 + 0;
                idst = du;
                for (i = 0; i < 8; i++) {
                    memcpy(idst, isrc, 8 * sizeof(int));
                    isrc += jw; idst += 8;
                }
                jfif_encode_du(jfif, DU_TYPE_LUMIN, du, &(dc[0]));
                //- y du2

                //+ y du3
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

    //�ռ��ͷ�
    void jfif_free(void* ctxt)
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
    int jfif_save(void* ctxt, char* file)
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
};