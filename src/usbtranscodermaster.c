/*
* 
*/

#include "usbtranscodermaster.h"

UMT_WORK_SB umt_work_sb;

int SAMPLE_UMT_Quit(void *param)
{
	umt_work_sb.flag_end_umt = true;
	
	return 0;
}

int SAMPLE_UMT_AddSendFrameRate(void *param)
{
	UMT_WORK_SB *umt_work_sb_t = NULL; 

	umt_work_sb_t = (UMT_WORK_SB *)param;

	if (umt_work_sb_t->h265_ctl.FrameRate < 30) {
		umt_work_sb_t->h265_ctl.FrameRate += 5;
	}

	return 0;
}

int SAMPLE_UMT_MinusSendFrameRate(void *param)
{
	UMT_WORK_SB *umt_work_sb_t = NULL; 

	umt_work_sb_t = (UMT_WORK_SB *)param;

	if (umt_work_sb_t->h265_ctl.FrameRate > 0) {
		umt_work_sb_t->h265_ctl.FrameRate -= 5;
	}

	return 0;
}

int SAMPLE_UMT_EnableSaveFile(void *param)
{
	UMT_WORK_SB *umt_work_sb_t = NULL; 

	umt_work_sb_t = (UMT_WORK_SB *)param;

	umt_work_sb_t->svac2_ctl.enable_savefile = !umt_work_sb_t->svac2_ctl.enable_savefile;

	return 0;
}

int SAMPLE_UMT_UsbSendAndRec_4chn_hevc_1080p(void *param)
{
	if (!umt_work_sb.enter_pth_1chn4k && !umt_work_sb.enter_pth_4chn1k) {
		umt_work_sb.enter_pth_4chn1k = true;
		umt_work_sb.flag_end_umt = false;
		umt_work_sb.h265_ctl.chnum = 4;

		if (-1 == pthread_create(&umt_work_sb.gs_UmtEndpointPid[USB_EP1OUT], 0, SAMPLE_COMM_UMT_UsbSendMultFrameData, (void *)&umt_work_sb)) {
			UTM_PRT("Create pthread failed!\n");
			return -1;
		}
	} else {
		UTM_PRT("The sending thread has been created!\n");
	}
	
	return 0;
}

int SAMPLE_UMT_UsbSendAndRec_1chn_hevc_2160p(void *param)
{
	if (!umt_work_sb.enter_pth_1chn4k && !umt_work_sb.enter_pth_4chn1k) {
		umt_work_sb.enter_pth_1chn4k = true;
		umt_work_sb.flag_end_umt = false;
		umt_work_sb.h265_ctl.chnum = 1;

		if (-1 == pthread_create(&umt_work_sb.gs_UmtEndpointPid[USB_EP1OUT], 0, SAMPLE_COMM_UMT_UsbSendMultFrameData, (void *)&umt_work_sb)) {
			UTM_PRT("Create pthread failed!\n");
			return -1;
		}
	} else {
		UTM_PRT("The sending thread has been created!\n");
	}

	return 0;
}

SAMPLE_UMT_MENU_INFO UMT_menus[] = {
	{'q', "Quit Test", SAMPLE_UMT_Quit, NULL},
	{'a', "Add Send The Frame Rate(5 - 30fps).", SAMPLE_UMT_AddSendFrameRate, (void *)&umt_work_sb},
	{'m', "Minus Send The Frame Rate(5 - 30fps).", SAMPLE_UMT_MinusSendFrameRate, (void *)&umt_work_sb},
	{'z', "Whether to save Svac2 stream?", SAMPLE_UMT_EnableSaveFile, (void *)&umt_work_sb},
	{'4', "4chn-hevc-1080p.", SAMPLE_UMT_UsbSendAndRec_4chn_hevc_1080p, (void *)&umt_work_sb},
	{'1', "1chn-hevc-2160p.", SAMPLE_UMT_UsbSendAndRec_1chn_hevc_2160p, (void *)&umt_work_sb},

};

void SAMPLE_UMT_PrintMenu(SAMPLE_UMT_MENU_INFO *pstmenu_info, int memu_num)
{
    int i;

    for (i = 0; i < memu_num; i++) {
        printf("   press [%c] to %s\n", pstmenu_info[i].cmd, pstmenu_info[i].help_info);
    }
}

void SAMPLE_UMT_HandleMenu(SAMPLE_UMT_MENU_INFO *pstmenu_info)
{
	int memu_num  =sizeof(UMT_menus)/sizeof(SAMPLE_UMT_MENU_INFO);
    char c = 0;
    int i = 0;

    while (umt_work_sb.Umt_Exit == false) {
		printf("\nhelp:\n");
		printf("   Send H265 Frame Rate: %d fps\n", umt_work_sb.h265_ctl.FrameRate);
		printf("   Save SVAC2 stream? value: %d\n", umt_work_sb.svac2_ctl.enable_savefile);
        SAMPLE_UMT_PrintMenu(pstmenu_info, memu_num);

        c = SAMPLE_UMT_PAUSE(" ");

        for(i = 0; i< memu_num;i++) {
			if (c != pstmenu_info[i].cmd)
				continue;
			
            if (pstmenu_info[i].handler == NULL) {
                break;
            }
            pstmenu_info[i].handler(pstmenu_info[i].param);
        }
    }
}

static void usage(char **argv)
{
    printf("Usage: %s [options]\n\n"
            "Options:\n"
            "-h | --help Print this message.\n"
            "-p | --Specify the H265 file.\n"
            "", argv[0]);
}

static void su_handle_signal(int signo)
{
    printf("quit signal %d\n", signo);
    if (SIGINT == signo || SIGTERM == signo || SIGQUIT == signo) {
        umt_work_sb.Umt_Exit = true;
        fclose(stdin); 
    }
}
 
int main(int argc, char *argv[]) 
{
    int ret = 0, opt;
    libusb_device **list;
    libusb_device *usbdev;
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *config_desc;
    const struct libusb_endpoint_descriptor *ep_desc;
    int i = 0, is_match = 0;
    int ep_cnt;
	
	signal(SIGINT,  su_handle_signal);
    signal(SIGTERM, su_handle_signal);
    signal(SIGQUIT, su_handle_signal);
	
	while ((opt = getopt(argc, argv, "hp:")) != -1) {
		switch (opt) {
		case 'h':
			usage(argv);
            exit(0);
		case 'p':
			umt_work_sb.h265_ctl.H265Filepath = optarg;
			UTM_PRT("%s\n", umt_work_sb.h265_ctl.H265Filepath);
			break;
		default:
			fprintf(stderr, "Invalid option '-%c'\n", opt);
			usage(argv);
			return 1;
		}
	}

	ret = SAMPLE_COMM_UMT_FileExist(umt_work_sb.h265_ctl.H265Filepath);
	if (!ret) {
        UTM_PRT("<%s> does not exist!!!\n", umt_work_sb.h265_ctl.H265Filepath);
 
        return -1;
    }
 
    ret = libusb_init(NULL);
    if (ret < 0) {
        fprintf(stderr, "libusb_init failed, ret(%d)\n", ret);
 
        return -1;
    }
 
    // get usb device list
    ret = libusb_get_device_list(NULL, &list);
    if (ret < 0) {
        fprintf(stderr, "libusb_get_device_list failed,"
                "ret(%d)\n", ret);
 
        goto get_failed;
    }
 
    /* print/check the matched device */
    while ((usbdev = list[i++]) != NULL) {
        libusb_get_device_descriptor(usbdev, &dev_desc);
 
#if 0
        UTM_PRT("usb-%d: pid(0x%x), vid(0x%x)\n",
               i++,
               dev_desc.idVendor,
               dev_desc.idProduct);
#endif
 
        if (dev_desc.idVendor == MY_idVendor &&
                dev_desc.idProduct == MY_idProduct) {
            UTM_PRT("match, break!\n");
            is_match = 1;
            break;
        }
    }
 
    if (!is_match) {
        fprintf(stderr, "no matched usb device...\n");
 
        goto match_fail;
    }
 
    /* open usb device */
    ret = libusb_open(usbdev, &umt_work_sb.handle);
    if (ret < 0) {
        fprintf(stderr, "libusb_open failed. ret(%d)\n", ret);
 
        goto open_failed;
    }
 
    UTM_PRT("this usb device has %d configs, "
            "but still use default config 0\n",
            dev_desc.bNumConfigurations);
 
    /* get config descriptor */
    ret = libusb_get_config_descriptor(usbdev, 0, &config_desc);
    if (ret < 0) {
        fprintf(stderr, "get config descriptor failed...\n");
        goto configdesc_fail;
    }
 
    UTM_PRT("the config has %d interface..."
            "just use the 1st interface this time\n",
            config_desc->bNumInterfaces);
 
    ep_cnt = config_desc->interface->altsetting[0].bNumEndpoints;
 
    UTM_PRT("this interface has %d endpoint\n", ep_cnt);
 
    /* get bulk in/out ep */
    for (i = 0; i < ep_cnt; i++) {
        ep_desc = &config_desc->interface->altsetting[0].endpoint[i];
        if ((ep_desc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) & LIBUSB_TRANSFER_TYPE_BULK) {
			switch (ep_desc->bEndpointAddress) {
				case 0x81:
					umt_work_sb.EpAddress[USB_EP1IN] = ep_desc->bEndpointAddress;
					UTM_PRT("got bulk in %#X endppint!\n", umt_work_sb.EpAddress[USB_EP1IN]);
					break;
				case 0x82:
					umt_work_sb.EpAddress[USB_EP2IN] = ep_desc->bEndpointAddress;
					UTM_PRT("got bulk in %#X endppint!\n", umt_work_sb.EpAddress[USB_EP2IN]);
					break;
				case 0x83:
					umt_work_sb.EpAddress[USB_EP3IN] = ep_desc->bEndpointAddress;
					UTM_PRT("got bulk in %#X endppint!\n", umt_work_sb.EpAddress[USB_EP3IN]);
					break;
				case 0x84:
					umt_work_sb.EpAddress[USB_EP4IN] = ep_desc->bEndpointAddress;
					UTM_PRT("got bulk in %#X endppint!\n", umt_work_sb.EpAddress[USB_EP4IN]);
					break;
				case 0x1:
					umt_work_sb.EpAddress[USB_EP1OUT] = ep_desc->bEndpointAddress;
					UTM_PRT("got bulk in %#X endppint!\n", umt_work_sb.EpAddress[USB_EP1OUT]);
					break;
				case 0x2:
					umt_work_sb.EpAddress[USB_EP2OUT] = ep_desc->bEndpointAddress;
					UTM_PRT("got bulk in %#X endppint!\n", umt_work_sb.EpAddress[USB_EP2OUT]);
					break;
				case 0x3:
					umt_work_sb.EpAddress[USB_EP3OUT] = ep_desc->bEndpointAddress;
					UTM_PRT("got bulk in %#X endppint!\n", umt_work_sb.EpAddress[USB_EP3OUT]);
					break;
				case 0x4:
					umt_work_sb.EpAddress[USB_EP4OUT] = ep_desc->bEndpointAddress;
					UTM_PRT("got bulk in %#X endppint!\n", umt_work_sb.EpAddress[USB_EP4OUT]);
					break;
				default:
					UTM_PRT("default ep_addr = %#X\n", ep_desc->bEndpointAddress);
					break;
			}
        }
    }
 
    /* claim the interface */
    UTM_PRT("claim usb interface\n");
	libusb_detach_kernel_driver(umt_work_sb.handle, 0);
    ret = libusb_claim_interface(umt_work_sb.handle, 0);
    if (ret < 0) {
        UTM_PRT("claim usb interface failed... ret(%d)\n", ret);
        goto claim_failed;
    }

    umt_work_sb.h265_ctl.codingFile = SAMPLE_COMM_UMT_OpenFile(umt_work_sb.h265_ctl.H265Filepath, "rb");
    if(umt_work_sb.h265_ctl.codingFile == NULL) {
        UTM_PRT("failed to open %s\n", umt_work_sb.h265_ctl.H265Filepath);
        //return errno;
        goto open_h265file_failed;
    } else {
    	umt_work_sb.h265_ctl.FrameBuffAddr = malloc( 8 * 1024 * 1024 );
		umt_work_sb.h265_ctl.FrameRate = 25;
    }

    if (-1 == pthread_create(&umt_work_sb.gs_UmtEndpointPid[USB_EP4IN], 0, SAMPLE_COMM_UMT_UsbRecIntData, (void *)&umt_work_sb)) {
		UTM_PRT("Create usb Receives pthread failed!\n");
		return -1;
	} else {
		UTM_PRT("Create usb Receives pthread succeed!\n");
	}

    while (umt_work_sb.Umt_Exit == false) {
		
		SAMPLE_UMT_HandleMenu(UMT_menus);
		
	}

	if (umt_work_sb.gs_UmtEndpointPid[USB_EP1OUT]) {
		pthread_join(umt_work_sb.gs_UmtEndpointPid[USB_EP1OUT], NULL);
	}
	if (umt_work_sb.gs_UmtEndpointPid[USB_EP4IN]) {
		pthread_join(umt_work_sb.gs_UmtEndpointPid[USB_EP4IN], NULL);
	}

	//sleep(3);
	UTM_PRT(" Quit the test...\n");
	
	SAMPLE_COMM_UMT_CloseFile(umt_work_sb.h265_ctl.codingFile);
	free(umt_work_sb.h265_ctl.FrameBuffAddr);
	umt_work_sb.h265_ctl.FrameBuffAddr = NULL;


open_h265file_failed:
configdesc_fail:
claim_failed:
    libusb_close(umt_work_sb.handle);
 
open_failed:
match_fail:
    libusb_free_device_list(list, 1);
 
get_failed:
    libusb_exit(NULL);

    UTM_PRT("%s exit\n", __FUNCTION__);
 
    return ret;
}

