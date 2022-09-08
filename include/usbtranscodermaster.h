#ifndef _UTM_H
#define _UTM_H

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <signal.h>
#include <regex.h>
#include <poll.h>
#include <sys/ioctl.h>    
#include <stropts.h> 
#include <stdbool.h>

#include <libusb.h>
#include <vdec.h>
#include <usbprotocol.h>
 
#define 	TIMEOUT 			5000
#define 	STR_LEN 			1024*1024*2
#define 	MY_idVendor 		0x0AC8
#define 	MY_idProduct 		0x7180
#define 	VENC_MAX_CHN_NUM	8
#define 	USB_MAX_EP_NUM		8

#define	UTM_PRT(fmt...)   									\
		 do {												 	\
			 printf("[%s] - %d: ", __FUNCTION__, __LINE__);   	\
			 printf(fmt);									 	\
			}while(0)

enum _endpoint_pid {
	USB_EP1OUT	= 0,
	USB_EP2OUT	= 1,
	USB_EP3OUT	= 2,
	USB_EP4OUT	= 3,
	USB_EP1IN	= 4,
	USB_EP2IN	= 5,
	USB_EP3IN	= 6,
	USB_EP4IN	= 7,
} EP_PID;

typedef struct _h265_ctl {
	int FrameRate;
	int chnum;
	char *H265Filepath;
	char *FrameBuffAddr;
	FILE* codingFile;
	
} H265_CTL;

typedef struct _svac2_ctl {
	bool enable_savefile;
	int stream_count[VENC_MAX_CHN_NUM];
	int cnt_index[VENC_MAX_CHN_NUM];
	char aszFileName[VENC_MAX_CHN_NUM][128];
	FILE* svac2File[VENC_MAX_CHN_NUM];
	
} SVAC2_CTL;

typedef struct _umt_work_sb {
	bool flag_end_umt;
	bool Umt_Exit;
	bool enter_pth_1chn4k;
	bool enter_pth_4chn1k;
	pthread_t gs_UmtEndpointPid[USB_MAX_EP_NUM];
	long Rec_deltaTime[USB_MAX_EP_NUM], Send_deltaTime[USB_MAX_EP_NUM];
	struct timeval Rec_tBegin[USB_MAX_EP_NUM], Rec_tEnd[USB_MAX_EP_NUM], Send_tBegin[USB_MAX_EP_NUM], Send_tEnd[USB_MAX_EP_NUM];
	unsigned char EpAddress[USB_MAX_EP_NUM];
	
	int transfered[USB_MAX_EP_NUM];
	libusb_device_handle *handle;
	char send_data[USB_MAX_EP_NUM][STR_LEN];
    char recv_data[USB_MAX_EP_NUM][STR_LEN];

    H265_CTL h265_ctl;
    SVAC2_CTL svac2_ctl;

    gs_interrupt_data ep4in_epxin_interrupt;
    gs_bulk_ep1in_dec bulk_ep1in_dec;
    gs_bulk_ep1out_dec bulk_ep1out_dec;
	
} UMT_WORK_SB;

typedef struct _BulkRead_ctl {
	pthread_t p_atter;
	pthread_mutex_t p_mutex;
	pthread_cond_t p_cond;
	int p_run;
	unsigned int data_len;					//the length of the data to be read
	unsigned int channel_num;				// 0 - 7 
	UMT_WORK_SB *umt_work_info;
	bool p_exit;
} BULKREAD_CTL;

typedef struct menu_info {
	char cmd;
    char *help_info;
    int (*handler)(void *param);
    void* param;
} SAMPLE_UMT_MENU_INFO;

#define SAMPLE_UMT_PAUSE(...) ({ \
    int ch1, ch2; \
    printf(__VA_ARGS__); \
    ch1 = getchar(); \
    if (ch1 != '\n' && ch1 != EOF) { \
        while((ch2 = getchar()) != '\n' && (ch2 != EOF)); \
    } \
    ch1; \
})

void *SAMPLE_COMM_UMT_UsbRecIntData(void *p);
void *SAMPLE_COMM_UMT_UsbSendFrameData(void *p);	
void *SAMPLE_COMM_UMT_UsbSendMultFrameData(void *p);	
void SAMPLE_COMM_UMT_InitSvac2Pa(UMT_WORK_SB *umt_work_sb_t);

FILE *SAMPLE_COMM_UMT_OpenFile(const char *filename, const char *mode);
int SAMPLE_COMM_UMT_CloseFile(FILE * stream);
size_t SAMPLE_COMM_UMT_WriteFile(const void *ptr, size_t size, FILE *stream);
int SAMPLE_COMM_UMT_FseekFile(FILE *stream, long offset, int fromwhere);
int SAMPLE_COMM_UMT_FileExist(const char *path);

size_t SAMPLE_COMM_UMT_SaveStream(unsigned int channel_num, UMT_WORK_SB *umt_work_sb_t, char *buf);

#endif
