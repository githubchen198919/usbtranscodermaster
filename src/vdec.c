
#include "usbtranscodermaster.h"


unsigned int SAMPLE_COMM_UMT_GetOneFrameFromFile(FILE *codingFile, char *pu8Addr, PAYLOAD_TYPE_E codeType)
{
    SAMPLE_VDEC_NAL_TYPE nalType;
    unsigned int frameLen;
    unsigned char currentStage = 0; // 0-3: find header(00 00 00 01); 4: parse type; 9: found
    unsigned char flagHeaderFound = 0;
    frameLen = 0;
    do{
        if(fread(&pu8Addr[frameLen], 1, 1, codingFile) <= 0)
            break;
        switch(currentStage){
            case 0:
            case 1:
            case 2:
                if(pu8Addr[frameLen] != 0x00){
                    currentStage = 0;
                }
                else
                    currentStage ++;                // [1-3]*0x00
                break;
            case 3:
                if(pu8Addr[frameLen] == 0x00)    // [3-N]*0x00
                    break;
                if(pu8Addr[frameLen] != 0x01){   // [3-N]*0x00 not followed by 0x01: BAD header
                    currentStage = 0;
                    break;
                }
                if(flagHeaderFound){
                    frameLen -= 4;
                    currentStage = 9;
                    fseek(codingFile, -4, SEEK_CUR);
                }
                else
                    currentStage = 4;
                break;
            case 4:
                //if(pu8Addr[*frameLen] & 0x80 == 0x80 )
                if(codeType == PT_SVAC2)
                    nalType = (pu8Addr[frameLen] & 0x3C) >> 2;
                else
                    nalType = pu8Addr[frameLen] >> 1;
                switch(nalType){
// #define SVAC2_PARSE_EL_AND_BL_AS_ONE_FRAME
#ifndef SVAC2_PARSE_EL_AND_BL_AS_ONE_FRAME
                    case SAMPLE_VDEC_NAL_TYPE_HEVC_BSLICE:
						//printf("SAMPLE_VDEC_NAL_TYPE_HEVC_BSLICE\n");
						flagHeaderFound = 1;
                        break;
                    case SAMPLE_VDEC_NAL_TYPE_HEVC_PSLICE:
						// P 
						//printf("[ P-frame ]\n");
						flagHeaderFound = 1;
                        break;
                    // case SAMPLE_VDEC_NAL_TYPE_SVAC_NON_ISLICE:
                    case SAMPLE_VDEC_NAL_TYPE_SVAC_ISLICE:
						//printf("SAMPLE_VDEC_NAL_TYPE_SVAC_ISLICE\n");
						flagHeaderFound = 1;
                        break;
                    // case SAMPLE_VDEC_NAL_TYPE_SVAC_ISLICE_EL:
                    // case SAMPLE_VDEC_NAL_TYPE_SVAC_NON_ISLICE_EL:
#endif
                    case SAMPLE_VDEC_NAL_TYPE_HEVC_IDR_W_RADL:
						// I 
						//printf("[ I-frame ]\n");
						flagHeaderFound = 1;
                        break;
                    case SAMPLE_VDEC_NAL_TYPE_HEVC_IDR_N_LP:
						//printf("SAMPLE_VDEC_NAL_TYPE_HEVC_IDR_N_LP\n");
						flagHeaderFound = 1;
                        break;
                    case SAMPLE_VDEC_NAL_TYPE_HEVC_CRA:
						// I 
						//printf("[ I-frame ]\n");
						flagHeaderFound = 1;
                        break;
                    case SAMPLE_VDEC_NAL_TYPE_HEVC_XXXXX3:
						//printf("SAMPLE_VDEC_NAL_TYPE_HEVC_XXXXX3\n");
                    // case SAMPLE_VDEC_NAL_TYPE_HEVC_SPS:
                    // case SAMPLE_VDEC_NAL_TYPE_SVAC_SPS:
                    // case SAMPLE_VDEC_NAL_TYPE_SVAC_PPS:
                        flagHeaderFound = 1;
                        break;
                    default:
                        break;
                }
                currentStage = 0;
                break;
            default:
                currentStage = 0;
                break;
        }
        frameLen ++;
    }while(currentStage != 9);
    // if(testParam[0].u32FeedIndex % 5 == 1)
    // {
    //     return test;
    // }
    return frameLen;
}
