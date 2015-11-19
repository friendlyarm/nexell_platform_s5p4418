#ifndef __LIBNXSCALER__
#define	__LIBNXSCALER__

#include <stdint.h>
#include <stdbool.h>
#include <nx_alloc_mem.h>

typedef int32_t		NX_SCALER_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

//
//	Description:
//		Open Fine Scaler.
//	Parameters:
//		None.
//	Return value:
//		If success returns positive value, otherwise nagetive value.
//
NX_SCALER_HANDLE NX_SCLOpen( void );


//
//	Description:
//		Open Fine Scaler.
//	Parameters:
//		NX_SCALER_HANDLE handle
//			Fine scaler handler.
//		NX_VID_MEMORY_INFO pInMem
//			Input image memory.
//		NX_VID_MEMORY_INFO pOutMem
//			Output image memory.
//	Return value:
//		If success returns 0, otherwise nagetive value.
//
//	Note:
//		Currently support format.
//			==> planar 420 --> planar 420
//
int32_t NX_SCLScaleImage( NX_SCALER_HANDLE handle, NX_VID_MEMORY_INFO *pInMem, NX_VID_MEMORY_INFO *pOutMem );


//
//	Description:
//		Close fine scaler instance.
//	Parameter:
//		Scaler Handler
//	Return Value:
//		None.
//
void NX_SCLClose( NX_SCALER_HANDLE handle );

#ifdef __cplusplus
}
#endif	

#endif	//	__LIBNXSCALER__
