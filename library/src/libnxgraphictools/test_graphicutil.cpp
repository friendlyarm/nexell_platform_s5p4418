#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <errno.h>

#include <linux/fb.h>	//	for FB
#include <sys/types.h>	//	for open
#include <sys/stat.h>	//	for open
#include <fcntl.h>		//	for open
#include <sys/mman.h>	//	for mmap
#include <sys/ioctl.h>	//	for _IOWR

#include <nx_fourcc.h>
#include <nx_alloc_mem.h>
#include <nx_graphictools.h>
#include "src/vr_deinterlace.h"
#include "src/vr_egl_runtime.h"

#include "src/dbgmsg.h"

#define	ALIGNED16(X)	(((X+15)>>4)<<4)

void print_usage(const char *appName)
{
	printf("\nUsage : %s [options]\n", appName);
	printf("  -h                  : help\n");
	printf("  -f [file name]      : Input file name(mandatory)\n");
	printf("  -o [file name]      : Output file name(optional)\n");
	printf("  -s [width,height]   : Source file image size(mandatory)\n");
	printf("  -d [width,height]   : Destination file image size(optional)\n");
	printf("  -m [1~5]            : m = 1 Deinterlace)\n");
	printf("	                    m = 2 Scaling\n");
	printf("	                    m = 3 Deinterlace & Scaling\n");
	printf("	                    m = 4 Frame buffer caputer & Encoding\n");
	printf("	                    m = 5 YUV(YV12) to RGB(ARGB)\n");
	printf("                        m = 6 RGB(ARGB) to YUV(YV12)\n");
	printf("  -t                  : Thread test mode\n");
	printf("-----------------------------------------------------------------------\n");
	printf(" example) \n");
	printf(" 1. deinterlace test\n");
	printf("  #> %s -m 1 -f input.yuv -s 1920,1080\n", appName);
	printf(" 2. scale test\n");
	printf("  #> %s -m 2 -f input.yuv -s 1920,1080 -d 1280,720\n", appName);
	printf(" 3. deinterlace & scale test\n");
	printf("  #> %s -m 3 -f input.yuv -s 1920,1080 -d 1280,720 -o deintscale_out.yuv\n", appName);
	printf(" 4. framebuffer capture & Encoding\n");
	printf("  #> %s -m 4 -d 1280,720\n", appName);
	printf(" 5. Yuv 2 Rgb Test\n");
	printf("  #> %s -m 5 -f input.yuv -s 1280,720\n", appName);
	printf(" 6. Rgb 2 Yuv Test\n");
	printf("  #> %s -m 6 -f input.rgba -s 1280,720\n", appName);
}

const char *gstModeStr[] = 
{
	"Deinterlace Mode",
	"Scaling Mode",
	"Deinterlace & Scaling Mode",
	"FrameBuffer Capture Mode",
	"YUV to RGB Mode",
	"RGB to YUV Mode"
};

//#define	USE_SW_DEINTERLACE
void _SWDeinterlace(NX_VID_MEMORY_INFO *pInBuf, NX_VID_MEMORY_INFO *pOutBuf);


#define	NUM_FB_BUFFER	3
#define	FB_DEV_NAME	"/dev/graphics/fb0"
#define NXPFB_GET_FB_FD		_IOWR('N', 101, __u32)
#define NXPFB_GET_ACTIVE	_IOR('N', 103, __u32)
//#define	ENABLE_FB_INFO

#define VSYNC_ON        "1"
#define VSYNC_OFF       "0"
#define VSYNC_CTL_FILE  "/sys/devices/platform/display/active.0"
#define VSYNC_MON_FILE  "/sys/devices/platform/display/vsync.0"

//////////////////////////////////////////////////////////////////////////////
//
//	Implementation Mode 5 ( YUV to RGB )
//

//
//	Step 1. Load YUV Image
//	Step 2. Create RGB Buffer(ION)
//	Step 3. Do YUV to RGB
//	Step 4. Save RGB Buffer
//
static int MODE5_YUV2RGB( const char *inFileName, int srcWidth, int srcHeight, const char *outFileName, int dstWidth, int dstHeight )
{
	NX_GT_YUV2RGB_HANDLE hYuv2Rgb = NULL;		//	Converter Handle
	NX_VID_MEMORY_HANDLE hInImage = NULL;		//	Input Image Handle
	NX_MEMORY_HANDLE hOutImage = NULL;			//	OUtput Image Handle

	FILE *inFd = NULL;							//	Input File Handle
	FILE *outFd = NULL;							//	Output File Handle
	int ret = 0, mode = 5;

	//	Step 0. Prepare processing.
	if( inFileName == NULL )
	{
		printf("%s() Error : input file name!!!\n", __func__);
		ret = -1;
		goto ErrorExit;
	}

	inFd = fopen( inFileName, "rb" );
	if( inFd == NULL )
	{
		printf( "%s() Error : Cannot open file(%s)", __func__, inFileName );
		ret = -1;
		goto ErrorExit;
	}

	if( outFileName == NULL )
		outFd = fopen("Yuv2RgbResult.rgb", "wb");
	else
		outFd = fopen(outFileName, "wb");

	if( outFd == NULL )
	{
		printf( "%s() Error : Cannot open file.(%s)", __func__, !outFileName ? "Yuv2RgbResult.rgb" : outFileName );
		ret = -1;
		goto ErrorExit;
	}

	//	Print In/Out Informations
	printf("\n====================================================\n");
	printf("  Mode = %s(%d)\n", gstModeStr[mode-1], mode);
	printf("  Source filename   : %s\n", inFileName);
	printf("  Source Image Size : %d, %d\n", srcWidth, srcHeight);
	printf("  Output filename   : %s\n", !outFileName ? "Yuv2RgbResult.rgb" : outFileName);
	printf("  Output Image Size : %d, %d\n", dstWidth, dstHeight);
	printf("====================================================\n");

	//	Step 1. Load YUV Image
	hInImage = NX_VideoAllocateMemory( 16, srcWidth, srcHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
	if( hInImage == NULL )
	{
		printf("%s() Error : Cannot Allocate hInImage!!!\n", __func__);
		ret = -1;
		goto ErrorExit;
	}
	fread( (unsigned char*)hInImage->luVirAddr, 1, srcWidth*srcHeight,   inFd );
	fread( (unsigned char*)hInImage->cbVirAddr, 1, srcWidth*srcHeight/4, inFd );
	fread( (unsigned char*)hInImage->crVirAddr, 1, srcWidth*srcHeight/4, inFd );

	//	Step 2. Create RGB Buffer(ION)
	hOutImage = NX_AllocateMemory( dstWidth*dstHeight*4, 4096 );
	if( hOutImage == NULL )
	{
		printf("%s() Error : Cannot Allocate hOutImage!!!\n", __func__);
		ret = -1;
		goto ErrorExit;
	}

	//	Step 3. Do YUV to RGB
	hYuv2Rgb = NX_GTYuv2RgbOpen( srcWidth, srcHeight, dstWidth, dstHeight, 1 );
	if( hYuv2Rgb == NULL )
	{
		printf("%s() Error : NX_GTYuv2RgbOpen() failed!!!\n", __func__);
		ret = -1;
		goto ErrorExit;
	}
	ret = NX_GTYuv2RgbDoConvert( hYuv2Rgb, hInImage, (int)hOutImage->privateDesc );
	if( ret != 0 )
	{
		printf("%s() Error : NX_GTYuv2RgbDoConvert() failed!!!(ret=%d)\n", __func__, ret);
		goto ErrorExit;
	}

	//	Step 4. Save RGB Buffer
	rewind(outFd);
	fwrite( (unsigned char*)hOutImage->virAddr, 1, dstWidth*dstHeight*4, outFd );
	printf("Converting Done.\n");

ErrorExit:
	if( inFd != NULL )
	{
		fclose( inFd );
	}
	if( outFd != NULL )
	{
		fclose( outFd );
	}
	if( hInImage != NULL )
	{
		NX_FreeVideoMemory( hInImage );
	}
	if( hOutImage != NULL )
	{
		NX_FreeMemory( hOutImage );
	}
	if( hYuv2Rgb != NULL )
	{
		NX_GTYuv2RgbClose( hYuv2Rgb );
	}
	return ret;
}

//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//	Implementation Mode 6 ( RGB to YUV )
//

//
//	Step 1. Load RGB Image
//	Step 2. Create YUV Buffer(ION)
//	Step 3. Do RGB to YUV
//	Step 4. Save YUV Buffer
//
static int MODE6_RGB2YUV( const char *inFileName, int srcWidth, int srcHeight, const char *outFileName, int dstWidth, int dstHeight )
{
	NX_GT_RGB2YUV_HANDLE hRgb2Yuv = NULL;		//	Converter Handle
	NX_MEMORY_HANDLE hInImage = NULL;			//	Input Image Handle
	NX_VID_MEMORY_HANDLE hOutImage = NULL;		//	OUtput Image Handle

	FILE *inFd = NULL;							//	Input File Handle
	FILE *outFd = NULL;							//	Output File Handle
	int ret = 0, mode = 6;

	//	Step 0. Prepare processing.
	if( inFileName == NULL )
	{
		printf("%s() Error : input file name!!!\n", __func__);
		ret = -1;
		goto ErrorExit;
	}
	
	inFd = fopen( inFileName, "rb" );
	if( inFd == NULL )
	{
		printf( "%s() Error : Cannot open file(%s)", __func__, inFileName );
		ret = -1;
		goto ErrorExit;
	}
	
	if( outFileName == NULL )
		outFd = fopen("Rgb2YuvResult.yuv", "wb");
	else
		outFd = fopen(outFileName, "wb");

	if( outFd == NULL )
	{
		printf( "%s() Error : Cannot open file.", __func__ );
		ret = -1;
		goto ErrorExit;
	}

	//	Print In/Out Informations
	printf("\n====================================================\n");
	printf("  Mode = %s(%d)\n", gstModeStr[mode-1], mode);
	printf("  Source filename   : %s\n", inFileName);
	printf("  Source Image Size : %d, %d\n", srcWidth, srcHeight);
	printf("  Output filename   : %s\n", !outFileName ? "Rgb2YuvResult.yuv" : outFileName);
	printf("  Output Image Size : %d, %d\n", dstWidth, dstHeight);
	printf("====================================================\n");

	//	Step 1. Load RGB Image
	hInImage = NX_AllocateMemory( srcWidth*srcHeight*4, 16 );
	if( hInImage == NULL )
	{
		printf("%s() Error : Cannot Allocate hInImage!!!\n", __func__);
		ret = -1;
		goto ErrorExit;
	}	
	fread( (unsigned char*)hInImage->virAddr, 1, srcWidth*srcHeight*4, inFd );

	//	Step 2. Create YUV Buffer(ION)
	hOutImage = NX_VideoAllocateMemory( 16, dstWidth, dstHeight, NX_MEM_MAP_LINEAR, FOURCC_NV12 );	// FOURCC_MVS0, FOURCC_NV12
	if( hOutImage == NULL )
	{
		printf("%s() Error : Cannot Allocate hOutImage!!!\n", __func__);
		ret = -1;
		goto ErrorExit;
	}

	//	Step 3. Do RGB to YUV
	hRgb2Yuv = NX_GTRgb2YuvOpen( srcWidth, srcHeight, dstWidth, dstHeight, 1 );
	if( hRgb2Yuv == NULL )
	{
		printf("%s() Error : NX_GTRgb2YuvOpen() failed!!!\n", __func__);
		ret = -1;
		goto ErrorExit;
	}
	ret = NX_GTRgb2YuvDoConvert( hRgb2Yuv, (int)hInImage->privateDesc, hOutImage );
	if( ret != 0 )
	{
		printf("%s() Error : NX_GTRgb2YuvDoConvert() failed!!!(ret=%d)\n", __func__, ret);
		goto ErrorExit;
	}	

	//	Step 4. Save YUV Buffer
	fwrite( (unsigned char*)hOutImage->luVirAddr, 1, dstWidth*dstHeight, outFd );
	fwrite( (unsigned char*)hOutImage->cbVirAddr, 1, dstWidth*dstHeight / 2, outFd );

	printf("Converting Done.\n");

ErrorExit:
	if( inFd != NULL )
	{
		fclose( inFd );
	}
	if( outFd != NULL )
	{
		fclose( outFd );
	}
	if( hInImage != NULL )
	{
		NX_FreeMemory( hInImage );
	}
	if( hOutImage != NULL )
	{
		NX_FreeVideoMemory( hOutImage );
	}
	if( hRgb2Yuv != NULL )
	{
		NX_GTRgb2YuvClose( hRgb2Yuv );
	}
	return ret;
}

//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//	Implementation Mode 99 ( Dinterlace -> Scaling Asing Test )
//

static int AGING_TEST( const char *inFileName, int srcWidth, int srcHeight, const char *outFileName, int dstWidth, int dstHeight )
{
	unsigned int counter = 0;
	NX_VID_MEMORY_HANDLE inBuffer  = NULL;
	NX_VID_MEMORY_HANDLE tmpBuffer = NULL;
	NX_VID_MEMORY_HANDLE outBuffer = NULL;
	NX_GT_DEINT_HANDLE   hDeint = NULL;
	NX_GT_SCALER_HANDLE  hScale = NULL;
	unsigned int time_start, time_end;
	
	inBuffer = NX_VideoAllocateMemory( 4096, srcWidth, srcHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
	memset( (unsigned char*)inBuffer->luVirAddr, 0x7f, srcWidth*srcHeight   );
	memset( (unsigned char*)inBuffer->cbVirAddr, 0x7f, srcWidth*srcHeight/4 );
	memset( (unsigned char*)inBuffer->crVirAddr, 0x7f, srcWidth*srcHeight/4 );

	tmpBuffer = NX_VideoAllocateMemory( 4096, srcWidth, srcHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
	outBuffer = NX_VideoAllocateMemory( 4096, dstWidth, dstHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );

	while( 1 )
	{
		time_start = (unsigned int)base_util_time_get_usec();

		counter++;
		printf("Loop Counter : %d\n", counter);
		
		//	Open Deinterlace		
		if( NULL == (hDeint = NX_GTDeintOpen( srcWidth, srcHeight, 1 )) )
		{
			printf("NX_DeintOpen() failed!!!\n");
			break;
		}

		//	Open Sacler
		if( NULL == (hScale = NX_GTSclOpen( srcWidth, srcHeight, dstWidth, dstHeight, 1 )) )
		{
			printf("NX_SclOpen() failed!!!\n");
			break;
		}

		if( 0 != NX_GTDeintDoDeinterlace( hDeint, inBuffer, tmpBuffer ) )
		{
			printf("NX_GTDeintDoDeinterlace failed!!!\n");
			break;
		}
		//	Deinterlace --> Scaling
		if( 0 != NX_GTSclDoScale( hScale, tmpBuffer, outBuffer ) )
		{
			printf("NX_GTSclDoScale failed!!!\n");
			break;
		}

		if( hDeint ) NX_GTDeintClose(hDeint);
		if( hScale ) NX_GTSclClose(hScale);
		hDeint = NULL;
		hScale = NULL;

		time_end = (unsigned int)base_util_time_get_usec();
		//if(!(counter % 100))
		DbgMsg("[%5i]----TIME. %5i us\n", counter, time_end-time_start);
	}

	if( inBuffer )
		NX_FreeVideoMemory(inBuffer);
	if( outBuffer )
		NX_FreeVideoMemory(outBuffer);
	if( tmpBuffer )
		NX_FreeVideoMemory(tmpBuffer);

	if( hDeint )
		NX_GTDeintClose(hDeint);
	if( hScale )
		NX_GTSclClose(hScale);

	return 0;
}

//
//////////////////////////////////////////////////////////////////////////////


#include <nx_video_api.h>

//#define	DISABLE_ENCODING
//#define	DISABLE_CONVERTING

//
//	Application Sequence
//	Step
//		1. Initialize & Open Devices
//		2. Enable Vertical Sync Event
//		3. Wait Vertical Sync Event
//		4. Get Currrent FrameBuffer Index
//		5. Convert FrameBuffer(RGB32) to Encodable Memory(NV12)
//		6. Encoding Image
//		7. Write Capture Image
//		8. Loop 3 ~ 7
//
int FrameBufferCapture( const char *outFileName, int outWidth, int outHeight )
{
	int err = 0;
	static char uevent_desc[4096];
	memset(uevent_desc, 0, sizeof(uevent_desc));
	unsigned char seqBuf[1024];
	int seqBufSize=1024;
	NX_VID_ENC_IN encIn;
	NX_VID_ENC_OUT encOut;
	NX_VID_ENC_HANDLE hEncoder = NULL;

	int instIdx;

	//	Frame Buffer Related Variables
	int vsync_fd = -1;						//	Vertical Sync
	int vsync_ctl_fd = -1;					//	Vertical Sync En/Disalbe Descriptor
	int fbFd = -1;							//	Frame Buffer File Descriptor
	int ionFd[NUM_FB_BUFFER] = {-1,};		//	Frame Buffer's ION Memory File Descriptor
	struct fb_var_screeninfo info;			//	for Frame Buffer Screen Information

	//	Frame Buffer Capture Related
	NX_GT_RGB2YUV_HANDLE hRgb2Yuv = NULL;
	NX_VID_MEMORY_HANDLE encBuffer = NULL;

	//	Output File Descriptor
	FILE *outFd = NULL;
	int capCnt = 0;

	//
	//	Step 1. Initialize & Open Drivers
	//

	//	Open Vertical Sync
	vsync_fd = open(VSYNC_MON_FILE, O_RDONLY);		//	Open Vertical Sync Monitor File
	vsync_ctl_fd = open(VSYNC_CTL_FILE, O_RDWR);	//	Open Vertical Sync Control File

	//	Open FrameBuffer & Frame Buffer Information
	fbFd = open(FB_DEV_NAME, O_RDWR, 0);			//	Open FrameBuffer Device Driver
	if( fbFd<0 )
	{
		printf("Cannot create FB Driver(%s)\n", FB_DEV_NAME);
		goto ErrorExit;
	}
	if (ioctl(fbFd, FBIOGET_VSCREENINFO, &info) == -1)
	{
		printf("FBIOGET_VSCREENINFO ioctl failed\n");
		goto ErrorExit;
	}
#ifdef ENABLE_FB_INFO
	printf("\n===========================================\n");
	printf(" Frame Buffer Information :\n");
	printf("===========================================\n");
	printf(" xres           : %d\n", info.xres);
	printf(" yres           : %d\n", info.yres);
	printf(" bits_per_pixel : %d\n", info.bits_per_pixel);
	printf(" width          : %d\n", info.width);
	printf(" height         : %d\n", info.height);
	printf("===========================================\n");
#endif
	//	Get Frame Buffer's ION File Descriptor
	for( int i=0 ; i<NUM_FB_BUFFER ; i++ )
	{
		ionFd[i] = i;
		int ret = ioctl(fbFd, NXPFB_GET_FB_FD, &ionFd[i]);
		if (ret < 0)
		{
			printf("%s: failed to NXPFB_GET_FB_FD!!!(%d)\n", __func__, ret);
			goto ErrorExit;
		}
	}

#ifndef DISABLE_ENCODING
	//	Set to Default SIze
	if( outWidth==0 || outHeight==0 )
	{
		outWidth = info.xres;
		outHeight = info.yres;
	}

	printf("Encoding Size = %dx%d\n", outWidth, outHeight);
	hEncoder = NX_VidEncOpen( NX_AVC_ENC, &instIdx );
	if( hEncoder )
	{
		NX_VID_ENC_INIT_PARAM initParam;
		memset( &initParam, 0, sizeof(initParam) );

		initParam.width = outWidth;
		initParam.height = outHeight;
		initParam.fpsNum = 30;
		initParam.fpsDen = 1;
		initParam.gopSize = 30;
		initParam.bitrate = 5000000;
		initParam.chromaInterleave = 1;
		initParam.enableAUDelimiter = 0;			//	Enable / Disable AU Delimiter
		initParam.searchRange = 0;

		//	Rate Control
		initParam.maximumQp= 51;
		initParam.disableSkip = 0;
		initParam.initialQp = 23;
		initParam.enableRC = 1;
		initParam.RCAlgorithm = 1;
		initParam.rcVbvSize = -1;

		NX_VidEncInit( hEncoder, &initParam );
		NX_VidEncGetSeqInfo(hEncoder, seqBuf, &seqBufSize);
	}
	else
	{
		printf("Encoder Open Failed!!!\n");
		goto ErrorExit;
	}
#endif

#ifndef DISABLE_CONVERTING
	//	Open RGB to YUV Engine
	hRgb2Yuv = NX_GTRgb2YuvOpen( info.xres, info.yres, outWidth, outHeight, 4 );
	if( hRgb2Yuv == NULL )
	{
		printf("NX_GTRgb2YuvOpen Failed!!\n");
		goto ErrorExit;
	}
	encBuffer = NX_VideoAllocateMemory( 4096, outWidth, outHeight, NX_MEM_MAP_LINEAR, FOURCC_NV12 );
	if( NULL == encBuffer )
	{
		printf("NX_VideoAllocateMemory failed!!(Encoder input buffer)\n");
		goto ErrorExit;
	}
#endif

	//
	//	Step 2. Enable Vertical Sync
	//
	{
		err = write(vsync_ctl_fd, VSYNC_ON, sizeof(VSYNC_ON));
		if( err < 0 )
		{
			printf("Virtical sync eanble failed!!!\n");
			goto ErrorExit;
		}
	}

	struct pollfd fds[1];
	fds[0].fd = vsync_fd;
	fds[0].events = POLLPRI;

#ifndef DISABLE_ENCODING
	if( outFileName == NULL )
	{
		outFd = fopen("./FrameBufferCap.264", "wb");
	}
	else
	{
		outFd = fopen(outFileName, "wb");
	}

	if( outFd && seqBufSize>0 )
	{
		fwrite(seqBuf, 1, seqBufSize, outFd);
	}
#endif

	while(true)
	{
		//
		//	Step 3. Wait Vertical Sync Event
		//
		err = poll(fds, 1, -1);
		if (err > 0)
		{
			if( fds[0].revents & POLLPRI )
			{
				int encRet;
				int activeFbIdx;
				//
				//	Step 4. Get Currrent FrameBuffer Index
				//
				if( ioctl(fbFd, NXPFB_GET_ACTIVE, &activeFbIdx) < 0 )
				{
					printf("Cannot Get Active Framebuffer Index\n");
					goto ErrorExit;
				}

				if( activeFbIdx >= NUM_FB_BUFFER || activeFbIdx < 0 )
				{
					printf("Invalid active FbIndex (%d)\n", activeFbIdx);
					goto ErrorExit;
				}
#ifndef DISABLE_CONVERTING
				//
				//	Step 5. Convert FrameBuffer to Encoding Memory
				//
				printf("ActiveFd[%d] = %d, ConvertResult = %d\n", activeFbIdx, ionFd[activeFbIdx], NX_GTRgb2YuvDoConvert( hRgb2Yuv, ionFd[activeFbIdx], encBuffer ) );
#endif
				capCnt ++;

				if( capCnt == 100 )
					break;

#ifndef DISABLE_ENCODING
				//
				//	Step 6. Encoding Image
				//
				encIn.forcedIFrame = 0;
				encIn.forcedSkipFrame = 0;
				encIn.quantParam = 23;
				encIn.timeStamp = 0;
				encRet = NX_VidEncEncodeFrame( hEncoder, &encIn, &encOut );

				//
				//	Step 7. Write Encoding
				//
				if( encRet == 0 )
				{
					printf("Out size = %d\n", encOut.bufSize);
					if( outFd )
					{
						fwrite(encOut.outBuf, 1, encOut.bufSize, outFd);
					}
				}
				else
				{
					printf("Encoder Error : encRet = %d\n", encRet);
				}
#endif
			}
		}
		else if (err == -1)
		{
			if (errno == EINTR)
				break;
			printf("error in vsync thread: %s", strerror(errno));
		}
	}

ErrorExit:
#ifndef DISABLE_ENCODING
	if( hEncoder )
	{
		NX_VidEncClose( hEncoder );
	}
	if( outFd )
	{
		fclose( outFd );
	}
#endif
#ifndef DISABLE_CONVERTING
	if( hRgb2Yuv )
	{
		NX_GTRgb2YuvClose( hRgb2Yuv );
	}
#endif
	if( vsync_fd > 0 )
	{
		close(vsync_fd);
	}
	if( vsync_ctl_fd> 0 )
	{
		close( vsync_ctl_fd );
	}
	// for( int i=0 ; i<3 ; i++ )
	// {
	// 	printf("====== Release ION FD[%d]\n", i);
	// 	if( ionFd[i] > 0 )
	// 	{
	// 		close( ionFd[i] );
	// 	}
	// }
	if( fbFd > 0 )
	{
		printf("====== fbFd\n");
		close( fbFd );
	}
	if( encBuffer )
	{
		NX_FreeVideoMemory( encBuffer );
	}
	return 0;
}


int main( int argc, char *argv[] )
{
	int opt;
	char *inFileName = NULL;
	char *outFileName = NULL;
	int srcWidth = 0, srcHeight = 0;
	int dstWidth = 0, dstHeight = 0;
	int mode = 0;

	while( -1 != (opt=getopt(argc, argv, "hf:o:s:d:m:")))
	{
		switch( opt ){
			case 'h':
				print_usage( argv[0] );
				return 0;
			case 'f':
				inFileName = strdup(optarg);
				break;
			case 'o':
				outFileName = strdup(optarg);
				break;
			case 's':
				sscanf( optarg, "%d,%d", &srcWidth, &srcHeight );
				break;
			case 'd':
				sscanf( optarg, "%d,%d", &dstWidth, &dstHeight );
				break;
			case 'm':
				mode = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if( dstWidth==0 && dstHeight==0 )
	{
		dstWidth = srcWidth;
		dstHeight = srcHeight;
	}

	//	Destination width & height
	if( mode == 1 )
	{
		printf("Deinterlace Mode.\n");
		if( (srcWidth!=dstWidth) || (srcHeight!=dstHeight) )
		{
			printf(" Error Invalid Argument\n");
			printf(" Source Size(%dx%d) != Output Size(%dx%d) !!!\n", srcWidth, srcHeight, dstWidth, dstHeight);
			exit(-1);
		}
	}
	else if( mode == 2 )
	{
		printf("Scaling Mode.\n");
		if( (srcWidth == dstWidth) && (srcHeight == dstHeight) )
		{
			printf(" Warning!! Source Size(%dx%d) == Output Size(%dx%d) !!!\n", srcWidth, srcHeight, dstWidth, dstHeight);
		}
	}
	else if( mode == 3 )
	{
		printf("Deinterlace & Scaling Mode.\n");
		if( (srcWidth == dstWidth) && (srcHeight == dstHeight) )
		{
			printf(" Warning!! Source Size(%dx%d) == Output Size(%dx%d) !!!\n", srcWidth, srcHeight, dstWidth, dstHeight);
			printf(" Mode change : Deinterlace & Scale Mode  to Deinterlace only mode.!!!\n");
			mode = 1;
		}
	}
	else if( mode == 4 )
	{
		printf("dstWidth = %d, dstHeight=%d\n", dstWidth, dstHeight);
		return FrameBufferCapture(outFileName, dstWidth, dstHeight);
	}
	else if( mode == 5 )
	{
		printf("YUV 2 RGB Mode.\n");
		return MODE5_YUV2RGB( inFileName, srcWidth, srcHeight, outFileName, dstWidth, dstHeight );
	}
	else if( mode == 6 )
	{
		printf("RGB 2 YUV Mode.\n");
		return MODE6_RGB2YUV( inFileName, srcWidth, srcHeight, outFileName, dstWidth, dstHeight );
	}
	else if( mode == 99 )
	{
		printf("================ Aging Test ================\n");
		return AGING_TEST( inFileName, srcWidth, srcHeight, outFileName, dstWidth, dstHeight );
	}
	else
	{
		printf("Invalid Mode\n");
		print_usage( argv[0] );
		exit(-1);
	}

	if( outFileName == NULL )
	{
		outFileName = (char*)malloc( strlen(inFileName) + 5 );
		strcpy( outFileName, inFileName );
		strcat( outFileName, ".out" );
		outFileName[ strlen(outFileName)+1 ] = 0;
	}

	//	Print In/Out Informations
	printf("\n====================================================\n");
	printf("  Mode = %s(%d)\n", gstModeStr[mode-1], mode);
	printf("  Source filename   : %s\n", inFileName);
	printf("  Source Image Size : %d, %d\n", srcWidth, srcHeight);
	printf("  Output filename   : %s\n", outFileName);
	printf("  Output Image Size : %d, %d\n", dstWidth, dstHeight);
	printf("====================================================\n");

	//
	//	Load Input Image to Input Buffer
	//	 : Allocation Memory --> Load Image
	//
	NX_VID_MEMORY_HANDLE inBuffer = NULL; 	//	Allocate 3 plane memory for YUV
	inBuffer = NX_VideoAllocateMemory( 4096, srcWidth, srcHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );

	//	Load Image to Allocated Memory
	FILE *inFd = fopen( inFileName, "rb");
	if( inFd == NULL )
	{
		printf("Cannot open input file(%s)\n", inFileName);
		exit(-1);
	}
	else
	{
		size_t allocSize = (srcWidth * srcHeight * 3) / 2;
		size_t readSize = 0;
		unsigned char *inImgBuffer = (unsigned char *)malloc(allocSize);
		unsigned char *tmpSrc;
		unsigned char *tmpDst;
		readSize = fread( inImgBuffer, 1, allocSize, inFd );

		printf("ReadSize = %d\n", readSize);

		if( readSize != allocSize )
		{
			printf("[Warning] Input file information is not valid.!!\n");
		}
		tmpSrc = inImgBuffer;
		//	Load Lu
		tmpDst = (unsigned char *)inBuffer->luVirAddr;
		for( int i=0 ; i<srcHeight ; i ++ )
		{
			memcpy( tmpDst, tmpSrc, srcWidth );
			tmpDst += inBuffer->luStride;
			tmpSrc += srcWidth;
		}
		//	Load Cb
		tmpDst = (unsigned char *)inBuffer->cbVirAddr;
		for( int i=0 ; i<srcHeight/2 ; i ++ )
		{
			memcpy( tmpDst, tmpSrc, srcWidth/2 );
			tmpDst += inBuffer->cbStride;
			tmpSrc += srcWidth/2;
		}
		//	Load Cr
		tmpDst = (unsigned char *)inBuffer->crVirAddr;
		for( int i=0 ; i<srcHeight/2 ; i ++ )
		{
			memcpy( tmpDst, tmpSrc, srcWidth/2 );
			tmpDst += inBuffer->crStride;
			tmpSrc += srcWidth/2;
		}
		fclose( inFd );
	}

	//
	//	Deinter / Scale / Deinterlace & Scale
	//
	NX_VID_MEMORY_HANDLE outBuffer = NX_VideoAllocateMemory( 4096, dstWidth, dstHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
	NX_VID_MEMORY_HANDLE tmpBuffer = NULL;
	NX_GT_DEINT_HANDLE hDeint = NULL;
	NX_GT_SCALER_HANDLE hScale = NULL;

	switch( mode )
	{
		case 1:	//	Deinterlace Only
		{
#ifndef		USE_SW_DEINTERLACE
			hDeint = NX_GTDeintOpen( srcWidth, srcHeight, 1 );
			if( NULL == hDeint )
			{
				printf("NX_DeintOpen() failed!!!\n");
				exit(1);
			}
			printf("Hardware!!!\n");
			NX_GTDeintDoDeinterlace( hDeint, inBuffer, outBuffer );
#else
			_SWDeinterlace( inBuffer, outBuffer );
#endif
			break;
		}
		case 2:	//	Scaling Only
		{
			hScale = NX_GTSclOpen( srcWidth, srcHeight, dstWidth, dstHeight, 1 );
			if( NULL == hScale )
			{
				printf("NX_SclOpen() failed!!!\n");
				exit(1);
			}
			NX_GTSclDoScale( hScale, inBuffer, outBuffer );
			break;
		}
		case 3:	//	Deinterlace -> Scaling
		{
			tmpBuffer = NX_VideoAllocateMemory( 4096, srcWidth, srcHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );

			if( NULL == (hDeint = NX_GTDeintOpen( srcWidth, srcHeight, 1 )) )
			{
				printf("NX_DeintOpen() failed!!!\n");
				exit(1);
			}

			if( NULL == (hScale = NX_GTSclOpen( srcWidth, srcHeight, dstWidth, dstHeight, 1 )) )
			{
				printf("NX_SclOpen() failed!!!\n");
				exit(1);
			}

			NX_GTDeintDoDeinterlace( hDeint, inBuffer, tmpBuffer );
			NX_GTSclDoScale( hScale, tmpBuffer, outBuffer );
		}
	}

	//
	//	Write to Output File
	//
	FILE *outFd = fopen(outFileName, "wb");

	if( outFd != NULL )
	{
		for( int i=0 ; i<dstHeight ; i++ )
		{
			fwrite( (unsigned char*)(outBuffer->luVirAddr + outBuffer->luStride*i), 1, dstWidth, outFd );
		}
		for( int i=0 ; i<dstHeight/2 ; i++ )
		{
			fwrite( (unsigned char*)(outBuffer->cbVirAddr + i * outBuffer->luStride/2), 1, dstWidth/2, outFd );
		}
		for( int i=0 ; i<dstHeight/2 ; i++ )
		{
			fwrite( (unsigned char*)(outBuffer->crVirAddr + i * outBuffer->luStride/2), 1, dstWidth/2, outFd );
		}
		fclose(outFd);
	}
	else
	{
		printf("Cannot open output file(%s)\n", outFileName);
		exit(-1);
	}


	if( inFileName )
		free( inFileName );

	if( outFileName )
		free( outFileName );

	if( outBuffer )
		NX_FreeVideoMemory(outBuffer);

	if( tmpBuffer )
		NX_FreeVideoMemory(tmpBuffer);

	if( hDeint != NULL )
	{
		NX_GTDeintClose(hDeint);
	}

	if( hScale != NULL )
	{
		NX_GTSclClose(hScale);
	}

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
//
//
//	Low Leve Debugging Functions
//
//


#if 0	//	Low Level Debugging Function for Frame Buffer Capture

#include <vr_deinterlace_private.h>
int FrameBufferCapture( const char *outFileName, int outWidth, int outHeight )
{
	//	Get Frame Buffer Information
	int ionFd[NUM_FB_BUFFER] = {-1,};
	int fbFd = open(FB_DEV_NAME, O_RDWR, 0);
	if( fbFd<0 )
	{
		printf("Cannot create FB Driver(%s)\n", FB_DEV_NAME);
		return -1;
	}

	struct fb_var_screeninfo info;
	if (ioctl(fbFd, FBIOGET_VSCREENINFO, &info) == -1)
	{
		return -1;
	}

	printf("\n===========================================\n");
	printf(" Frame Buffer Information :\n");
	printf("===========================================\n");
	printf(" xres           : %d\n", info.xres);
	printf(" yres           : %d\n", info.yres);
	printf(" bits_per_pixel : %d\n", info.bits_per_pixel);
	printf(" width          : %d\n", info.width);
	printf(" height         : %d\n", info.height);
	printf("===========================================\n");

	//	Get Frame Buffer's ION File Descriptor
	for( int i=0 ; i<NUM_FB_BUFFER ; i++ )
	{
		ionFd[i] = i;
		int ret = ioctl(fbFd, NXPFB_GET_FB_FD, &ionFd[i]);
		if (ret < 0)
		{
			printf("%s: failed to NXPFB_GET_FB_FD!!!", __func__);
			return -1;
		}
	}

	vrInitializeGLSurface();

	{
		NX_VID_MEMORY_HANDLE outBuffer = NX_VideoAllocateMemory( 4096, info.xres, info.yres, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
		HSURFSOURCE source = vrCreateCvt2YSource (info.xres, info.yres, (NX_MEMORY_HANDLE)ionFd[0]);
		HSURFTARGET target = vrCreateCvt2YTarget(info.xres, info.yres, (NX_MEMORY_HANDLE)outBuffer->privateDesc[0]);
		
		vrRunCvt2Y(target, source);
		vrWaitCvt2YDone();

		vrDestroyCvt2YTarget(target);
		vrDestroyCvt2YSource(source);
		int size = info.xres * info.yres;
		unsigned char *virAddr = (unsigned char *)outBuffer->luVirAddr;
		printf("dst surface(0x%x)\n", (int)virAddr);

	    FILE *outFd;
	    if( outFileName )
	    {
	    	outFd = fopen(outFileName, "wb");
	    }
	    else
	    {
	    	outFd = fopen("framebuffer.rgb.y", "wb");
	    }
	    if( outFd == NULL )
	    {
	    	printf("Cannot open output file\n");
	    	return -1;
	    }
	    fwrite(virAddr, 1, size, outFd);
	    fclose( outFd );
		if( fbFd )
		{
			close( fbFd );
		}
		if( outBuffer )
			NX_FreeVideoMemory(outBuffer);	
	}
	{
		NX_VID_MEMORY_HANDLE outBuffer = NX_VideoAllocateMemory( 4096, info.xres, info.yres/2, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
		HSURFSOURCE source = vrCreateCvt2UVSource (info.xres, info.yres, (NX_MEMORY_HANDLE)ionFd[0]);
		HSURFTARGET target = vrCreateCvt2UVTarget(info.xres, info.yres, (NX_MEMORY_HANDLE)outBuffer->privateDesc[0]);
		
		vrRunCvt2UV(target, source);
		vrWaitCvt2UVDone();

		vrDestroyCvt2UVTarget(target);
		vrDestroyCvt2UVSource(source);
		int size = (info.xres * info.yres) /2;
		unsigned char *virAddr = (unsigned char *)outBuffer->luVirAddr;
		printf("dst surface(0x%x)\n", (int)virAddr);

	    FILE *outFd;
	    if( outFileName )
	    {
	    	outFd = fopen(outFileName, "wb");
	    }
	    else
	    {
	    	outFd = fopen("framebuffer.rgb.uv", "wb");
	    }
	    if( outFd == NULL )
	    {
	    	printf("Cannot open output file\n");
	    	return -1;
	    }
	    fwrite(virAddr, 1, size, outFd);
	    fclose( outFd );
		if( fbFd )
		{
			close( fbFd );
		}
		if( outBuffer )
			NX_FreeVideoMemory(outBuffer);	
	}
	vrTerminateGLSurface();
	return 0;
}
#endif


#if 0	//	Low Level Debugging Function for Scaler & Deinterlacer & CVT Operations
#define VR_SURFACE_WIDTH		1920//1920//1280
#define VR_SURFACE_HEIGHT		1080//1080//800
#define VR_INPUT_WIDTH			1920//1920//1280
#define VR_INPUT_HEIGHT			1080//1080//800

#define VR_SCALE_IN_WIDTH			1920//1920//1280
#define VR_SCALE_IN_HEIGHT			1080//1080//800
#define VR_SCALE_OUT_WIDTH			1280//1920//1280
#define VR_SCALE_OUT_HEIGHT			720//1080//800

#define VR_CVT_WIDTH			1024
#define VR_CVT_HEIGHT			768

#define VR_SURF_BUFF_CNT	3

static NX_MEMORY_HANDLE mem_alloc(unsigned int size, unsigned int align)
{	
	NX_MEMORY_HANDLE handle = NX_AllocateMemory(size, align);
	if(!handle)
	{	
		fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}
	//printf("mem_alloc, fd(0x%x, 0x%x, 0x%x)\n", (int)handle, (int)handle->privateDesc, handle->virAddr);
	return handle;
}

static void mem_free(NX_MEMORY_HANDLE handle)
{
	NX_FreeMemory((NX_MEMORY_HANDLE)handle);
}

static void* mem_get_ptr(void* handle)
{
		return (void*)(((NX_MEMORY_HANDLE)handle)->virAddr);
}

static void mem_save_to_file(char* pbuf, const char *filename, unsigned int buf_idx, int is_yuv, unsigned int width, unsigned int height, unsigned int bytes_per_pixel)
{
	char sName[256];
	static int run_count;
	FILE* fp;
	unsigned int bytes_size = width * bytes_per_pixel * height;
	if(is_yuv)
		sprintf(sName, "./res/image/rst_%s%d_%d_%dx%d_%d.yuv", filename, buf_idx, run_count++, width, height, bytes_per_pixel);
	else
		sprintf(sName, "./res/image/rst_%s%d_%d_%dx%d_%d.rgba", filename, buf_idx, run_count++, width, height, bytes_per_pixel);
	fp = fopen(sName, "w");	
	fwrite(pbuf, bytes_size , 1, fp);
	fflush(fp);		
	fclose(fp);
}

#define DEINTERLACE_OUT_SCALE_IN
#define TEST_DEINTERLACE
#define TEST_SCALE
#define TEST_CVT2RGBA
NX_VID_MEMORY_HANDLE tempBuffer0  = NULL;
NX_VID_MEMORY_HANDLE tempBuffer1  = NULL;
static float gTemp;
static void test_dummy_init(void)
{
	tempBuffer0 = NX_VideoAllocateMemory( 4096, 1920, 1080, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
	if (!tempBuffer0)
	{
		ErrMsg("failed to NX_VideoAllocateMemory\n");
	}
}
static void test_dummy_deinit(void)
{
	if( tempBuffer0 )
		NX_FreeVideoMemory(tempBuffer0);
}
static int test_dummy_run(void)
{
	/*int fd_handle = (int)(((NX_MEMORY_HANDLE)tempBuffer1->privateDesc[0])->privateDesc);
	int mem_fd = dup(fd_handle);
	int mapping_size = 1920 * 1080 * 4;
	void* mapping;
	*/
	
	unsigned int time_start, time_end;
	
	time_start = (unsigned int)base_util_time_get_usec();

	tempBuffer1 = NX_VideoAllocateMemory( 4096, 1920, 1080, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
	if (!tempBuffer1)
	{
		ErrMsg("failed to NX_VideoAllocateMemory\n");
	}
	for(int i = 0 ; i < 100 ; i++)
	{
		gTemp += i;
		for(int j = 0 ; j < 100 ; j++)
		{
			gTemp /= (float)(j+i);	
		}
		gTemp += (int)gTemp;
	}
	if( tempBuffer1 )
		NX_FreeVideoMemory(tempBuffer1);

	/*
	mapping = mmap(0, mapping_size, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0);
	if (MAP_FAILED == mapping)
	{
		ErrMsg("failed to map dma-buf memory\n");
		return -1;
	}
	*/
	time_end = (unsigned int)base_util_time_get_usec();
	DbgMsg("TIME. test_dummy_run() %5i us\n", time_end-time_start);
	
	return 0;
}

int main(void)
{
	/* Declare variables. */
	unsigned int time_start, time_end;
	int bEnd = 0;
	unsigned int input_width = VR_INPUT_WIDTH;
	unsigned int input_height = VR_INPUT_HEIGHT;
	NX_MEMORY_HANDLE pInData [VR_SURF_BUFF_CNT ] = {NULL, };
	NX_MEMORY_HANDLE pOutData[VR_SURF_BUFF_CNT] = {NULL, };
	NX_MEMORY_HANDLE pInScaleData[VR_SURF_BUFF_CNT] = {NULL, };
	NX_MEMORY_HANDLE pOutScaleData[VR_SURF_BUFF_CNT] = {NULL, };
	NX_MEMORY_HANDLE pInCvtDataY = NULL;
	NX_MEMORY_HANDLE pInCvtDataU = NULL;
	NX_MEMORY_HANDLE pInCvtDataV = NULL;
	NX_MEMORY_HANDLE pOutCvtData = NULL;
	
	HSURFSOURCE surface_input [VR_SURF_BUFF_CNT ] = {NULL, };
	HSURFTARGET surface_output[VR_SURF_BUFF_CNT] = {NULL, };
	HSURFSOURCE scale_input[VR_SURF_BUFF_CNT] = {NULL, };
	HSURFTARGET scale_output[VR_SURF_BUFF_CNT] = {NULL, };
	HSURFSOURCE cvt_input = NULL;
	HSURFTARGET cvt_output = NULL;
	
	/* Initialize windowing system. */
#if 1 /* for test */
	FILE *fp = NULL;
	FILE *fp_scale = NULL;
	FILE *fp_cvt = NULL;
	DbgMsg( "test start\n" );
	//fp = fopen("./res/image/StoneRGB_yuv420.bin" , "rb" );
	//fp = fopen("./res/image/CaptureImage_004.yuv" , "rb" );
	fp = fopen("./res/image/girl_1920x1080.yuv" , "rb" );
	#if !defined( DEINTERLACE_OUT_SCALE_IN )
	//fp_scale = fopen("./res/image/lena_64x64.yuv" , "rb" );	
	fp_scale = fopen("./res/image/girl_1920x1080.yuv" , "rb" );
	#endif
	fp_cvt = fopen("./res/image/penguins_1024_768.yuv" , "rb" );	
#endif

	//unsigned int buffer_idx = 0;	
	unsigned int test_cnt = 0, test_max_cnt = 10000;

	//while(!bEnd)
	{
		
		/* for test */	
		//create deinterlace target and source mem
		for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
		{
			pInData[i] = mem_alloc(input_width * input_height * 1, 4);
			if(!pInData[i])
			{
				fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
				exit(-1);
			}
			fseek(fp, 0L, SEEK_SET);
			fread(mem_get_ptr(pInData[i]), input_width * input_height * 1, 1, fp);
		}
		for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
		{
			pOutData[i] = mem_alloc(VR_SURFACE_WIDTH * VR_SURFACE_HEIGHT * 1, 4);
			if(!pOutData[i])
			{
				fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
				exit(-1);
			}
		}
		
		//create scaler target and source mem
		for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
		{
			#ifdef DEINTERLACE_OUT_SCALE_IN
			pInScaleData[i] = pOutData[i];
			#else
			pInScaleData[i] = mem_alloc(VR_SCALE_IN_WIDTH * VR_SCALE_IN_HEIGHT * 1, 4);
			if(!pInScaleData)
			{
				fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
				exit(-1);
			}		
			fseek(fp_scale, 0L, SEEK_SET);
			fread(mem_get_ptr(pInScaleData[i]), input_width * input_height * 1, 1, fp_scale);
			#endif
		}
		for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
		{
			pOutScaleData[i] = mem_alloc(VR_SCALE_OUT_WIDTH * VR_SCALE_OUT_HEIGHT * 1, 4);
			if(!pOutScaleData[i])
			{
				fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
				exit(-1);
			}
		}
		
		//create cvt target and source mem
		pInCvtDataY = mem_alloc(VR_CVT_WIDTH * VR_CVT_HEIGHT * 1, 4);
		if(!pInCvtDataY)
		{
			fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
			exit(-1);
		}
		pInCvtDataU = mem_alloc((VR_CVT_WIDTH * VR_CVT_HEIGHT)/4, 4);
		if(!pInCvtDataU)
		{
			fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
			exit(-1);
		}
		pInCvtDataV = mem_alloc((VR_CVT_WIDTH * VR_CVT_HEIGHT)/4, 4);
		if(!pInCvtDataV)
		{
			fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
			exit(-1);
		}
		fseek(fp_cvt, 0L, SEEK_SET);
		fread(mem_get_ptr(pInCvtDataY), VR_CVT_WIDTH * VR_CVT_HEIGHT, 1, fp_cvt);
		fread(mem_get_ptr(pInCvtDataU), (VR_CVT_WIDTH * VR_CVT_HEIGHT)/4, 1, fp_cvt);
		fread(mem_get_ptr(pInCvtDataV), (VR_CVT_WIDTH * VR_CVT_HEIGHT)/4, 1, fp_cvt);
		pOutCvtData = mem_alloc(VR_CVT_WIDTH * VR_CVT_HEIGHT * 4, 4);
		if(!pOutCvtData)
		{
			fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
			exit(-1);
		}

		//
		//	Initialize
		//
		
		//
		//	Create
		//
		//-------------------------------1---------------------------------------
		while(!bEnd)
		//if(0)
		{
			vrInitializeGLSurface();		
			time_start = (unsigned int)base_util_time_get_usec();
		
			
			//-------------------------------2---------------------------------------
#ifdef TEST_DEINTERLACE
			//	create target & source
			{
				//create deinterlace target and source handle
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{	
					surface_input[i] = vrCreateDeinterlaceSource( input_width, input_height, pInData[i] );
					if( ! surface_input[i] )
					{
						fprintf(stderr, "Err! %s:%i\n", __FILE__, __LINE__);
					}
				}
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{	
					surface_output[i] = vrCreateDeinterlaceTarget( VR_SURFACE_WIDTH, VR_SURFACE_HEIGHT, pOutData[i], 0 );
					if( ! surface_output[i] )
					{
						fprintf(stderr, "Err! %s:%i\n", __FILE__, __LINE__);
					}
				}
			}
#endif

#ifdef TEST_DEINTERLACE		
			//	Run
			{			
				//	deinterlace
				#if 0
				time_start = (unsigned int)base_util_time_get_usec();
				time_end = (unsigned int)base_util_time_get_usec();
				time_start = time_end;
				if(!(test_cnt % 100))
					DbgMsg("[%5i]----TIME-0. %5i us\n", test_cnt, time_end-time_start);
				#endif	
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{
					vrRunDeinterlace(surface_output[i], surface_input[i]);		
					vrWaitDeinterlaceDone();
					#if 1
					if(!test_cnt || test_cnt == (test_max_cnt/2) || test_cnt == (test_max_cnt - 1))
					{			
						printf("output, %dx%d\n", VR_SURFACE_WIDTH, VR_SURFACE_HEIGHT);
						mem_save_to_file((char*)mem_get_ptr(pOutData[i]), "deinter", i, 1, VR_SURFACE_WIDTH, VR_SURFACE_HEIGHT, 1);
					}
					#endif		 
				}
				

				#if 0
				//memcpy(mem_get_ptr(pInScaleData), mem_get_ptr(pOutData[buffer_idx]), VR_SURFACE_WIDTH* VR_SURFACE_HEIGHT * 1);
				fread(mem_get_ptr(pInScaleData), VR_SCALE_IN_WIDTH * VR_SCALE_IN_WIDTH * 1, 1, fp_scale);
				#endif
			}
#endif

#ifdef TEST_SCALE
			//	create target & source
			{
				//create scaler target and source handle
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{	
					scale_input[i] = vrCreateScaleSource( VR_SCALE_IN_WIDTH, VR_SCALE_IN_HEIGHT, pInScaleData[i] );
					if( ! scale_input[i] )
					{
						fprintf(stderr, "Err! %s:%i\n", __FILE__, __LINE__);
					}
				}
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{	
					scale_output[i] = vrCreateScaleTarget( VR_SCALE_OUT_WIDTH, VR_SCALE_OUT_HEIGHT, pOutScaleData[i], 0 );
					if( ! scale_output[i] )
					{
						fprintf(stderr, "Err! %s:%i\n", __FILE__, __LINE__);
					}
				}
			}
#endif

#ifdef TEST_SCALE	
			//	Run
			{
				//	scale
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{
					vrRunScale(scale_output[i], scale_input[i]);		
					vrWaitDeinterlaceDone();
					#if 1
					if(!test_cnt || test_cnt == (test_max_cnt/2) || test_cnt == (test_max_cnt - 1))
					{			
						printf("output, %dx%d\n", VR_SCALE_OUT_WIDTH, VR_SCALE_OUT_HEIGHT);
						mem_save_to_file((char*)mem_get_ptr(pOutScaleData[i]), "scale", i, 1, VR_SCALE_OUT_WIDTH, VR_SCALE_OUT_HEIGHT, 1);
					}
					#endif	
				}
			}
#endif		

#ifdef TEST_CVT2RGBA
				//create cvt2rgba target and source handle
				cvt_output = vrCreateCvt2RgbaTarget( VR_CVT_WIDTH, VR_CVT_HEIGHT, pOutCvtData, 0 );
				if( ! cvt_output )
				{
					fprintf(stderr, "Err! %s:%i\n", __FILE__, __LINE__);
				}
				cvt_input = vrCreateCvt2RgbaSource( VR_CVT_WIDTH, VR_CVT_HEIGHT, pInCvtDataY, pInCvtDataU, pInCvtDataV );
				if( ! cvt_input )
				{
					fprintf(stderr, "Err! %s:%i\n", __FILE__, __LINE__);
				}
#endif

				//-------------------------------3---------------------------------------
				/* Test drawing. */
				#if 0 /* for test */
				{
					unsigned char* pbyte = (unsigned char*)mem_get_ptr(pInData[buffer_idx]);
					/*
					for( int y=0; y<input_height; y++ )
					for( int x=0; x<64; x++ )
					{
						if( y<input_height/2 )
							pbyte[y*input_width+x] = (y&1) ? 0xFF : 0x0;	
						else
							pbyte[y*input_width+x] = (y&1) ? 128 : 64;	
					}
					*/
					for( int k=0; k<input_height-4; k++ )
					{
						pbyte[k*input_width+k+0] = 255;	
						pbyte[k*input_width+k+1] =   0;	
						pbyte[k*input_width+k+2] = 128;	
						pbyte[k*input_width+k+3] =  64;	
					}
				}
				#endif	

#ifdef TEST_CVT2RGBA
			{
				//	CVT2Rgba
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{
					vrRunCvt2Rgba(cvt_output, cvt_input);		
					vrWaitCvt2RgbaDone();
					#if 1
					if(!test_cnt || test_cnt == (test_max_cnt/2) || test_cnt == (test_max_cnt - 1))
					{			
						printf("output, %dx%d\n", VR_CVT_WIDTH, VR_CVT_HEIGHT);
						mem_save_to_file((char*)mem_get_ptr(pOutCvtData), "cvt2rgba", 0, 0, VR_CVT_WIDTH, VR_CVT_HEIGHT, 4);
					}
					#endif	
				}
			}
#endif				

			#if 0
			++buffer_idx;
			if(buffer_idx == VR_SURF_BUFF_CNT)
				buffer_idx = 0;
			#endif	
				
			//
			//	Destroy
			//		
			{	
				//-------------------------------8---------------------------------------
#ifdef TEST_DEINTERLACE		
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{
					vrDestroyDeinterlaceSource(surface_input[i]);
				}
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{
					vrDestroyDeinterlaceTarget(surface_output[i], 0);
				}			
#endif	
#ifdef TEST_SCALE		
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{
					vrDestroyScaleSource(scale_input[i]);
				}
				for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
				{
					vrDestroyScaleTarget(scale_output[i], 0);
				}
#endif	
#ifdef TEST_CVT2RGBA		
				vrDestroyCvt2RgbaSource(cvt_input);
				vrDestroyCvt2RgbaTarget(cvt_output, 0);
#endif	
			}
					
			++test_cnt;
			if(test_cnt == test_max_cnt)
				bEnd = 1;					

			time_end = (unsigned int)base_util_time_get_usec();
			DbgMsg("[%5i]	 TIME. %5i us\n", test_cnt, time_end-time_start);
			
			vrTerminateGLSurface();
		}

		//
		//	End
		//	
		DbgMsg("/////////////////////////////////////////////////////////");
		DbgMsg("/////////////////////////////////////////////////////////");
		DbgMsg("Test End! test_cnt(%d)\n", test_cnt);
		DbgMsg("/////////////////////////////////////////////////////////");
		DbgMsg("/////////////////////////////////////////////////////////");

		//printf( "vrTerminateGLSurface end\n" );

		for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
		{
			if(pInData[i])
				mem_free(pInData[i]);
		}
		for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
		{
			if(pOutData[i])
				mem_free(pOutData[i]);
		}
		#if !defined( DEINTERLACE_OUT_SCALE_IN )
		for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
		{
			if(pInScaleData[i])
				mem_free(pInScaleData[i]);
		}
		#endif
		for(int i = 0 ; i < VR_SURF_BUFF_CNT ; i++)
		{
			if(pOutScaleData[i])
				mem_free(pOutScaleData[i]);
		}
	}
	
	if(fp)
		fclose(fp);
	if(fp_scale)
		fclose(fp_scale);
	if(fp_cvt)
		fclose(fp_cvt);
		
	printf("Test exit\n");
    return 0;
}

#endif

//
//
//////////////////////////////////////////////////////////////////////////////


//
//	Linear
//
#if 0	//	linear
void _SWDeinterlace(NX_VID_MEMORY_INFO *pInBuf, NX_VID_MEMORY_INFO *pOutBuf)
{
	int32_t i, j;
	uint8_t *pSrc;
	uint8_t *pDst;

	//	Process Y
	pSrc = (uint8_t*) pInBuf->luVirAddr;
	pDst = (uint8_t*) pOutBuf->luVirAddr;

	int32_t width = pInBuf->imgWidth;
	int32_t height = pInBuf->imgHeight;
	int32_t stride = pInBuf->luStride;

	for( i=1 ; i < height-1 ; i++ )
	{
		if( i&1 )
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = (pSrc[(i-1)*stride + j] + pSrc[(i+1)*stride + j])/2;
			}
		}
		else
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = pSrc[i*stride + j];
			}
		}
	}

	width /= 2;
	height /= 2;
	stride = pInBuf->cbStride;

	//	Process U
	pSrc = (uint8_t*) pInBuf->cbVirAddr;
	pDst = (uint8_t*) pOutBuf->cbVirAddr;
	for( i=0 ; i < height ; i++ )
	{
		if( i&1 )
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = (pSrc[(i-1)*stride + j] + pSrc[(i+1)*stride + j])/2;
			}
		}
		else
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = pSrc[i*stride + j];
			}
		}
	}

	//	Process V
	pSrc = (uint8_t*) pInBuf->crVirAddr;
	pDst = (uint8_t*) pOutBuf->crVirAddr;
	for( i=0 ; i < height ; i++ )
	{
		if( i&1 )
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = (pSrc[(i-1)*stride + j] + pSrc[(i+1)*stride + j])/2;
			}
		}
		else
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = pSrc[i*stride + j];
			}
		}
	}

}
#else	//	FFMPEG Algorithm
void _SWDeinterlace(NX_VID_MEMORY_INFO *pInBuf, NX_VID_MEMORY_INFO *pOutBuf)
{
	int32_t i, j;
	uint8_t *pSrc;
	uint8_t *pDst;

	//	Process Y
	pSrc = (uint8_t*) pInBuf->luVirAddr;
	pDst = (uint8_t*) pOutBuf->luVirAddr;

	int32_t width = pInBuf->imgWidth;
	int32_t height = pInBuf->imgHeight;
	int32_t stride = pInBuf->luStride;
	int32_t target = 0;

	for( i=1 ; i < height-1 ; i++ )
	{
		if( i&1 )
		{
			for( j=0 ; j<width ; j++ )
			{
				target = pSrc[i*stride + j]/4 + pSrc[(i-1)*stride + j]/2 + pSrc[(i+1)*stride + j]/2 - pSrc[(i-2)*stride + j]/8 - pSrc[(i+2)*stride + j]/8;
				if( target < 0   ) target = 0;
				if( target > 255 ) target = 255;
				pDst[i*stride + j] = (uint8_t)target;
			}
		}
		else
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = pSrc[i*stride + j];
			}
		}
	}

	width /= 2;
	height /= 2;
	stride = pInBuf->cbStride;

	//	Process U
	pSrc = (uint8_t*) pInBuf->cbVirAddr;
	pDst = (uint8_t*) pOutBuf->cbVirAddr;
	for( i=0 ; i < height-1 ; i++ )
	{
		if( i&1 )
		{
			for( j=0 ; j<width ; j++ )
			{
				target = pSrc[i*stride + j]/4 + pSrc[(i-1)*stride + j]/2 + pSrc[(i+1)*stride + j]/2 - pSrc[(i-2)*stride + j]/8 - pSrc[(i+2)*stride + j]/8;
				if( target < 0   ) target = 0;
				if( target > 255 ) target = 255;
				pDst[i*stride + j] = (uint8_t)target;
			}
		}
		else
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = pSrc[i*stride + j];
			}
		}
	}

	//	Process V
	pSrc = (uint8_t*) pInBuf->crVirAddr;
	pDst = (uint8_t*) pOutBuf->crVirAddr;
	for( i=0 ; i < height-1 ; i++ )
	{
		if( i&1 )
		{
			for( j=0 ; j<width ; j++ )
			{
				target = pSrc[i*stride + j]/4 + pSrc[(i-1)*stride + j]/2 + pSrc[(i+1)*stride + j]/2 - pSrc[(i-2)*stride + j]/8 - pSrc[(i+2)*stride + j]/8;
				if( target < 0   ) target = 0;
				if( target > 255 ) target = 255;
				pDst[i*stride + j] = (uint8_t)target;
			}
		}
		else
		{
			for( j=0 ; j<width ; j++ )
			{
				pDst[i*stride + j] = pSrc[i*stride + j];
			}
		}
	}

}
#endif
