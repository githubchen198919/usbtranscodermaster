
#include "usbtranscodermaster.h"

FILE *SAMPLE_COMM_UMT_OpenFile(const char *filename, const char *mode)
{
	return fopen(filename, mode);
}

int SAMPLE_COMM_UMT_CloseFile(FILE * stream)
{
	return fclose(stream);
}

size_t SAMPLE_COMM_UMT_WriteFile(const void *ptr, size_t size, FILE *stream)
{
	return fwrite(ptr, size, 1, stream);
}

int SAMPLE_COMM_UMT_FseekFile(FILE *stream, long offset, int fromwhere)
{
	return fseek(stream, offset, fromwhere);
}

int SAMPLE_COMM_UMT_FileExist(const char *path)
{
    return (access(path, F_OK) == 0);
}

size_t SAMPLE_COMM_UMT_SaveStream(unsigned int channel_num, UMT_WORK_SB *umt_work_sb_t, char *buf)
{
	int ret = 0;
	FILE **pFile = umt_work_sb_t->svac2_ctl.svac2File;

	int w = umt_work_sb_t->bulk_ep1in_dec.bulkin_dec.nWidth;
	int h = umt_work_sb_t->bulk_ep1in_dec.bulkin_dec.nHeight;
	int frameType = umt_work_sb_t->bulk_ep1in_dec.bulkin_dec.frameType;
	int decryp_data_length = umt_work_sb_t->bulk_ep1in_dec.bulkin_dec.decryp_data_length;
	int stream_offset = umt_work_sb_t->bulk_ep1in_dec.bulkin_dec.stream_offset;

	if (channel_num >= VENC_MAX_CHN_NUM) {
		UTM_PRT("The maximum number of channels in venc is exceeded!\n");
		return -1;
	}

	if (umt_work_sb_t->svac2_ctl.stream_count[channel_num]++ >= 1000 && frameType == 0) {
		if (pFile[channel_num] != NULL) {
			SAMPLE_COMM_UMT_CloseFile(pFile[channel_num]);
			pFile[channel_num] = NULL;
		}
		umt_work_sb_t->svac2_ctl.stream_count[channel_num] = 0;
		umt_work_sb_t->svac2_ctl.cnt_index[channel_num]++;
	}

	if (pFile[channel_num] == NULL) {
		sprintf((char *)umt_work_sb_t->svac2_ctl.aszFileName[channel_num], "stream%d_chn%d_w%d_h%d.svac2", umt_work_sb_t->svac2_ctl.cnt_index[channel_num], channel_num, w, h);

		//UTM_PRT("frameType = %d, data_length = %d\n", frameType, decryp_data_length);
		UTM_PRT("chn%d, name: %s\n", channel_num, umt_work_sb_t->svac2_ctl.aszFileName[channel_num]);

		if (pFile[channel_num]) {
			SAMPLE_COMM_UMT_CloseFile(pFile[channel_num]);
			pFile[channel_num] = NULL;
		}

		pFile[channel_num] = SAMPLE_COMM_UMT_OpenFile(umt_work_sb_t->svac2_ctl.aszFileName[channel_num], "wb");
		if (!pFile[channel_num]) {
			UTM_PRT("open file[%s] failed!\n", umt_work_sb_t->svac2_ctl.aszFileName[channel_num]);
			return -1;
		}
	}

	if (pFile[channel_num] != NULL) {
		ret = SAMPLE_COMM_UMT_WriteFile(buf + stream_offset, decryp_data_length, pFile[channel_num]);
		if (ret < 0) {
			free(pFile[channel_num]);
			pFile[channel_num] = NULL;
			UTM_PRT("save stream failed!\n");
		}
	}

	return ret;
}

