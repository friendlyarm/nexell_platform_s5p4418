#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <linux/v4l2-subdev.h>
#include <mach/nxp-scaler.h>

#include "libnxscaler.h"

//struct nxp_scaler_ioctl_data {
//	uint32_t src_phys[3];
//	uint32_t src_stride[3];
//	uint32_t src_width;
//	uint32_t src_height;
//	uint32_t src_code;			//	pixel code
//	uint32_t dst_phys[3];
//	uint32_t dst_stride[3];
//	uint32_t dst_width;
//	uint32_t dst_height;
//	uint32_t dst_code;			//	pixel code
//};

#define	SCALER_DEVICE_NAME	"/dev/nxp-scaler"

NX_SCALER_HANDLE NX_SCLOpen( void )
{
	int32_t fd;

	fd = open(SCALER_DEVICE_NAME, O_RDWR);
	if( fd < 0 )
	{
		printf("Cannot open %s device.\n", SCALER_DEVICE_NAME);
	}

	return fd;
}

int32_t NX_SCLScaleImage( NX_SCALER_HANDLE handle, NX_VID_MEMORY_INFO *pInMem, NX_VID_MEMORY_INFO *pOutMem )
{
	struct nxp_scaler_ioctl_data data;
	memset( &data, 0, sizeof(struct nxp_scaler_ioctl_data) );

	if( handle < 0 )
	{
		printf("Invalid Handle\n");
		return -1;
	}

	data.src_phys[0]   = pInMem->luPhyAddr;
	data.src_phys[1]   = pInMem->cbPhyAddr;
	data.src_phys[2]   = pInMem->crPhyAddr;
	data.src_stride[0] = pInMem->luStride;
	data.src_stride[1] = pInMem->cbStride;
	data.src_stride[2] = pInMem->crStride;
	data.src_width     = pInMem->imgWidth;
	data.src_height    = pInMem->imgHeight;
	data.src_code      = V4L2_MBUS_FMT_YUYV8_1_5X8;

	data.dst_phys[0]   = pOutMem->luPhyAddr;
	data.dst_phys[1]   = pOutMem->cbPhyAddr;
	data.dst_phys[2]   = pOutMem->crPhyAddr;
	data.dst_stride[0] = pOutMem->luStride;
	data.dst_stride[1] = pOutMem->cbStride;
	data.dst_stride[2] = pOutMem->crStride;
	data.dst_width     = pOutMem->imgWidth;
	data.dst_height    = pOutMem->imgHeight;
	data.dst_code      = V4L2_MBUS_FMT_YUYV8_1_5X8;

	return ioctl( handle, IOCTL_SCALER_SET_AND_RUN, &data );
}

void NX_SCLClose( NX_SCALER_HANDLE handle )
{
	if( handle > 0 )
	{
		close( handle );
	}
}
