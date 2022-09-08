
#include "usbtranscodermaster.h"

///< BULKREAD_CTL bulkread_ctl;


int SAMPLE_COMM_UMT_UsbReadBulk0Data(UMT_WORK_SB *umt_work_sb_t, unsigned int channel_num, unsigned int data_len)
{
	int ret = 0;
	static int count = 0;
	struct timeval Rec_delay_s;
	struct timeval Rec_delay_e;
	int transport_delay;
	float RecSvac2Rate;

	if (count == 0) {
		gettimeofday(&umt_work_sb_t->Rec_tBegin[USB_EP1IN], NULL);
	}

	gettimeofday(&Rec_delay_s, NULL);
	ret = libusb_bulk_transfer(umt_work_sb_t->handle, umt_work_sb_t->EpAddress[USB_EP1IN], 
								(unsigned char*)umt_work_sb_t->recv_data[USB_EP1IN], 
								data_len,
								&umt_work_sb_t->transfered[USB_EP1IN], 
								TIMEOUT);
	gettimeofday(&Rec_delay_e, NULL);
	transport_delay = 1000000L * (Rec_delay_e.tv_sec - Rec_delay_s.tv_sec ) + 
								(Rec_delay_e.tv_usec - Rec_delay_s.tv_usec);
	if (ret == 0) {
		if (umt_work_sb_t->svac2_ctl.enable_savefile) {
			memcpy(&umt_work_sb_t->bulk_ep1in_dec, umt_work_sb_t->recv_data[USB_EP1IN], sizeof(gs_bulk_ep1in_dec));

			SAMPLE_COMM_UMT_SaveStream(channel_num, umt_work_sb_t, umt_work_sb_t->recv_data[USB_EP1IN]);
		}
		if (count++ == 150) {
			count = 0;
			gettimeofday(&umt_work_sb_t->Rec_tEnd[USB_EP1IN], NULL);
			umt_work_sb_t->Rec_deltaTime[USB_EP1IN] = 1000000L * (umt_work_sb_t->Rec_tEnd[USB_EP1IN].tv_sec - umt_work_sb_t->Rec_tBegin[USB_EP1IN].tv_sec ) + 
												(umt_work_sb_t->Rec_tEnd[USB_EP1IN].tv_usec - umt_work_sb_t->Rec_tBegin[USB_EP1IN].tv_usec);
			RecSvac2Rate = (float)(150 * 1000000L / (float)(umt_work_sb_t->Rec_deltaTime[USB_EP1IN] - transport_delay));
			printf("(from device)Receives the svac2 frame rate: %.2f fps\n", RecSvac2Rate);
		}
	} else {
		UTM_PRT("%#X ep failed(%d), transfered[USB_EP1IN](%d)\n", umt_work_sb_t->EpAddress[USB_EP1IN], 
																							ret, 
																							umt_work_sb_t->transfered[USB_EP1IN]);
		//umt_work_sb_t->flag_end_umt = true;
	}

	return ret;
}

void SAMPLE_COMM_UMT_InitSvac2Pa(UMT_WORK_SB *umt_work_sb_t)
{
	int i;

	for (i = 0; i < VENC_MAX_CHN_NUM; i++) {

		umt_work_sb_t->enter_pth_1chn4k = false;
		umt_work_sb_t->enter_pth_4chn1k = false;
		umt_work_sb_t->svac2_ctl.stream_count[i] = 0;
		umt_work_sb_t->svac2_ctl.cnt_index[i] = 0;

		if (umt_work_sb_t->svac2_ctl.svac2File[i]) {
			SAMPLE_COMM_UMT_CloseFile(umt_work_sb_t->svac2_ctl.svac2File[i]);
			umt_work_sb_t->svac2_ctl.svac2File[i] = NULL;
		}
	}
}

void *SAMPLE_COMM_UMT_UsbRecIntData(void *p)
{
	int ret = 0;
	UMT_WORK_SB *umt_work_sb_t = NULL; 

	umt_work_sb_t = (UMT_WORK_SB *)p;

	SAMPLE_COMM_UMT_InitSvac2Pa(umt_work_sb_t);
	
	//while(!umt_work_sb_t->flag_end_umt && !umt_work_sb_t->Umt_Exit) {
	while(!umt_work_sb_t->Umt_Exit) {

		ret = libusb_interrupt_transfer(umt_work_sb_t->handle, 
									umt_work_sb_t->EpAddress[USB_EP4IN], 
									(unsigned char*)umt_work_sb_t->recv_data[USB_EP4IN], 
									1024,
									&umt_work_sb_t->transfered[USB_EP4IN], 
									TIMEOUT);

		if (ret == 0) {
			memcpy(&umt_work_sb_t->ep4in_epxin_interrupt, umt_work_sb_t->recv_data[USB_EP4IN], sizeof(gs_interrupt_data));
			// printf("bulkin_num=%d, data_len=%d, channel_num=%d\n", umt_work_sb_t->ep4in_epxin_interrupt.bulkin_data.bulkin_num,
			// 														umt_work_sb_t->ep4in_epxin_interrupt.bulkin_data.data_len,
			// 														umt_work_sb_t->ep4in_epxin_interrupt.bulkin_data.channel_num);
			/** 说明：
			* 在读完中断数据后，根据bulk号，采用多线程去读不同bulk端点的数据，这样效率最高；
			* 本程序demo只使用了一个bulk端点，所以这里做的串行方式读取数据。
			*/
			switch (umt_work_sb_t->ep4in_epxin_interrupt.bulkin_data.bulkin_num) {
				case 0:
					SAMPLE_COMM_UMT_UsbReadBulk0Data(umt_work_sb_t, 
													umt_work_sb_t->ep4in_epxin_interrupt.bulkin_data.channel_num, 
													umt_work_sb_t->ep4in_epxin_interrupt.bulkin_data.data_len);

				break;

				case 1:
				break;
			}

		} else {
			if (LIBUSB_ERROR_TIMEOUT != ret) {
				UTM_PRT("%#X ep failed(%d), transfered[USB_EP4IN](%d)\n", umt_work_sb_t->EpAddress[USB_EP4IN], ret, 
																	umt_work_sb_t->transfered[USB_EP4IN]);
				break;
			}
			
			//umt_work_sb_t->flag_end_umt = true;
		}
	}

	SAMPLE_COMM_UMT_InitSvac2Pa(umt_work_sb_t);

	UTM_PRT("%s exit\n", __func__);
	
	return NULL;
}

void SAMPLE_COMM_WinUSB_FillBulkOutDesc(gs_bulk_ep1out_dec *BulkDesc, int channel_id, int video_length)
{
    memset(BulkDesc, 0x00, sizeof(gs_bulk_ep1out_dec));
    
    BulkDesc->header.dwtype = 0xFAFAFAFA;
    BulkDesc->header.ver = 1;
    BulkDesc->header.payload_type = VM_USB_DEC_UNCOMPRESSEDFRAME;
    BulkDesc->header.len = 16;

    BulkDesc->bulkout_dec.channel_id = channel_id;
    BulkDesc->bulkout_dec.video_offset = sizeof(gs_bulk_ep1out_dec);
    BulkDesc->bulkout_dec.video_length = video_length;
}

void *SAMPLE_COMM_UMT_UsbSendMultFrameData(void *p)
{
	int ret = 0, H265FrameSize = 0;
	int transport_delay = 0;
	struct timeval read_delay_s;
	struct timeval read_delay_e;
	int read_h265_delay;
	int u_delay;
	// static int flag[8] = {0};
	int chn, total_chn;
	UMT_WORK_SB *umt_work_sb_t = NULL; 
	gs_bulk_ep1out_dec *BulkOutDesc_t = NULL; 

	umt_work_sb_t = (UMT_WORK_SB *)p;
	BulkOutDesc_t = (gs_bulk_ep1out_dec *)umt_work_sb_t->h265_ctl.FrameBuffAddr;
	total_chn = umt_work_sb_t->h265_ctl.chnum;

	while(!umt_work_sb_t->flag_end_umt && !umt_work_sb_t->Umt_Exit) {

		gettimeofday(&read_delay_s, NULL);
		H265FrameSize = SAMPLE_COMM_UMT_GetOneFrameFromFile(umt_work_sb_t->h265_ctl.codingFile, umt_work_sb_t->h265_ctl.FrameBuffAddr + sizeof(gs_bulk_ep1out_dec), PT_H265);
		//H265FrameSize = SAMPLE_COMM_UMT_GetOneFrameFromFile(umt_work_sb_t->h265_ctl.codingFile, umt_work_sb_t->h265_ctl.FrameBuffAddr, PT_H265);
		gettimeofday(&read_delay_e, NULL);
        read_h265_delay = 1000000L * (read_delay_e.tv_sec - read_delay_s.tv_sec ) + 
                            		(read_delay_e.tv_usec - read_delay_s.tv_usec);
		if (H265FrameSize) {

			for (chn = 0; chn < total_chn; chn++) {

				SAMPLE_COMM_WinUSB_FillBulkOutDesc(BulkOutDesc_t, chn, H265FrameSize);

				H265FrameSize += sizeof(gs_bulk_ep1out_dec);

				gettimeofday(&umt_work_sb_t->Send_tBegin[USB_EP1OUT], NULL);

				ret = libusb_bulk_transfer(umt_work_sb_t->handle, 
											umt_work_sb_t->EpAddress[USB_EP1OUT], 
											(unsigned char*)umt_work_sb_t->h265_ctl.FrameBuffAddr, 
											// 发送数据长度
											H265FrameSize,
											&umt_work_sb_t->transfered[USB_EP1OUT], 
											TIMEOUT);
											
				gettimeofday(&umt_work_sb_t->Send_tEnd[USB_EP1OUT], NULL);
				umt_work_sb_t->Send_deltaTime[USB_EP1OUT] = 1000000L * (umt_work_sb_t->Send_tEnd[USB_EP1OUT].tv_sec - umt_work_sb_t->Send_tBegin[USB_EP1OUT].tv_sec ) + 
															(umt_work_sb_t->Send_tEnd[USB_EP1OUT].tv_usec - umt_work_sb_t->Send_tBegin[USB_EP1OUT].tv_usec);
				
				if (ret == 0) {
					transport_delay = umt_work_sb_t->Send_deltaTime[USB_EP1OUT];
					// UTM_PRT("[chn]: %d;\n[Frame Size]: %d Byte;\n[Usb Send 'a Frame' Time]: %ld us\n\n",chn,
					// 																										H265FrameSize, 
					//  																										umt_work_sb_t->Send_deltaTime[USB_EP1OUT]);

				} else {
					UTM_PRT("%#X ep failed(%d), transfered[USB_EP1OUT](%d)\n", umt_work_sb_t->EpAddress[USB_EP1OUT], ret, 
																				umt_work_sb_t->transfered[USB_EP1OUT]);
					umt_work_sb_t->flag_end_umt = true;
				}

				/** 说明：
				* WinUSB设备驱动version: 2022-08-31后的版本，增加USER_ZLP_END参数。
				* USER_ZLP_END=0 时：只有发送的帧数据长度能被 1024 整除时，才绑定发送一个空包；
				* USER_ZLP_END=1 时：发送的帧数据均需要绑定空包，被1024整除时，需要绑定两个空包。
				* 在本次主控程序中，使用 USER_ZLP_END=0 的方式给设备发送数据。
				*/
				if (H265FrameSize % 1024 == 0) {
					//UTM_PRT("Send ZLP\n");
					gettimeofday(&umt_work_sb_t->Send_tBegin[USB_EP1OUT], NULL);
					ret = libusb_bulk_transfer(umt_work_sb_t->handle, 
											umt_work_sb_t->EpAddress[USB_EP1OUT], 
											(unsigned char*)umt_work_sb_t->h265_ctl.FrameBuffAddr, 
											0,
											&umt_work_sb_t->transfered[USB_EP1OUT], 
											TIMEOUT);
					gettimeofday(&umt_work_sb_t->Send_tEnd[USB_EP1OUT], NULL);
					umt_work_sb_t->Send_deltaTime[USB_EP1OUT] = 1000000L * (umt_work_sb_t->Send_tEnd[USB_EP1OUT].tv_sec - umt_work_sb_t->Send_tBegin[USB_EP1OUT].tv_sec ) + 
																(umt_work_sb_t->Send_tEnd[USB_EP1OUT].tv_usec - umt_work_sb_t->Send_tBegin[USB_EP1OUT].tv_usec);
					
					if (ret == 0) {
						transport_delay += umt_work_sb_t->Send_deltaTime[USB_EP1OUT];
						//printf("[Usb Send 'zero-length packet' Time]: %ld us\n", umt_work_sb_t->Send_deltaTime[USB_EP1OUT]);
					} else {
						UTM_PRT("%#X ep failed(%d), transfered[USB_EP1OUT](%d)\n",
								umt_work_sb_t->EpAddress[USB_EP1OUT], ret, umt_work_sb_t->transfered[USB_EP1OUT]);
						umt_work_sb_t->flag_end_umt = true;
					}
				}

				H265FrameSize -= sizeof(gs_bulk_ep1out_dec);

				// if (flag[chn] == 0) {
				// 	flag[chn]++;
				// 	usleep(200*1000);
				// }

			}
			// 简单的延时控制帧率，将减去usb传输延迟和读取h265帧数据延迟(误差在零点几帧左右)。
			u_delay = (umt_work_sb_t->h265_ctl.FrameRate == 0 ? 1000 : (1000 / umt_work_sb_t->h265_ctl.FrameRate)) * 1000 - transport_delay*total_chn - read_h265_delay;
			if (u_delay > 0) {
				usleep(u_delay);
			}
			transport_delay = 0;
		} else {
			//umt_work_sb_t->flag_end_umt = true;
			//fseek(umt_work_sb_t->h265_ctl.codingFile, 0, SEEK_SET);
			SAMPLE_COMM_UMT_FseekFile(umt_work_sb_t->h265_ctl.codingFile, 0, SEEK_SET);
		}

	}

	umt_work_sb_t->enter_pth_4chn1k = false;
	umt_work_sb_t->enter_pth_1chn4k = false;

	//fseek(umt_work_sb_t->h265_ctl.codingFile, 0, SEEK_SET);
	SAMPLE_COMM_UMT_FseekFile(umt_work_sb_t->h265_ctl.codingFile, 0, SEEK_SET);

	UTM_PRT("%s exit\n", __func__);
	
	return NULL;
}


