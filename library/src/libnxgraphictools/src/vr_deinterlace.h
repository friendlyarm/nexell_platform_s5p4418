#ifndef __VR_DEINTERACE__
#define __VR_DEINTERACE__

#include <nx_alloc_mem.h>
#include "vr_common_def.h"

extern const char deinterace_vertex_shader[];
extern const char deinterace_frag_shader[];
extern const char scaler_vertex_shader[];
extern const char scaler_frag_shader[];
extern const char cvt2y_vertex_shader[];
extern const char cvt2y_frag_shader[];
extern const char cvt2uv_vertex_shader[];
extern const char cvt2uv_frag_shader[];
extern const char cvt2rgba_vertex_shader[];
extern const char cvt2rgba_frag_shader[];

typedef struct vrSurfaceTarget* HSURFTARGET;
typedef struct vrSurfaceSource* HSURFSOURCE;

int         vrInitializeGLSurface    ( void );

/*
	input   : Y, Y4개를 한꺼번에
	output : Y
*/
#ifdef VR_FEATURE_SEPERATE_FD_USE
HSURFTARGET vrCreateDeinterlaceTarget  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault );
HSURFSOURCE vrCreateDeinterlaceSource  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData);
void        vrRunDeinterlace           ( HSURFTARGET hTarget, HSURFSOURCE hSource);
void        vrDestroyDeinterlaceTarget ( HSURFTARGET hTarget, int iIsDefault );
void        vrDestroyDeinterlaceSource ( HSURFSOURCE hSource );
#else
HSURFTARGET vrCreateDeinterlaceTarget  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault );
HSURFSOURCE vrCreateDeinterlaceSource  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData);
void        vrRunDeinterlace           ( HSURFTARGET hTarget, HSURFSOURCE hSource);
void        vrDestroyDeinterlaceTarget ( HSURFTARGET hTarget, int iIsDefault );
void        vrDestroyDeinterlaceSource ( HSURFSOURCE hSource );
#endif

/*
	input   : YUV
	output : YUV
*/
#ifdef VR_FEATURE_SEPERATE_FD_USE
int 	    vrCreateScaleTarget  ( HSURFTARGET* ptarget, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE* pData, int iIsDefault);
int         vrCreateScaleSource  ( HSURFSOURCE* psource, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE* pData );
void        vrRunScale           	( HSURFTARGET* ptarget, HSURFSOURCE* psource );
void        vrDestroyScaleTarget ( HSURFTARGET* ptarget, int iIsDefault );
void        vrDestroyScaleSource ( HSURFSOURCE* psource );
#else
HSURFTARGET vrCreateScaleTarget  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault );
HSURFSOURCE vrCreateScaleSource  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData );
void        vrRunScale           	      ( HSURFTARGET hTarget, HSURFSOURCE hSource );
void        vrDestroyScaleTarget ( HSURFTARGET hTarget, int iIsDefault );
void        vrDestroyScaleSource ( HSURFSOURCE hSource );
#endif

#ifdef VR_FEATURE_SEPERATE_FD_USE
HSURFTARGET vrCreateCvt2YTarget  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault);
HSURFSOURCE vrCreateCvt2YSource  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData);
void        vrRunCvt2Y           	      ( HSURFTARGET hTarget, HSURFSOURCE hSource);
void        vrDestroyCvt2YTarget ( HSURFTARGET hTarget, int iIsDefault );
void        vrDestroyCvt2YSource ( HSURFSOURCE hSource );

HSURFTARGET vrCreateCvt2UVTarget  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault);
HSURFSOURCE vrCreateCvt2UVSource  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData);
void        vrRunCvt2UV           	      ( HSURFTARGET hTarget, HSURFSOURCE hSource);
void        vrDestroyCvt2UVTarget ( HSURFTARGET hTarget, int iIsDefault );
void        vrDestroyCvt2UVSource ( HSURFSOURCE hSource );
#else
HSURFTARGET vrCreateCvt2YuvTarget  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault);
HSURFSOURCE vrCreateCvt2YuvSource  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData);
void        vrRunCvt2Yuv           	      ( HSURFTARGET hTarget, HSURFSOURCE hSource);
void        vrDestroyCvt2YuvTarget ( HSURFTARGET hTarget, int iIsDefault );
void        vrDestroyCvt2YuvSource ( HSURFSOURCE hSource );
#endif

#ifdef VR_FEATURE_SEPERATE_FD_USE
HSURFTARGET vrCreateCvt2RgbaTarget  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault);
HSURFSOURCE vrCreateCvt2RgbaSource  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE DataY , NX_MEMORY_HANDLE DataU, NX_MEMORY_HANDLE DataV );
#else
HSURFTARGET vrCreateCvt2RgbaTarget  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault);
HSURFSOURCE vrCreateCvt2RgbaSource  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData);
#endif
void        vrRunCvt2Rgba           	( HSURFTARGET hTarget, HSURFSOURCE hSource);
void        vrDestroyCvt2RgbaTarget ( HSURFTARGET hTarget, int iIsDefault );
void        vrDestroyCvt2RgbaSource ( HSURFSOURCE hSource );

void        vrWaitForDone      ( void );
void        vrTerminateGLSurface     ( void );

#endif  /* __VR_DEINTERACE__ */

