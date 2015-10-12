#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "vr_common_def.h"
#include "vr_deinterlace_private.h"
#include "vr_egl_runtime.h"

#include "dbgmsg.h"

static bool    s_Initialized = false;
static Statics s_Statics;
Statics* vrGetStatics( void )
{
	return &s_Statics;
}

namespace 
{
	class AutoBackupCurrentEGL 
	{
	public:
		EGLContext eglCurrentContext;
		EGLSurface eglCurrentSurface[2];
		EGLDisplay eglCurrentDisplay;
		AutoBackupCurrentEGL(void)
		{
			eglCurrentContext    = eglGetCurrentContext();
			eglCurrentSurface[0] = eglGetCurrentSurface(EGL_DRAW);
			eglCurrentSurface[1] = eglGetCurrentSurface(EGL_READ);
			eglCurrentDisplay    = eglGetCurrentDisplay();	
		}
		~AutoBackupCurrentEGL()
		{
			//????
			//eglMakeCurrent(eglCurrentDisplay, eglCurrentSurface[0], eglCurrentSurface[1], eglCurrentContext);
		}
	};
	#define _AUTO_BACKUP_CURRENT_EGL_ AutoBackupCurrentEGL instanceAutoBackupCurrentEGL
};

static int vrInitializeDeinterlace( HSURFTARGET hTarget );
static int vrInitializeScaler( HSURFTARGET hTarget );
static int vrInitializeCvt2Y( HSURFTARGET hTarget );
static int vrInitializeCvt2UV( HSURFTARGET hTarget );
static int vrInitializeCvt2Rgba( HSURFTARGET hTarget );
static int vrDeinitializeDeinterlace( HSURFTARGET hTarget );
static int vrDeinitializeScaler( HSURFTARGET hTarget );
static int vrDeinitializeCvt2Y( HSURFTARGET hTarget );
static int vrDeinitializeCvt2UV( HSURFTARGET hTarget );
static int vrDeinitializeCvt2Rgba( HSURFTARGET hTarget );
#ifdef VR_FEATURE_SHADER_FILE_USE
static char *loadShader( const char *sFilename);
#endif
static int processShader(GLuint *pShader, const char *sFilename, GLint iShaderType);
static HSURFTARGET vrCreateCvt2YTargetDefault  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData);
static HSURFTARGET vrCreateCvt2UVTargetDefault  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData);


int VR_GRAPHIC_DBG_OPEN_CLOSE = 0;
int VR_GRAPHIC_DBG_TARGET = 0;
int VR_GRAPHIC_DBG_SOURCE = 0;
int VR_GRAPHIC_DBG_CTX = 0;
int VR_GRAPHIC_DBG_RUN = 0;
int VR_GRAPHIC_DBG_HEIGHT_ALIGN = 0;

#ifdef VR_FEATURE_SEPERATE_FD_USE
	#if 1 //only for scaler(because this usage is handle pointer case)
	#define VR_YUV_CNT  3
	#else
	#define VR_YUV_CNT  1
	#endif
#else
	#define VR_YUV_CNT 	3 
	#define VR_Y_UV_CNT 2
#endif
static int vrInitializeEGLSurface( void )
{
	int ret;
	Statics* pStatics = &s_Statics;
	if( !pStatics ){ return -1; }

	MEMSET(pStatics, 0x0, sizeof(s_Statics));

	VR_INFO("\n", VR_GRAPHIC_DBG_OPEN_CLOSE, "vrInitializeEGLSurface start <===\n"); 	
	ret = vrInitializeEGLConfig();
	if( ret ){ ErrMsg("Error: vrInitializeEGLSurface at %s:%i\n", __FILE__, __LINE__); return ret; }

	pStatics->default_target_memory[VR_PROGRAM_DEINTERLACE] = NX_AllocateMemory(64*64, 4);		
	pStatics->default_target_memory[VR_PROGRAM_SCALE] = NX_AllocateMemory(64*64, 4);		
	pStatics->default_target_memory[VR_PROGRAM_CVT2Y] = NX_AllocateMemory(64*64*4, 4);		
	pStatics->default_target_memory[VR_PROGRAM_CVT2UV] = NX_AllocateMemory(64*64*4, 4);		
	pStatics->default_target_memory[VR_PROGRAM_CVT2RGBA] = NX_AllocateMemory(64*64*4, 4);		

	VR_INFO("", VR_GRAPHIC_DBG_OPEN_CLOSE, "vrInitializeEGLSurface end\n"); 	

	return 0;
}

int vrInitializeGLSurface( void )
{
	int ret;
	if( s_Initialized )
	{ 
		//ErrMsg("Error: vrInitializeEGLSurface at %s:%i\n", __FILE__, __LINE__); 
		return 0; 
	}
	
	ret = vrInitializeEGLSurface();
	if( ret ){ ErrMsg("Error: vrInitializeGLSurface at %s:%i\n", __FILE__, __LINE__); return ret; }

	Statics* pStatics = &s_Statics;
	if( !pStatics ){ return -1; }
		
	VR_INFO("", VR_GRAPHIC_DBG_OPEN_CLOSE, "vrInitializeGLSurface start <=\n"); 	
	ret = vrInitEglExtensions();
	if( ret ){ ErrMsg("Error: vrInitializeGLSurface at %s:%i\n", __FILE__, __LINE__); return ret; }	

	if(!pStatics->default_target[VR_PROGRAM_DEINTERLACE])
	{
		#ifdef VR_FEATURE_SEPERATE_FD_USE	
		pStatics->default_target[VR_PROGRAM_DEINTERLACE] = vrCreateDeinterlaceTarget(64, 64, pStatics->default_target_memory[VR_PROGRAM_DEINTERLACE], VR_TRUE);
		#else
		pStatics->default_target[VR_PROGRAM_DEINTERLACE] = vrCreateDeinterlaceTarget(0, 64, 64, pStatics->default_target_memory[VR_PROGRAM_DEINTERLACE], VR_TRUE);
		#endif
	}
	if(!pStatics->default_target[VR_PROGRAM_SCALE])
	{		
		#ifdef VR_FEATURE_SEPERATE_FD_USE	
		vrCreateScaleTarget(&pStatics->default_target[VR_PROGRAM_SCALE], 64, 64, &pStatics->default_target_memory[VR_PROGRAM_SCALE], VR_TRUE);
		#else
		pStatics->default_target[VR_PROGRAM_SCALE] = vrCreateScaleTarget(0, 64, 64, pStatics->default_target_memory[VR_PROGRAM_SCALE], VR_TRUE);
		#endif
	}
	#ifdef VR_FEATURE_SEPERATE_FD_USE	
	if(!pStatics->default_target[VR_PROGRAM_CVT2Y])
		pStatics->default_target[VR_PROGRAM_CVT2Y] = vrCreateCvt2YTarget(64, 64, pStatics->default_target_memory[VR_PROGRAM_CVT2Y], VR_TRUE);
	if(!pStatics->default_target[VR_PROGRAM_CVT2UV])
		pStatics->default_target[VR_PROGRAM_CVT2UV] = vrCreateCvt2UVTarget(64, 64, pStatics->default_target_memory[VR_PROGRAM_CVT2UV], VR_TRUE);
	if(!pStatics->default_target[VR_PROGRAM_CVT2RGBA])
		pStatics->default_target[VR_PROGRAM_CVT2RGBA] = vrCreateCvt2RgbaTarget(64, 64, pStatics->default_target_memory[VR_PROGRAM_CVT2RGBA], VR_TRUE);
	#else
	if(!pStatics->default_target[VR_PROGRAM_CVT2Y])
		pStatics->default_target[VR_PROGRAM_CVT2Y] = vrCreateCvt2YTargetDefault(0, 64, 64, pStatics->default_target_memory[VR_PROGRAM_CVT2Y]);
	if(!pStatics->default_target[VR_PROGRAM_CVT2UV])
		pStatics->default_target[VR_PROGRAM_CVT2UV] = vrCreateCvt2UVTargetDefault(0, 64, 64, pStatics->default_target_memory[VR_PROGRAM_CVT2UV]);
	if(!pStatics->default_target[VR_PROGRAM_CVT2RGBA])
		pStatics->default_target[VR_PROGRAM_CVT2RGBA] = vrCreateCvt2RgbaTarget(0, 64, 64, pStatics->default_target_memory[VR_PROGRAM_CVT2RGBA], VR_TRUE);
	#endif
	
	for(unsigned int program = 0 ; program < VR_PROGRAM_MAX ; program++)
	{
		pStatics->shader[program].iVertName = 0;
		pStatics->shader[program].iFragName = 0;
		pStatics->shader[program].iProgName = 0;
		pStatics->shader[program].iLocPosition = -1;
		pStatics->shader[program].iLocTexCoord[0] = -1;
		pStatics->shader[program].iLocTexCoord[1] = -1;
		pStatics->shader[program].iLocTexCoord[2] = -1;
		pStatics->shader[program].iLocInputHeight = -1;
		pStatics->shader[program].iLocOutputHeight = -1;
		pStatics->shader[program].iLocMainTex[0] = -1;
		pStatics->shader[program].iLocMainTex[1] = -1;
		pStatics->shader[program].iLocMainTex[2] = -1;
		pStatics->shader[program].iLocRefTex = -1;
	}	
 	pStatics->tex_deinterlace_ref_id = 0;

	if(vrInitializeDeinterlace( pStatics->default_target[VR_PROGRAM_DEINTERLACE] ) != 0)
		return -1;
	if(vrInitializeScaler(pStatics->default_target[VR_PROGRAM_SCALE] ) != 0)
		return -1;
	if(vrInitializeCvt2Y(pStatics->default_target[VR_PROGRAM_CVT2Y] ) != 0)
		return -1;
	if(vrInitializeCvt2UV(pStatics->default_target[VR_PROGRAM_CVT2UV] ) != 0)
		return -1;
	if(vrInitializeCvt2Rgba(pStatics->default_target[VR_PROGRAM_CVT2RGBA] ) != 0)
		return -1;
		
	s_Initialized = true;

	VR_INFO("", VR_GRAPHIC_DBG_OPEN_CLOSE, "vrInitializeGLSurface end\n"); 	
	return 0;
}

static void  vrTerminateEGLSurface( void )
{
	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return; }
	NX_FreeMemory( pStatics->default_target_memory[VR_PROGRAM_DEINTERLACE] );
	NX_FreeMemory( pStatics->default_target_memory[VR_PROGRAM_SCALE] );
	NX_FreeMemory( pStatics->default_target_memory[VR_PROGRAM_CVT2Y] );
	NX_FreeMemory( pStatics->default_target_memory[VR_PROGRAM_CVT2UV] );
	NX_FreeMemory( pStatics->default_target_memory[VR_PROGRAM_CVT2RGBA] );
	
	vrTerminateEGL();

	VR_INFO("", VR_GRAPHIC_DBG_OPEN_CLOSE, "vrTerminateEGLSurface done ===>\n"); 
}

void  vrTerminateGLSurface( void )
{	
	if( ! s_Initialized ){ return; }

	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return; }

	VR_INFO("", VR_GRAPHIC_DBG_OPEN_CLOSE, "vrTerminateGLSurface start\n"); 	

	if(vrDeinitializeDeinterlace(pStatics->default_target[VR_PROGRAM_DEINTERLACE]) != 0)
		ErrMsg("Error: vrDeinitializeDeinterlace() %s:%i\n", __FILE__, __LINE__);
	if(vrDeinitializeScaler(pStatics->default_target[VR_PROGRAM_SCALE]) != 0)
		ErrMsg("Error: vrDeinitializeScaler() %s:%i\n", __FILE__, __LINE__);
	if(vrDeinitializeCvt2Y(pStatics->default_target[VR_PROGRAM_CVT2Y]) != 0)
		ErrMsg("Error: vrDeinitializeCvt2Y() %s:%i\n", __FILE__, __LINE__);
	if(vrDeinitializeCvt2UV(pStatics->default_target[VR_PROGRAM_CVT2UV]) != 0)
		ErrMsg("Error: vrDeinitializeCvt2UV() %s:%i\n", __FILE__, __LINE__);
	if(vrDeinitializeCvt2Rgba(pStatics->default_target[VR_PROGRAM_CVT2RGBA]) != 0)
		ErrMsg("Error: vrDeinitializeCvt2Rgba() %s:%i\n", __FILE__, __LINE__);

	for(int i = 0 ; i < VR_PROGRAM_MAX ; i++)
	{
		if(pStatics->egl_info.sEGLContext[i])
			ErrMsg("ERROR: vrTerminateGLSurface(0x%x), idx(%d), ref(%d)\n", (int)pStatics->egl_info.sEGLContext[i], i, pStatics->egl_info.sEGLContextRef[i]);
		VR_ASSERT("ctx_ref must be zero", !pStatics->egl_info.sEGLContextRef[i]);
	}
	
	if(pStatics->default_target[VR_PROGRAM_DEINTERLACE])	
		vrDestroyDeinterlaceTarget( pStatics->default_target[VR_PROGRAM_DEINTERLACE], VR_TRUE );
	if(pStatics->default_target[VR_PROGRAM_SCALE])	
	{	
		#ifdef VR_FEATURE_SEPERATE_FD_USE
		vrDestroyScaleTarget( &pStatics->default_target[VR_PROGRAM_SCALE], VR_TRUE );
		#else
		vrDestroyScaleTarget( pStatics->default_target[VR_PROGRAM_SCALE], VR_TRUE );
		#endif
	}
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	if(pStatics->default_target[VR_PROGRAM_CVT2Y])	
		vrDestroyCvt2YTarget( pStatics->default_target[VR_PROGRAM_CVT2Y], VR_TRUE );
	if(pStatics->default_target[VR_PROGRAM_CVT2UV])	
		vrDestroyCvt2UVTarget( pStatics->default_target[VR_PROGRAM_CVT2UV], VR_TRUE );
	#else
	if(pStatics->default_target[VR_PROGRAM_CVT2Y])	
		vrDestroyCvt2YuvTarget( pStatics->default_target[VR_PROGRAM_CVT2Y], VR_TRUE );
	if(pStatics->default_target[VR_PROGRAM_CVT2UV])	
		vrDestroyCvt2YuvTarget( pStatics->default_target[VR_PROGRAM_CVT2UV], VR_TRUE );
	#endif	
	if(pStatics->default_target[VR_PROGRAM_CVT2RGBA])	
		vrDestroyCvt2RgbaTarget( pStatics->default_target[VR_PROGRAM_CVT2RGBA], VR_TRUE );

	pStatics->default_target[VR_PROGRAM_DEINTERLACE] = NULL;
	pStatics->default_target[VR_PROGRAM_SCALE] = NULL;
	pStatics->default_target[VR_PROGRAM_CVT2Y] = NULL;
	pStatics->default_target[VR_PROGRAM_CVT2UV] = NULL;
	pStatics->default_target[VR_PROGRAM_CVT2RGBA] = NULL;	
	VR_INFO("", VR_GRAPHIC_DBG_OPEN_CLOSE, "vrTerminateGLSurface end =>\n"); 

	vrTerminateEGLSurface();

	s_Initialized = false;	
}

struct vrSurfaceTarget
{
#ifdef VR_FEATURE_STRIDE_USE
	unsigned int		stride;
#endif
	unsigned int        width;
	unsigned int        height;
#ifdef VR_FEATURE_SEPERATE_FD_USE	
	EGLNativePixmapType target_native_pixmap;
	EGLSurface			target_pixmap_surface;
#else
	EGLNativePixmapType target_native_pixmaps[VR_INPUT_MODE_YUV_MAX];
	EGLSurface			target_pixmap_surfaces[VR_INPUT_MODE_YUV_MAX];
#endif
};

struct vrSurfaceSource
{
#ifdef VR_FEATURE_STRIDE_USE
	unsigned int		stride;
#endif
	unsigned int        width        ;
	unsigned int        height       ;
	unsigned int        total_texture_src_height[VR_INPUT_MODE_YUV_MAX];
	GLuint              texture_name[VR_INPUT_MODE_YUV_MAX] ;
	/* 
	case VR_FEATURE_SEPERATE_FD_USE => use Y,U,V
	*/	
	EGLNativePixmapType src_native_pixmaps[VR_INPUT_MODE_YUV_MAX];
	EGLImageKHR         src_pixmap_images[VR_INPUT_MODE_YUV_MAX];
};

static void vrDestroySurfaceTarget ( HSURFTARGET hTarget)
{
	Statics *pStatics = vrGetStatics();
#ifdef VR_FEATURE_SEPERATE_FD_USE	
	if( hTarget->target_pixmap_surface ) { EGL_CHECK(eglDestroySurface(pStatics->egl_info.sEGLDisplay,hTarget->target_pixmap_surface)); }
	if( hTarget->target_native_pixmap  ) { vrDestroyPixmap(hTarget->target_native_pixmap); }
#else
	for(int i = 0 ; i < VR_INPUT_MODE_YUV_MAX ; i++)
	{		
		if( hTarget->target_pixmap_surfaces[i] ) { EGL_CHECK(eglDestroySurface(pStatics->egl_info.sEGLDisplay,hTarget->target_pixmap_surfaces[i])); }
		if( hTarget->target_native_pixmaps[i]  ) { vrDestroyPixmap(hTarget->target_native_pixmaps[i]); }
	}
#endif	
	FREE(hTarget);
}

static void vrResetShaderInfo(Shader* pShader)
{
	pShader->iVertName = -1;
	pShader->iFragName = -1;
	pShader->iProgName = -1;
}

#ifdef VR_FEATURE_SEPERATE_FD_USE
HSURFTARGET vrCreateDeinterlaceTarget  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault )
{
	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFTARGET)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );

	/* Create a EGLNativePixmapType. */
	/* Y4개를 한꺼번에*/
	EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(uiWidth/4, uiHeight, hData, VR_TRUE, 32, VR_IMAGE_FORMAT_Y/*VR_TRUE*/);
	if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE( result );
		return (HSURFTARGET)0;
	}

	/* Create a EGLSurface. */
	EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_DEINTERLACE], (EGLNativePixmapType)pixmap_output, NULL);
	if(surface == EGL_NO_SURFACE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
		vrDestroyPixmap(pixmap_output);
		FREE( result );
		return (HSURFTARGET)0;
	}

	result->width          = uiWidth ;
	result->height         = uiHeight;
	#ifdef VR_FEATURE_SEPERATE_FD_USE	
	result->target_pixmap_surface = surface;	
	result->target_native_pixmap  = pixmap_output;
	#else
	result->target_pixmap_surfaces[VR_INPUT_MODE_Y] = surface;	
	result->target_native_pixmaps[VR_INPUT_MODE_Y]  = pixmap_output;
	#endif
	/* Increase Ctx ref. */
	if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE];
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateDeinterlaceTarget, 32ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE]); 	
	
	return result;
}

void vrDestroyDeinterlaceTarget ( HSURFTARGET hTarget, int iIsDefault )
{
	if( !hTarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;

	vrDestroySurfaceTarget(hTarget);
	
	/* Decrease Ctx ref. */
	if(!iIsDefault) 
	{
		VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE] > 0);
		--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE];
	}
	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyDeinterlaceTarget done, 32ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE]); 			
}

int vrCreateScaleTarget  (HSURFTARGET* ptarget, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE* pData, int iIsDefault )
{
	Statics *pStatics = vrGetStatics();
	if( !pStatics || !pData || !pData[0] ){ return VR_FALSE; }
	_AUTO_BACKUP_CURRENT_EGL_;

	VRImageFormatMode format[3] = { VR_IMAGE_FORMAT_Y, VR_IMAGE_FORMAT_U, VR_IMAGE_FORMAT_V };
	
	unsigned int pixmap_width, pixmap_height;
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{
		if(!pData[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);
			continue;
		}
		
		HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
		if( !result )
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
			return VR_FALSE;
		}
		MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );

		if(0 == i)
		{
			//Y case
			pixmap_width = uiWidth;
			pixmap_height = uiHeight;
		}
		else if(1 == i)
		{
			//U case
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight/2;
		}
		else
		{
			//V case
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight/2;
		}

		/* Create a EGLNativePixmapType. */
		EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(pixmap_width, pixmap_height, pData[i], VR_TRUE, 8, format[i]);
		if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
			FREE( result );
			return VR_FALSE;
		}
		
		/* Create a EGLSurface. */
		EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_SCALE], (EGLNativePixmapType)pixmap_output, NULL);
		if(surface == EGL_NO_SURFACE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
			vrDestroyPixmap(pixmap_output);
			FREE( result );
			return VR_FALSE;
		}

		result->width          = pixmap_width ;
		result->height         = pixmap_height;
		result->target_native_pixmap  = pixmap_output;
		result->target_pixmap_surface = surface;	
		ptarget[i] = result;

		if(iIsDefault)
		{ 								
			break; 
		}
	}
	
	/* Increase Ctx ref. */
	if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE];
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateScaleTarget, 8ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE]); 	
	
	return VR_TRUE;
}

void vrDestroyScaleTarget ( HSURFTARGET* ptarget, int iIsDefault )
{
	if( !ptarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;

	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{		
		if(!ptarget[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);		
			continue;
		}			
		vrDestroySurfaceTarget(ptarget[i]);
		
		if(iIsDefault){ break; }
	}
	
	/* Decrease Ctx ref. */
	if(!iIsDefault)
	{
		VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE] > 0);
		--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE];
	}
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyScaleTarget done, 8ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE]); 		
}

HSURFTARGET vrCreateCvt2YTarget  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault)
{
	//ErrMsg("vrCreateCvt2YTarget start(%dx%d)\n", uiWidth, uiHeight);

	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFTARGET)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );

	/* Create a EGLNativePixmapType. */
	EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(uiWidth, uiHeight, hData, VR_TRUE, 8, VR_IMAGE_FORMAT_Y/*VR_TRUE*/);
	if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE( result );
		return (HSURFTARGET)0;
	}
	
	/* Create a EGLSurface. */
	EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_CVT2Y], (EGLNativePixmapType)pixmap_output, NULL);
	if(surface == EGL_NO_SURFACE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
		vrDestroyPixmap(pixmap_output);
		FREE( result );
		return (HSURFTARGET)0;
	}

	result->width          = uiWidth ;
	result->height         = uiHeight;
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	result->target_native_pixmap  = pixmap_output;
	result->target_pixmap_surface = surface;	
	#else
	result->target_native_pixmaps[VR_INPUT_MODE_Y]  = pixmap_output;
	result->target_pixmap_surfaces[VR_INPUT_MODE_Y] = surface;	
	#endif
	
	/* Increase Ctx ref. */
	if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2Y];
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateCvt2YTarget, 8ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2Y]); 	
	
	return result;
}

void vrDestroyCvt2YTarget ( HSURFTARGET hTarget, int iIsDefault )
{
	if( !hTarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
	
	vrDestroySurfaceTarget(hTarget);

	/* Decrease Ctx ref. */
	if(!iIsDefault)
	{
		VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2Y] > 0);
		--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2Y];
	}	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyCvt2YTarget done, 8ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2Y]); 		
}

HSURFTARGET vrCreateCvt2UVTarget  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault)
{
	//ErrMsg("vrCreateCvt2UVTarget start(%dx%d)\n", uiWidth, uiHeight);

	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFTARGET)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );

	//4pixel마다 UV존재
	uiWidth /= 2;
	uiHeight /= 2;
	
	/* Create a EGLNativePixmapType. */
	EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(uiWidth, uiHeight, hData, VR_TRUE, 16, VR_IMAGE_FORMAT_UV/*VR_TRUE*/);
	if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE( result );
		return (HSURFTARGET)0;
	}
	
	/* Create a EGLSurface. */
	EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_CVT2UV], (EGLNativePixmapType)pixmap_output, NULL);
	if(surface == EGL_NO_SURFACE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
		vrDestroyPixmap(pixmap_output);
		FREE( result );
		return (HSURFTARGET)0;
	}

	result->width          = uiWidth ;
	result->height         = uiHeight;
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	result->target_native_pixmap  = pixmap_output;
	result->target_pixmap_surface = surface;	
	#else
	if(iIsDefault)
	{
		result->target_native_pixmaps[0]  = pixmap_output;
		result->target_pixmap_surfaces[0] = surface;	
	}
	else
	{
		result->target_native_pixmaps[VR_INPUT_MODE_U]  = pixmap_output;
		result->target_pixmap_surfaces[VR_INPUT_MODE_U] = surface;	
	}
	#endif

	/* Increase Ctx ref. */
	if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV];
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateCvt2UVTarget, 16ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV]); 		
	
	return result;
}

void vrDestroyCvt2UVTarget ( HSURFTARGET hTarget, int iIsDefault )
{
	if( !hTarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
	
	vrDestroySurfaceTarget(hTarget);

	/* Decrease Ctx ref. */
	if(!iIsDefault)
	{
		VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV] > 0);
		--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV];		
	}	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyCvt2UVTarget done, 16ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV]); 		
}

HSURFTARGET vrCreateCvt2RgbaTarget  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault)
{
	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFTARGET)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );

	/* Create a EGLNativePixmapType. */
	EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(uiWidth, uiHeight, hData, VR_TRUE, 32, VR_IMAGE_FORMAT_RGBA);
	if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE( result );
		return (HSURFTARGET)0;
	}
	
	/* Create a EGLSurface. */
	EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_CVT2RGBA], (EGLNativePixmapType)pixmap_output, NULL);
	if(surface == EGL_NO_SURFACE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
		vrDestroyPixmap(pixmap_output);
		FREE( result );
		return (HSURFTARGET)0;
	}

	result->width          = uiWidth ;
	result->height         = uiHeight;
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	result->target_native_pixmap  = pixmap_output;
	result->target_pixmap_surface = surface;	
	#else
	result->target_native_pixmaps[0]  = pixmap_output;
	result->target_pixmap_surfaces[0] = surface;	
	#endif

	/* Increase Ctx ref. */
	if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA];
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateCvt2RgbaTarget, 32ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA]); 		
	
	return result;
}

void vrDestroyCvt2RgbaTarget ( HSURFTARGET hTarget, int iIsDefault )
{
	if( !hTarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
		
	vrDestroySurfaceTarget(hTarget);

	/* Decrease Ctx ref. */
	if(!iIsDefault)
	{
		VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA] > 0);
		--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA];
	}
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyCvt2RgbaTarget done, 32ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA]); 			
}

#else

HSURFTARGET vrCreateDeinterlaceTarget  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault)
{
	VR_ASSERT("width must be 8X and height must be 2X", uiWidth && uiHeight && !(uiWidth % 8) && ((uiHeight&0x1) == 0x0) );
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "%s start\n", __FUNCTION__);

	Statics *pStatics = vrGetStatics();
	if( !pStatics || !hData ){ return VR_FALSE; }
	_AUTO_BACKUP_CURRENT_EGL_;

	VRImageFormatMode format[3] = { VR_IMAGE_FORMAT_V, VR_IMAGE_FORMAT_U, VR_IMAGE_FORMAT_Y };
	
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );
	result->width		   = uiWidth ;
	result->height		   = uiHeight;
	#ifdef VR_FEATURE_STRIDE_USE
	result->stride		   = uiStride;
	#endif

	unsigned int pixmap_stride, pixmap_width, pixmap_height;
	if(!uiStride){ uiStride = uiWidth; }
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{
		if(2 == i)
		{
			//Y case
			pixmap_stride = uiStride;
			pixmap_width = uiWidth;
			pixmap_height = uiHeight;
		}
		else if(1 == i)
		{
			//U case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight/2;			
			#if (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				//add Y align blank offset
				unsigned int y_height, y_align_blank_height = 0;
				y_height = uiHeight;
				if(y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				pixmap_height += y_align_blank_height;
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d)\n", i, y_align_blank_height);		
			}
			#endif
		}
		else
		{
			//V case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight;
			#if (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				unsigned int y_height, y_align_blank_height = 0;
				unsigned int u_height, u_align_blank_height = 0;
				//add Y align blank offset
				y_height = uiHeight;
				if(y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				//add U align blank offset
				u_height = uiHeight/2;
				if(u_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)
				{	
					u_align_blank_height = (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE - (u_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE));
				}
				pixmap_height += (y_align_blank_height + u_align_blank_height);
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d), u_align_blank_height(%d)\n", i, y_align_blank_height, u_align_blank_height);		
			}
			#endif			
		}
		
		/* Create a EGLNativePixmapType. */
		/* input Y 4개를 한꺼번에*/
		#ifdef VR_FEATURE_STRIDE_USE
		pixmap_width = pixmap_stride;
		#endif
		EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(pixmap_width/4, pixmap_height, hData, VR_TRUE, 32, format[i]);
		if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
			FREE( result );
			return (HSURFTARGET)0;
		}
		
		/* Create a EGLSurface. */
		EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_DEINTERLACE], (EGLNativePixmapType)pixmap_output, NULL);
		if(surface == EGL_NO_SURFACE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
			vrDestroyPixmap(pixmap_output);
			FREE( result );
			return (HSURFTARGET)0;
		}

		result->target_native_pixmaps[i]  = pixmap_output;
		result->target_pixmap_surfaces[i] = surface;	

		VR_INFO("", VR_GRAPHIC_DBG_TARGET, "size(%dx%d, %d)\n", pixmap_width, pixmap_height, pixmap_stride);		

		if(iIsDefault)
		{
			break; 
		}		
	}
	/* Increase Ctx ref. */
	if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE];
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateDeinterlaceTarget, 32ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE]); 	
	
	return result;
}

void vrDestroyDeinterlaceTarget ( HSURFTARGET hTarget, int iIsDefault )
{
	if( !hTarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
			
	vrDestroySurfaceTarget(hTarget);

	/* Decrease Ctx ref. */
	if(!iIsDefault)
	{
		VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE] > 0);
		--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE];
	}
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyDeinterlaceTarget done, 32ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_DEINTERLACE]); 		
}

HSURFTARGET vrCreateScaleTarget  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault)
{
	VR_ASSERT("width must be 2X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );

	Statics *pStatics = vrGetStatics();
	if( !pStatics || !hData ){ return VR_FALSE; }
	_AUTO_BACKUP_CURRENT_EGL_;

	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "%s start\n", __FUNCTION__);

	VRImageFormatMode format[3] = { VR_IMAGE_FORMAT_V, VR_IMAGE_FORMAT_U, VR_IMAGE_FORMAT_Y };
	
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );
	result->width		   = uiWidth ;
	result->height		   = uiHeight;
	#ifdef VR_FEATURE_STRIDE_USE
	result->stride		   = uiStride;
	#endif

	unsigned int pixmap_stride, pixmap_width, pixmap_height;
	if(!uiStride){ uiStride = uiWidth; }
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{
		if(2 == i)
		{
			//Y case
			pixmap_stride = uiStride;
			pixmap_width = uiWidth;
			pixmap_height = uiHeight;
		}
		else if(1 == i)
		{
			//U case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight/2;
			#if (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				//add Y align blank offset
				unsigned int y_height, y_align_blank_height = 0;
				y_height = uiHeight;
				if(y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				pixmap_height += y_align_blank_height;
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d)\n", i, y_align_blank_height);		
			}
			#endif
		}
		else
		{
			//V case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight;
			#if (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				unsigned int y_height, y_align_blank_height = 0;
				unsigned int u_height, u_align_blank_height = 0;
				//add Y align blank offset
				y_height = uiHeight;
				if(y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				//add U align blank offset
				u_height = uiHeight/2;
				if(u_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)
				{	
					u_align_blank_height = (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE - (u_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE));
				}
				pixmap_height += (y_align_blank_height + u_align_blank_height);
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d), u_align_blank_height(%d)\n", i, y_align_blank_height, u_align_blank_height);		
			}
			#endif
		}
		
		/* Create a EGLNativePixmapType. */
		#ifdef VR_FEATURE_STRIDE_USE
		pixmap_width = pixmap_stride;
		#endif
		EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(pixmap_width, pixmap_height, hData, VR_TRUE, 8, format[i]);
		if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
			FREE( result );
			return (HSURFTARGET)0;
		}
		
		/* Create a EGLSurface. */
		EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_SCALE], (EGLNativePixmapType)pixmap_output, NULL);
		if(surface == EGL_NO_SURFACE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
			vrDestroyPixmap(pixmap_output);
			FREE( result );
			return (HSURFTARGET)0;
		}

		result->target_native_pixmaps[i]  = pixmap_output;
		result->target_pixmap_surfaces[i] = surface;	

		VR_INFO("", VR_GRAPHIC_DBG_TARGET, "size(%dx%d, %d)\n", pixmap_width, pixmap_height, pixmap_stride);		

		if(iIsDefault)
		{
			break; 
		}		
	}
	/* Increase Ctx ref. */
	if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE];
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateScaleTarget, 8ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE]); 	
	
	return result;
}

void vrDestroyScaleTarget ( HSURFTARGET hTarget, int iIsDefault )
{
	if( !hTarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
			
	vrDestroySurfaceTarget(hTarget);

	/* Decrease Ctx ref. */
	if(!iIsDefault)
	{
		VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE] > 0);
		--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE];
	}
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyScaleTarget done, 8ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_SCALE]); 		
}

static HSURFTARGET vrCreateCvt2YTargetDefault  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	VR_ASSERT("width must be 2X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );

	//ErrMsg("vrCreateCvt2YTargetDefault start(%dx%d)\n", uiWidth, uiHeight);
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "%s start\n", __FUNCTION__);

	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFTARGET)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );

	/* Create a EGLNativePixmapType. */
	EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(uiWidth, uiHeight, hData, VR_TRUE, 8, VR_IMAGE_FORMAT_Y);
	if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE( result );
		return (HSURFTARGET)0;
	}
	
	/* Create a EGLSurface. */
	EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_CVT2Y], (EGLNativePixmapType)pixmap_output, NULL);
	if(surface == EGL_NO_SURFACE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
		vrDestroyPixmap(pixmap_output);
		FREE( result );
		return (HSURFTARGET)0;
	}

	result->width          = uiWidth ;
	result->height         = uiHeight;
	result->target_native_pixmaps[0]  = pixmap_output;
	result->target_pixmap_surfaces[0] = surface;
	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateCvt2YTargetDefault\n"); 		
	
	return result;
}

static HSURFTARGET vrCreateCvt2UVTargetDefault  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	VR_ASSERT("width must be 2X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );

	//ErrMsg("vrCreateCvt2UVTarget start(%dx%d)\n", uiWidth, uiHeight);
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "%s start\n", __FUNCTION__);

	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFTARGET)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );
	
	/* Create a EGLNativePixmapType. */
	EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(uiWidth, uiHeight, hData, VR_TRUE, 16, VR_IMAGE_FORMAT_UV);
	if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE( result );
		return (HSURFTARGET)0;
	}
	
	/* Create a EGLSurface. */
	EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_CVT2UV], (EGLNativePixmapType)pixmap_output, NULL);
	if(surface == EGL_NO_SURFACE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
		vrDestroyPixmap(pixmap_output);
		FREE( result );
		return (HSURFTARGET)0;
	}

	result->width          = uiWidth ;
	result->height         = uiHeight;
	result->target_native_pixmaps[0]  = pixmap_output;
	result->target_pixmap_surfaces[0] = surface;	

	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateCvt2UVTargetDefault\n"); 		
	
	return result;
}


HSURFTARGET vrCreateCvt2YuvTarget  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault)
{
	VR_ASSERT("width must be 8X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );

	Statics *pStatics = vrGetStatics();
	if( !pStatics || !hData ){ return VR_FALSE; }
	_AUTO_BACKUP_CURRENT_EGL_;

	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "%s start\n", __FUNCTION__);

	VRImageFormatMode format[2] = { VR_IMAGE_FORMAT_UV, VR_IMAGE_FORMAT_Y };
	unsigned int target_ctx_bits[2] = {16, 8};
	
	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );
	result->width		   = uiWidth ;
	result->height		   = uiHeight;
	#ifdef VR_FEATURE_STRIDE_USE
	result->stride		   = uiStride;
	#endif

	unsigned int pixmap_stride, pixmap_width, pixmap_height;
	if(!uiStride){ uiStride = uiWidth; }
	for(int i = 0 ; i < 2/*UV*/ ; i++)
	{
		if(1 == i)
		{
			//Y case
			pixmap_stride = uiStride;
			pixmap_width = uiWidth;
			pixmap_height = uiHeight;
		}
		else
		{
			//UV case
			pixmap_stride = uiStride/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight + uiHeight/2;
			#if (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				//add Y align blank offset
				unsigned int y_height, y_align_blank_height = 0;
				y_height = uiHeight;
				if(y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE));
				}
				pixmap_height += y_align_blank_height;
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d)\n", i, y_align_blank_height);		
			}
			#endif			
		}
		
		/* Create a EGLNativePixmapType. */
		#ifdef VR_FEATURE_STRIDE_USE
		pixmap_width = pixmap_stride;
		#endif
		EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(pixmap_width, pixmap_height, hData, VR_TRUE, target_ctx_bits[i], format[i]);
		if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
			FREE( result );
			return (HSURFTARGET)0;
		}
		
		/* Create a EGLSurface. */
		EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_CVT2UV+i], (EGLNativePixmapType)pixmap_output, NULL);
		if(surface == EGL_NO_SURFACE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
			vrDestroyPixmap(pixmap_output);
			FREE( result );
			return (HSURFTARGET)0;
		}

		result->target_native_pixmaps[i]  = pixmap_output;
		result->target_pixmap_surfaces[i] = surface;	

		VR_INFO("", VR_GRAPHIC_DBG_TARGET, "size(%dx%d, %d)\n", pixmap_width, pixmap_height, pixmap_stride);		

		if(iIsDefault)
		{
			break; 
		}		
		
		/* Increase Ctx ref. */
		if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV+i];
	}

	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateCvt2YuvTarget, 8ctx ref(%d), 16ctx ref(%d)\n", 
		pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2Y], pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV]); 	
	
	return result;
}

void vrDestroyCvt2YuvTarget ( HSURFTARGET hTarget, int iIsDefault )
{
	if( !hTarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
			
	vrDestroySurfaceTarget(hTarget);

	/* Decrease Ctx ref. */
	for(int i = 0 ; i < 2/*UV*/ ; i++)
	{
		if(!iIsDefault)
		{
			VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV+i] > 0);
			--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV+i];
		}
	}
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyScaleTarget done, 8ctx ref(%d), 16ctx ref(%d)\n", 
			pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2Y], pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2UV]); 		
}

HSURFTARGET vrCreateCvt2RgbaTarget  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData, int iIsDefault)
{
	VR_ASSERT("width must be 8X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );

	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFTARGET)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "%s start\n", __FUNCTION__);

	HSURFTARGET result = (HSURFTARGET)MALLOC(sizeof(vrSurfaceTarget));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFTARGET)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceTarget) );
	result->width          = uiWidth ;
	result->height         = uiHeight;
	#ifdef VR_FEATURE_STRIDE_USE
	result->stride		   = uiStride;
	#endif	

	/* Create a EGLNativePixmapType. */
	unsigned int pixmap_stride, pixmap_width, pixmap_height;
	if(!uiStride){ uiStride = uiWidth; }
	pixmap_stride = uiStride;
	pixmap_width = uiWidth;
	pixmap_height = uiHeight;

	/* Create a EGLNativePixmapType. */
	#ifdef VR_FEATURE_STRIDE_USE
	pixmap_width = pixmap_stride;
	#endif			
	EGLNativePixmapType pixmap_output = (fbdev_pixmap*)vrCreatePixmap(pixmap_width, pixmap_height, hData, VR_TRUE, 32, VR_IMAGE_FORMAT_RGBA);
	if(pixmap_output == NULL || ((fbdev_pixmap*)pixmap_output)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE( result );
		return (HSURFTARGET)0;
	}
	
	/* Create a EGLSurface. */
	EGLSurface surface = eglCreatePixmapSurface(pStatics->egl_info.sEGLDisplay, pStatics->egl_info.sEGLConfig[VR_PROGRAM_CVT2RGBA], (EGLNativePixmapType)pixmap_output, NULL);
	if(surface == EGL_NO_SURFACE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to create EGL surface at %s:%i\n", __FILE__, __LINE__);
		vrDestroyPixmap(pixmap_output);
		FREE( result );
		return (HSURFTARGET)0;
	}

	result->target_native_pixmaps[0]  = pixmap_output;
	result->target_pixmap_surfaces[0] = surface;	

	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "size(%dx%d, %d)\n", pixmap_width, pixmap_height, pixmap_stride);		

	/* Increase Ctx ref. */
	if(!iIsDefault) ++pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA];
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrCreateCvt2RgbaTarget, 32ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA]); 		
	
	return result;
}

void vrDestroyCvt2RgbaTarget ( HSURFTARGET hTarget, int iIsDefault )
{
	if( !hTarget ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
		
	vrDestroySurfaceTarget(hTarget);

	/* Decrease Ctx ref. */
	if(!iIsDefault)
	{
		VR_ASSERT("Ref must be greater than 0", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA] > 0);
		--pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA];
	}
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "vrDestroyCvt2RgbaTarget done, 32ctx ref(%d)\n", pStatics->egl_info.sEGLContextRef[VR_PROGRAM_CVT2RGBA]); 			
}
#endif

#ifdef VR_FEATURE_SEPERATE_FD_USE
HSURFSOURCE vrCreateDeinterlaceSource  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFSOURCE)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFSOURCE)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceSource) );
	
	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_DEINTERLACE]->target_pixmap_surface, 
								pStatics->default_target[VR_PROGRAM_DEINTERLACE]->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_DEINTERLACE]);
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return 0;	
	}

	/* Y4개를 한꺼번에*/
	EGLNativePixmapType pixmapInput = (fbdev_pixmap*)vrCreatePixmap(uiWidth/4, uiHeight, hData, VR_TRUE, 32, VR_IMAGE_FORMAT_Y/*VR_TRUE*/);
	if(pixmapInput == NULL || ((fbdev_pixmap*)pixmapInput)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);\
		FREE(result);
		return (HSURFSOURCE)0;
	}

	//RGB is not supported	
	EGLint imageAttributes[] = {
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, 
		EGL_NONE
	};	
	EGLImageKHR eglImage = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
							           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput, imageAttributes));	

	GLuint textureName;
	GL_CHECK(glGenTextures(1, &textureName));
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_DEINTERLACE));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));		

	result->width        = uiWidth ;
	result->height       = uiHeight;
	result->texture_name[VR_INPUT_MODE_Y] = textureName;
	result->src_native_pixmaps[VR_INPUT_MODE_Y]= pixmapInput;
	result->src_pixmap_images[VR_INPUT_MODE_Y] = eglImage   ;	

	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateDeinterlaceSource done\n"); 		
	return result;
}

void vrDestroyDeinterlaceSource ( HSURFSOURCE hSource )
{
	if( !hSource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
	GL_CHECK(glDeleteTextures(1,&hSource->texture_name[VR_INPUT_MODE_Y]));
	EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[VR_INPUT_MODE_Y]));
	vrDestroyPixmap(hSource->src_native_pixmaps[VR_INPUT_MODE_Y]);
	FREE(hSource);
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyDeinterlaceSource done\n"); 		
}

int vrCreateScaleSource  (HSURFSOURCE* psource ,unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE* pData)
{
	Statics *pStatics = vrGetStatics();
	if( !pStatics )
	{	
		for(int i = 0 ; i < VR_YUV_CNT ; i++){ psource[i] = NULL; }
		return VR_FALSE; 
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	VRImageFormatMode format[3] = { VR_IMAGE_FORMAT_Y, VR_IMAGE_FORMAT_U, VR_IMAGE_FORMAT_V };

	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_SCALE]->target_pixmap_surface, 
									pStatics->default_target[VR_PROGRAM_SCALE]->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_SCALE]);
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		for(int i = 0 ; i < VR_YUV_CNT ; i++){ psource[i] = NULL; }
		return VR_FALSE;	
	}
	
	unsigned int pixmap_width, pixmap_height;
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{	
		if(!pData[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}

		HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
		if( !result )
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
			for(int i = 0 ; i < VR_YUV_CNT ; i++){ psource[i] = NULL; }
			return VR_FALSE;
		}
		MEMSET( result, 0, sizeof(struct vrSurfaceSource) );

		if(0 == i)
		{
			//Y case
			pixmap_width = uiWidth;
			pixmap_height = uiHeight;
		}
		else if(1 == i)
		{
			//U case
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight/2;
		}
		else
		{
			//V case
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight/2;
		}

		EGLNativePixmapType pixmapInput = (fbdev_pixmap*)vrCreatePixmap(pixmap_width, pixmap_height, pData[i], VR_TRUE, 8, format[i]);
		if(pixmapInput == NULL || ((fbdev_pixmap*)pixmapInput)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);\
			FREE(result);
			for(int i = 0 ; i < VR_YUV_CNT ; i++){ psource[i] = NULL; }
			return VR_FALSE;
		}

		//RGB is not supported	
		EGLint imageAttributes[] = {
			EGL_IMAGE_PRESERVED_KHR, /*EGL_TRUE*/EGL_FALSE, 
			EGL_NONE
		};	
		EGLImageKHR eglImage = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
								           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput, imageAttributes));	

		GLuint textureName;
		GL_CHECK(glGenTextures(1, &textureName));
		GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE0));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));		

		result->width        = pixmap_width ;
		result->height       = pixmap_height;
		result->texture_name[0] = textureName;
		result->src_native_pixmaps[0]= pixmapInput;
		result->src_pixmap_images[0] = eglImage   ;		

		psource[i] = result;
		VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateScaleSource done(%d)\n", i);		
	}
	return VR_TRUE;
}

void vrDestroyScaleSource ( HSURFSOURCE* psource )
{
	if( !psource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
	
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{
		if(!psource[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}
		
		GL_CHECK(glDeleteTextures(1,&(psource[i]->texture_name[0])));
		EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, psource[i]->src_pixmap_images[0]));
		vrDestroyPixmap(psource[i]->src_native_pixmaps[0]);
		FREE(psource[i]);
		VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyScaleSource done(%d)\n", i);		
	}
}

HSURFSOURCE vrCreateCvt2YSource  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	//ErrMsg("vrCreateCvt2YSource start(%dx%d)\n", uiWidth, uiHeight);

	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFSOURCE)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFSOURCE)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceSource) );
	
	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_CVT2Y]->target_pixmap_surface, 
								pStatics->default_target[VR_PROGRAM_CVT2Y]->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2Y]);
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return 0;	
	}

	EGLNativePixmapType pixmapInput = (fbdev_pixmap*)vrCreatePixmap(uiWidth, uiHeight, hData, VR_FALSE, 32, VR_IMAGE_FORMAT_RGBA/*VR_FALSE*/);
	if(pixmapInput == NULL || ((fbdev_pixmap*)pixmapInput)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);\
		FREE(result);
		return (HSURFSOURCE)0;
	}

	//RGB is not supported	
	EGLint imageAttributes[] = {
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, 
		EGL_NONE
	};	
	EGLImageKHR eglImage = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
							           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput, imageAttributes));	

	GLuint textureName;
	GL_CHECK(glGenTextures(1, &textureName));
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE0));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));		

	result->width        = uiWidth ;
	result->height       = uiHeight;
	result->texture_name[VR_INPUT_MODE_Y] = textureName;
	result->src_native_pixmaps[VR_INPUT_MODE_Y]= pixmapInput;
	result->src_pixmap_images[VR_INPUT_MODE_Y] = eglImage   ;	

	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateCvt2YSource done\n"); 		
	return result;
}

void vrDestroyCvt2YSource ( HSURFSOURCE hSource )
{
	if( !hSource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
	GL_CHECK(glDeleteTextures(1,&hSource->texture_name[VR_INPUT_MODE_Y]));
	EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[VR_INPUT_MODE_Y]));
	vrDestroyPixmap(hSource->src_native_pixmaps[VR_INPUT_MODE_Y]);
	FREE(hSource);
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyCvt2YSource done\n"); 		
}

HSURFSOURCE vrCreateCvt2UVSource  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	//ErrMsg("vrCreateCvt2UVSource start(%dx%d)\n", uiWidth, uiHeight);

	Statics *pStatics = vrGetStatics();
	if( !pStatics ){ return (HSURFSOURCE)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFSOURCE)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceSource) );
	
	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_CVT2UV]->target_pixmap_surface, 
								pStatics->default_target[VR_PROGRAM_CVT2UV]->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2UV]);
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return 0;	
	}

	EGLNativePixmapType pixmapInput = (fbdev_pixmap*)vrCreatePixmap(uiWidth, uiHeight, hData, VR_FALSE, 32, VR_IMAGE_FORMAT_RGBA/*VR_FALSE*/);
	if(pixmapInput == NULL || ((fbdev_pixmap*)pixmapInput)->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);\
		FREE(result);
		return (HSURFSOURCE)0;
	}

	//RGB is not supported	
	EGLint imageAttributes[] = {
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, 
		EGL_NONE
	};	
	EGLImageKHR eglImage = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
							           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput, imageAttributes));	

	GLuint textureName;
	GL_CHECK(glGenTextures(1, &textureName));
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE1));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));		

	result->width        = uiWidth ;
	result->height       = uiHeight;
	result->texture_name[0] = textureName;
	result->src_native_pixmaps[0]= pixmapInput;
	result->src_pixmap_images[0] = eglImage   ;	

	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateCvt2UVSource done\n"); 		
	return result;
}

void vrDestroyCvt2UVSource ( HSURFSOURCE hSource )
{
	if( !hSource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
	GL_CHECK(glDeleteTextures(1,&hSource->texture_name[0]));
	EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[0]));
	vrDestroyPixmap(hSource->src_native_pixmaps[0]);
	FREE(hSource);
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyCvt2UVSource done\n"); 		
}

HSURFSOURCE vrCreateCvt2RgbaSource  (unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hDataY , NX_MEMORY_HANDLE hDataU, NX_MEMORY_HANDLE hDataV )
{
	Statics *pStatics = vrGetStatics();
	
	if( !pStatics ){ return (HSURFSOURCE)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFSOURCE)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceSource) );
	
	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_CVT2RGBA]->target_pixmap_surface, 
								pStatics->default_target[VR_PROGRAM_CVT2RGBA]->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2RGBA]);
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return 0;	
	}

	EGLNativePixmapType pixmapInput[VR_INPUT_MODE_YUV_MAX] = {NULL,};

	pixmapInput[VR_INPUT_MODE_Y] = (fbdev_pixmap*)vrCreatePixmap(uiWidth, uiHeight, hDataY, VR_TRUE, 8, VR_IMAGE_FORMAT_Y/*VR_FALSE*/);
	if(pixmapInput[VR_INPUT_MODE_Y] == NULL || ((fbdev_pixmap*)pixmapInput[VR_INPUT_MODE_Y])->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE(result);
		return (HSURFSOURCE)0;
	}
	pixmapInput[VR_INPUT_MODE_U] = (fbdev_pixmap*)vrCreatePixmap(uiWidth/2, uiHeight/2, hDataU, VR_TRUE, 8, VR_IMAGE_FORMAT_U/*VR_FALSE*/);
	if(pixmapInput[VR_INPUT_MODE_U] == NULL || ((fbdev_pixmap*)pixmapInput[VR_INPUT_MODE_U])->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE(result);
		return (HSURFSOURCE)0;
	}
	pixmapInput[VR_INPUT_MODE_V] = (fbdev_pixmap*)vrCreatePixmap(uiWidth/2, uiHeight/2, hDataV, VR_TRUE, 8, VR_IMAGE_FORMAT_V/*VR_FALSE*/);
	if(pixmapInput[VR_INPUT_MODE_V] == NULL || ((fbdev_pixmap*)pixmapInput[VR_INPUT_MODE_V])->data == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		FREE(result);
		return (HSURFSOURCE)0;
	}

	//RGB is not supported	
	EGLint imageAttributes[] = {
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, 
		EGL_NONE
	};	
	EGLImageKHR eglImage[VR_INPUT_MODE_YUV_MAX] = {NULL,};
	eglImage[VR_INPUT_MODE_Y] = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
							           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput[VR_INPUT_MODE_Y], imageAttributes));	
	eglImage[VR_INPUT_MODE_U] = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
							           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput[VR_INPUT_MODE_U], imageAttributes));	
	eglImage[VR_INPUT_MODE_V] = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
							           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput[VR_INPUT_MODE_V], imageAttributes));	

	GLuint textureName[VR_INPUT_MODE_YUV_MAX];
	GL_CHECK(glGenTextures(VR_INPUT_MODE_YUV_MAX, textureName));
	
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_Y));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName[VR_INPUT_MODE_Y]));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage[VR_INPUT_MODE_Y]));		
	result->texture_name[VR_INPUT_MODE_Y] = textureName[VR_INPUT_MODE_Y];
	result->src_native_pixmaps[VR_INPUT_MODE_Y]= pixmapInput[VR_INPUT_MODE_Y];
	result->src_pixmap_images[VR_INPUT_MODE_Y] = eglImage[VR_INPUT_MODE_Y]   ;				 

	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_U));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName[VR_INPUT_MODE_U]));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage[VR_INPUT_MODE_U]));		
	result->texture_name[VR_INPUT_MODE_U] = textureName[VR_INPUT_MODE_U];
	result->src_native_pixmaps[VR_INPUT_MODE_U]= pixmapInput[VR_INPUT_MODE_U];
	result->src_pixmap_images[VR_INPUT_MODE_U] = eglImage[VR_INPUT_MODE_U]   ;				 

	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_V));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName[VR_INPUT_MODE_V]));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage[VR_INPUT_MODE_V]));		
	result->texture_name[VR_INPUT_MODE_V] = textureName[VR_INPUT_MODE_V];
	result->src_native_pixmaps[VR_INPUT_MODE_V]= pixmapInput[VR_INPUT_MODE_V];
	result->src_pixmap_images[VR_INPUT_MODE_V] = eglImage[VR_INPUT_MODE_V]   ;				 
	
	result->width        = uiWidth ;
	result->height       = uiHeight;

	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateCvt2RgbaSource done\n"); 		
	return result;
}

void vrDestroyCvt2RgbaSource ( HSURFSOURCE hSource )
{
	if( !hSource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
	GL_CHECK(glDeleteTextures(VR_INPUT_MODE_YUV_MAX, hSource->texture_name));
	EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[VR_INPUT_MODE_Y]));
	EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[VR_INPUT_MODE_U]));
	EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[VR_INPUT_MODE_V]));
	vrDestroyPixmap(hSource->src_native_pixmaps[VR_INPUT_MODE_Y]);
	vrDestroyPixmap(hSource->src_native_pixmaps[VR_INPUT_MODE_U]);
	vrDestroyPixmap(hSource->src_native_pixmaps[VR_INPUT_MODE_V]);
	FREE(hSource);
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyCvt2RgbaSource done\n"); 		
}

#else

HSURFSOURCE vrCreateDeinterlaceSource  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	VR_ASSERT("width must be 8X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "%s start\n", __FUNCTION__);

	Statics *pStatics = vrGetStatics();
	VRImageFormatMode format[3] = { VR_IMAGE_FORMAT_V, VR_IMAGE_FORMAT_U, VR_IMAGE_FORMAT_Y };

	if( !pStatics || !hData ){ return (HSURFSOURCE)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFSOURCE)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceSource) );
	result->width		 = uiWidth ;
	result->height		 = uiHeight;
	#ifdef VR_FEATURE_STRIDE_USE
	result->stride		   = uiStride;
	#endif

	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_DEINTERLACE]->target_pixmap_surfaces[0], 
									pStatics->default_target[VR_PROGRAM_DEINTERLACE]->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_DEINTERLACE]);
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return 0;	
	}

	unsigned int pixmap_stride, pixmap_width, pixmap_height;
	if(!uiStride){ uiStride = uiWidth; }
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{					
		if(2 == i)
		{
			//Y case
			pixmap_stride = uiStride;
			pixmap_width = uiWidth;
			pixmap_height = uiHeight;
			result->total_texture_src_height[i] = pixmap_height;
		}
		else if(1 == i)
		{
			//U case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight/2;
			result->total_texture_src_height[i] = pixmap_height;
			#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				//add Y align blank offset
				unsigned int y_height, y_align_blank_height = 0;
				y_height = uiHeight;
				if(y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				pixmap_height += y_align_blank_height;
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d)\n", i, y_align_blank_height);		
			}
			#endif			
		}
		else
		{
			//V case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight;
			result->total_texture_src_height[i] = pixmap_height;
			#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				unsigned int y_height, y_align_blank_height = 0;
				unsigned int u_height, u_align_blank_height = 0;
				//add Y align blank offset
				y_height = uiHeight;
				if(y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				//add U align blank offset
				u_height = uiHeight/2;
				if(u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					u_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE));
				}
				pixmap_height += (y_align_blank_height + u_align_blank_height);
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d), u_align_blank_height(%d)\n", i, y_align_blank_height, u_align_blank_height);		
			}
			#endif						
		}
		#ifdef VR_FEATURE_STRIDE_USE
		pixmap_width = pixmap_stride;
		#endif

		EGLNativePixmapType pixmapInput = (fbdev_pixmap*)vrCreatePixmap(pixmap_width/4, pixmap_height, hData, VR_TRUE, 32, format[i]);
		if(pixmapInput == NULL || ((fbdev_pixmap*)pixmapInput)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);\
			FREE(result);
			return (HSURFSOURCE)0;
		}

		//RGB is not supported	
		EGLint imageAttributes[] = {
			EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, 
			EGL_NONE
		};	
		EGLImageKHR eglImage = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
								           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput, imageAttributes));	

		GLuint textureName;
		GL_CHECK(glGenTextures(1, &textureName));
		GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_DEINTERLACE));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));		

		result->texture_name[i] = textureName;
		result->src_native_pixmaps[i]= pixmapInput;
		result->src_pixmap_images[i] = eglImage   ;		
		
		VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "size(%dx%d, %d)\n", pixmap_width, pixmap_height, pixmap_stride);		
	}
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateDeinterlaceSource done\n"); 		
	return result;
}

void vrDestroyDeinterlaceSource ( HSURFSOURCE hSource )
{
	if( !hSource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;

	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{	
		if(!hSource->src_pixmap_images[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}
		GL_CHECK(glDeleteTextures(1,&hSource->texture_name[i]));
		EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[i]));
		vrDestroyPixmap(hSource->src_native_pixmaps[i]);
	}
	FREE(hSource);
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyDeinterlaceSource done\n"); 		
}

HSURFSOURCE vrCreateScaleSource  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	VR_ASSERT("width must be 8X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "%s start\n", __FUNCTION__);

	Statics *pStatics = vrGetStatics();
	VRImageFormatMode format[3] = { VR_IMAGE_FORMAT_V, VR_IMAGE_FORMAT_U, VR_IMAGE_FORMAT_Y };

	if( !pStatics || !hData ){ return (HSURFSOURCE)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFSOURCE)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceSource) );
	result->width		 = uiWidth ;
	result->height		 = uiHeight;
	#ifdef VR_FEATURE_STRIDE_USE
	result->stride		   = uiStride;
	#endif

	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_SCALE]->target_pixmap_surfaces[0], 
									pStatics->default_target[VR_PROGRAM_SCALE]->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_SCALE]);
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return 0;	
	}

	unsigned int pixmap_stride, pixmap_width, pixmap_height;
	if(!uiStride){ uiStride = uiWidth; }
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{			
		if(2 == i)
		{
			//Y case
			pixmap_stride = uiStride;
			pixmap_width = uiWidth;
			pixmap_height = uiHeight;
			result->total_texture_src_height[i] = pixmap_height;
		}
		else if(1 == i)
		{
			//U case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight/2;			
			result->total_texture_src_height[i] = pixmap_height;
			#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				//add Y align blank offset
				unsigned int y_height, y_align_blank_height = 0;
				y_height = uiHeight;
				if(y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				pixmap_height += y_align_blank_height;
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d)\n", i, y_align_blank_height);		
			}
			#endif
		}
		else
		{
			//V case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight;
			result->total_texture_src_height[i] = pixmap_height;
			#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				unsigned int y_height, y_align_blank_height = 0;
				unsigned int u_height, u_align_blank_height = 0;
				//add Y align blank offset
				y_height = uiHeight;
				if(y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				//add U align blank offset
				u_height = uiHeight/2;
				if(u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					u_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE));
				}
				pixmap_height += (y_align_blank_height + u_align_blank_height);
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d), u_align_blank_height(%d)\n", i, y_align_blank_height, u_align_blank_height);		
			}
			#endif			
		}
		#ifdef VR_FEATURE_STRIDE_USE
		pixmap_width = pixmap_stride;
		#endif
		EGLNativePixmapType pixmapInput = (fbdev_pixmap*)vrCreatePixmap(pixmap_width, pixmap_height, hData, VR_TRUE, 8, format[i]);
		if(pixmapInput == NULL || ((fbdev_pixmap*)pixmapInput)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);\
			FREE(result);
			return (HSURFSOURCE)0;
		}

		//RGB is not supported	
		EGLint imageAttributes[] = {
			EGL_IMAGE_PRESERVED_KHR, /*EGL_TRUE*/EGL_FALSE, 
			EGL_NONE
		};	
		EGLImageKHR eglImage = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
								           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput, imageAttributes));	

		GLuint textureName;
		GL_CHECK(glGenTextures(1, &textureName));
		GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE0));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));		

		result->texture_name[i] = textureName;
		result->src_native_pixmaps[i]= pixmapInput;
		result->src_pixmap_images[i] = eglImage   ;		
		
		VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "size(%dx%d, %d)\n", pixmap_width, pixmap_height, pixmap_stride);		
	}
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateScaleSource done\n"); 		
	return result;
}

void vrDestroyScaleSource ( HSURFSOURCE hSource )
{
	if( !hSource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;

	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{	
		if(!hSource->src_pixmap_images[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}
		GL_CHECK(glDeleteTextures(1,&hSource->texture_name[i]));
		EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[i]));
		vrDestroyPixmap(hSource->src_native_pixmaps[i]);
	}
	FREE(hSource);
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyScaleSource done\n"); 		
}

HSURFSOURCE vrCreateCvt2YuvSource  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	VR_ASSERT("width must be 8X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "%s start\n", __FUNCTION__);

	Statics *pStatics = vrGetStatics();

	if( !pStatics || !hData ){ return (HSURFSOURCE)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFSOURCE)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceSource) );
	result->width		 = uiWidth ;
	result->height		 = uiHeight;
	#ifdef VR_FEATURE_STRIDE_USE
	result->stride		   = uiStride;
	#endif

	unsigned int pixmap_stride, pixmap_width, pixmap_height;
	if(!uiStride){ uiStride = uiWidth; }
	pixmap_stride = uiStride, pixmap_width = uiWidth, pixmap_height = uiHeight;
	for(int i = 0 ; i < 2/*Y and UV*/ ; i++)
	{			
		/* Make context current. */
		EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_CVT2UV+i]->target_pixmap_surfaces[0], 
										pStatics->default_target[VR_PROGRAM_CVT2UV+i]->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2UV+i]);
		if(bResult == EGL_FALSE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
			return 0;	
		}
		
		#ifdef VR_FEATURE_STRIDE_USE
		pixmap_width = pixmap_stride;
		#endif
		EGLNativePixmapType pixmapInput = (fbdev_pixmap*)vrCreatePixmap(pixmap_width, pixmap_height, hData, VR_TRUE, 32, VR_IMAGE_FORMAT_RGBA/*VR_TRUE*/);
		if(pixmapInput == NULL || ((fbdev_pixmap*)pixmapInput)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);\
			FREE(result);
			return (HSURFSOURCE)0;
		}

		//RGB is not supported	
		EGLint imageAttributes[] = {
			EGL_IMAGE_PRESERVED_KHR, /*EGL_TRUE*/EGL_FALSE, 
			EGL_NONE
		};	
		EGLImageKHR eglImage = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
								           EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput, imageAttributes));	

		GLuint textureName;
		GL_CHECK(glGenTextures(1, &textureName));
		GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE0+i));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));		

		result->texture_name[i] = textureName;
		result->src_native_pixmaps[i]= pixmapInput;
		result->src_pixmap_images[i] = eglImage   ;		
		result->total_texture_src_height[i] = pixmap_height;
		
		VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "size(%dx%d, %d)\n", pixmap_width, pixmap_height, pixmap_stride);		
	}
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateCvt2YuvSource done\n"); 		
	return result;
}

void vrDestroyCvt2YuvSource ( HSURFSOURCE hSource )
{
	if( !hSource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;

	for(int i = 0 ; i < 2/*Y and UV*/ ; i++)
	{	
		if(!hSource->src_pixmap_images[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}
		GL_CHECK(glDeleteTextures(1,&hSource->texture_name[i]));
		EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[i]));
		vrDestroyPixmap(hSource->src_native_pixmaps[i]);
	}
	FREE(hSource);
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyCvt2YuvSource done\n"); 		
}


HSURFSOURCE vrCreateCvt2RgbaSource  (unsigned int uiStride, unsigned int uiWidth, unsigned int uiHeight, NX_MEMORY_HANDLE hData)
{
	VR_ASSERT("width must be 8X and height must be 2X", uiWidth && uiHeight && ((uiWidth&0x1) == 0x0) && ((uiHeight&0x1) == 0x0) );
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "%s start\n", __FUNCTION__);

	Statics *pStatics = vrGetStatics();
	VRImageFormatMode format[3] = { VR_IMAGE_FORMAT_Y, VR_IMAGE_FORMAT_U, VR_IMAGE_FORMAT_V };

	if( !pStatics || !hData ){ return (HSURFSOURCE)0; }
	_AUTO_BACKUP_CURRENT_EGL_;
	HSURFSOURCE result = (HSURFSOURCE)MALLOC(sizeof(struct vrSurfaceSource));
	if( !result )
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return (HSURFSOURCE)0;
	}
	MEMSET( result, 0, sizeof(struct vrSurfaceSource) );
	result->width		 = uiWidth ;
	result->height		 = uiHeight;
	#ifdef VR_FEATURE_STRIDE_USE
	result->stride		   = uiStride;
	#endif

	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, pStatics->default_target[VR_PROGRAM_CVT2RGBA]->target_pixmap_surfaces[0], 
									pStatics->default_target[VR_PROGRAM_CVT2RGBA]->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2RGBA]);

	unsigned int pixmap_stride, pixmap_width, pixmap_height;
	if(!uiStride){ uiStride = uiWidth; }
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{			
		if(bResult == EGL_FALSE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
			return 0;	
		}

		if(0 == i)
		{
			//Y case
			pixmap_stride = uiStride;
			pixmap_width = uiWidth;
			pixmap_height = uiHeight;
			result->total_texture_src_height[i] = pixmap_height;
		}
		else if(1 == i)
		{
			//U case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight/2;
			result->total_texture_src_height[i] = pixmap_height;
			#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				//add Y align blank offset
				unsigned int y_height, y_align_blank_height = 0;
				y_height = uiHeight;
				if(y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				pixmap_height += y_align_blank_height;
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d)\n", i, y_align_blank_height);		
			}
			#endif			
		}
		else
		{
			//V case
			pixmap_stride = uiWidth/2 + (uiStride-uiWidth)/2;
			pixmap_width = uiWidth/2;
			pixmap_height = uiHeight*2 + uiHeight;
			result->total_texture_src_height[i] = pixmap_height;
			#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
			{
				unsigned int y_height, y_align_blank_height = 0;
				unsigned int u_height, u_align_blank_height = 0;
				//add Y align blank offset
				y_height = uiHeight;
				if(y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					y_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
				}
				//add U align blank offset
				u_height = uiHeight/2;
				if(u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
				{	
					u_align_blank_height = (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE));
				}
				pixmap_height += (y_align_blank_height + u_align_blank_height);
				VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "[%d] y_align_blank_height(%d), u_align_blank_height(%d)\n", i, y_align_blank_height, u_align_blank_height);		
			}
			#endif						
		}
		#ifdef VR_FEATURE_STRIDE_USE
		pixmap_width = pixmap_stride;
		#endif

		EGLNativePixmapType pixmapInput = (fbdev_pixmap*)vrCreatePixmap(pixmap_width, pixmap_height, hData, VR_TRUE, 8, format[i]);
		if(pixmapInput == NULL || ((fbdev_pixmap*)pixmapInput)->data == NULL)
		{
			ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);\
			FREE(result);
			return (HSURFSOURCE)0;
		}

		//RGB is not supported	
		EGLint imageAttributes[] = {
			EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, 
			EGL_NONE
		};	
		EGLImageKHR eglImage = EGL_CHECK(_eglCreateImageKHR( pStatics->egl_info.sEGLDisplay, EGL_NO_CONTEXT, 
										   EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmapInput, imageAttributes));	

		GLuint textureName;
		GL_CHECK(glGenTextures(1, &textureName));
		GL_CHECK(glActiveTexture(GL_TEXTURE0 + i));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureName));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));		

		result->texture_name[i] = textureName;
		result->src_native_pixmaps[i]= pixmapInput;
		result->src_pixmap_images[i] = eglImage   ; 	
		
		VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "size(%dx%d, %d)\n", pixmap_width, pixmap_height, pixmap_stride);		
	}
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrCreateCvt2RgbaSource done\n");		
	return result;
}

void vrDestroyCvt2RgbaSource ( HSURFSOURCE hSource )
{
	if( !hSource ){ return; }
	Statics *pStatics = vrGetStatics();
	_AUTO_BACKUP_CURRENT_EGL_;
	VR_ASSERT("Error: YUV must exist", hSource->src_pixmap_images[VR_INPUT_MODE_Y] && hSource->src_pixmap_images[VR_INPUT_MODE_U] && hSource->src_pixmap_images[VR_INPUT_MODE_V]);

	GL_CHECK(glDeleteTextures(VR_INPUT_MODE_YUV_MAX, hSource->texture_name));
	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{
		EGL_CHECK(_eglDestroyImageKHR(pStatics->egl_info.sEGLDisplay, hSource->src_pixmap_images[i]));
		vrDestroyPixmap(hSource->src_native_pixmaps[i]);
		
	}
	FREE(hSource);
	VR_INFO("", VR_GRAPHIC_DBG_SOURCE, "vrDestroyCvt2RgbaSource done\n"); 		
}
#endif

#ifdef VR_FEATURE_SEPERATE_FD_USE
void  vrRunDeinterlace( HSURFTARGET hTarget, HSURFSOURCE hSource)
{
	Statics* pStatics = vrGetStatics();
	Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_DEINTERLACE]);	
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	const float aSquareTexCoord[] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	if( NULL == pStatics || NULL == hTarget || NULL == hSource )
	{
		ErrMsg("Error: NULL output surface at %s:%i\n", __FILE__, __LINE__);
		return;
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	/* Make context current. */
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
					hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_DEINTERLACE]);
	#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[VR_INPUT_MODE_Y], 
					hTarget->target_pixmap_surfaces[VR_INPUT_MODE_Y], pStatics->egl_info.sEGLContext[VR_PROGRAM_DEINTERLACE]);
	#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return;	
	}
	GL_CHECK(glUseProgram(pshader->iProgName));
	unsigned int width, height;
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	width = ((fbdev_pixmap *)hTarget->target_native_pixmap)->width;
	height = ((fbdev_pixmap *)hTarget->target_native_pixmap)->height;
	#else
	width = ((fbdev_pixmap *)hTarget->target_native_pixmaps[VR_INPUT_MODE_Y])->width;
	height = ((fbdev_pixmap *)hTarget->target_native_pixmaps[VR_INPUT_MODE_Y])->height;
	#endif
	GL_CHECK(glViewport(0,0,width,height));
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
	GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
	GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[0], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord));

	GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[0]));

    GL_CHECK(glUniform1f(pshader->iLocInputHeight, hSource->height));
    //GL_CHECK(glUniform1f(pStatics->shader[program].iLocOutputHeight, output_height));
	GL_CHECK(glUniform1i(pshader->iLocMainTex[0], VR_INPUT_MODE_DEINTERLACE));
	GL_CHECK(glUniform1i(pshader->iLocRefTex, VR_INPUT_MODE_DEINTERLACE_REF));

	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_DEINTERLACE));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_Y]));

	VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw Deinterlace\n" ); 
	GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
	vrWaitForDone();
}

void vrRunScale( HSURFTARGET* ptarget, HSURFSOURCE* psource)
{
	Statics* pStatics = vrGetStatics();
	Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_SCALE]);	
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	const float aSquareTexCoord[] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	if( NULL == pStatics || NULL == pshader || NULL == ptarget || NULL == psource )
	{
		ErrMsg("Error: NULL output surface at %s:%i\n", __FILE__, __LINE__);
		return;
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{
		if(!ptarget[i] || !psource[i])
		{
			ErrMsg("Error: ptarget(0x%x) psource(0x%x)\n", (int)ptarget[i], (int)psource[i]);		
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}			
		
		/* Make context current. */
		EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, ptarget[i]->target_pixmap_surface, ptarget[i]->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_SCALE]);
		if(bResult == EGL_FALSE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
			return; 
		}

		GL_CHECK(glUseProgram(pshader->iProgName));
		int x = 0, y = 0, width, height;
		width  = ((fbdev_pixmap *)ptarget[i]->target_native_pixmap)->width;
		height = ((fbdev_pixmap *)ptarget[i]->target_native_pixmap)->height;
		if(1 == i)
		{
			//U case
			y = height * 4;
		}
		else if(2 == i)
		{
			//V case
			y = height * 4;
			y+= height;
		}		
		GL_CHECK(glViewport(x,y,width,height));				
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
		GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
		GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[0], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord));
		GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
		GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[0]));
		GL_CHECK(glUniform1i(pshader->iLocMainTex[0], VR_INPUT_MODE_TEXTURE0));
		GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE0));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, psource[i]->texture_name[0]));
		VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw scaler\n" );

		GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
		
		//temp test
		//EGL_CHECK(eglSwapBuffers(pStatics->egl_info.sEGLDisplay, ptarget[i]->target_pixmap_surface));

		vrWaitForDone();	
	}
}

void  vrRunCvt2Y( HSURFTARGET hTarget, HSURFSOURCE hSource)
{
	Statics* pStatics = vrGetStatics();
	Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_CVT2Y]);	
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	const float aSquareTexCoord[] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	if( NULL == pStatics || NULL == hTarget || NULL == hSource )
	{
		ErrMsg("Error: NULL output surface at %s:%i\n", __FILE__, __LINE__);
		return;
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	/* Make context current. */
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
							hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2Y]);
	#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[VR_INPUT_MODE_Y], 
							hTarget->target_pixmap_surfaces[VR_INPUT_MODE_Y], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2Y]);
	#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return;	
	}
	GL_CHECK(glUseProgram(pshader->iProgName));
	
	int x=0, y=0, width, height;
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	width  = ((fbdev_pixmap *)hTarget->target_native_pixmap)->width;
	height = ((fbdev_pixmap *)hTarget->target_native_pixmap)->height;
	#else
	width  = ((fbdev_pixmap *)hTarget->target_native_pixmaps[VR_INPUT_MODE_Y])->width;
	height = ((fbdev_pixmap *)hTarget->target_native_pixmaps[VR_INPUT_MODE_Y])->height;
	#endif
	GL_CHECK(glViewport(x,y,width,height));
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
	GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
	GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[0], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord));

	GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[0]));

	GL_CHECK(glUniform1i(pshader->iLocMainTex[0], VR_INPUT_MODE_TEXTURE0));

	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE0));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_Y]));

	VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw\n" ); 
	GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
	vrWaitForDone();
}

void  vrRunCvt2UV( HSURFTARGET hTarget, HSURFSOURCE hSource)
{
	//DbgMsg( "vrRunCvt2UV start\n" ); 

	Statics* pStatics = vrGetStatics();
	Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_CVT2UV]);	
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	const float aSquareTexCoord[] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	if( NULL == pStatics || NULL == hTarget || NULL == hSource )
	{
		ErrMsg("Error: NULL output surface at %s:%i\n", __FILE__, __LINE__);
		return;
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	/* Make context current. */
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
							hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2UV]);
	#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[VR_INPUT_MODE_U], 
							hTarget->target_pixmap_surfaces[VR_INPUT_MODE_U], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2UV]);
	#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return;	
	}
	GL_CHECK(glUseProgram(pshader->iProgName));
	int x=0, y=0, width, height;
#ifdef VR_FEATURE_SEPERATE_FD_USE
	width  = ((fbdev_pixmap *)hTarget->target_native_pixmap)->width;
	height = ((fbdev_pixmap *)hTarget->target_native_pixmap)->height;
#else
	width  = ((fbdev_pixmap *)hTarget->target_native_pixmaps[VR_INPUT_MODE_U])->width;
	height = ((fbdev_pixmap *)hTarget->target_native_pixmaps[VR_INPUT_MODE_U])->height;
#endif
	GL_CHECK(glViewport(x,y,width,height));
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
	GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
	GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[0], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord));

	GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[0]));

	GL_CHECK(glUniform1i(pshader->iLocMainTex[0], VR_INPUT_MODE_TEXTURE1));

	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE1));
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[0]));
	#else
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_U]));
	#endif

	VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw(%d) Cvt2Yuv, (%d,%d) %dx%d\n", x, y, width, height );
	GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
	vrWaitForDone();
}

void  vrRunCvt2Rgba( HSURFTARGET hTarget, HSURFSOURCE hSource)
{
	Statics* pStatics = vrGetStatics();
	Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_CVT2RGBA]);	
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	const float aSquareTexCoord[] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	if( NULL == pStatics || NULL == hTarget || NULL == hSource )
	{
		ErrMsg("Error: NULL output surface at %s:%i\n", __FILE__, __LINE__);
		return;
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	/* Make context current. */
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
							hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2RGBA]);
	#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
							hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2RGBA]);
	#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return;	
	}
	GL_CHECK(glUseProgram(pshader->iProgName));
	int x=0, y=0, width, height;
	width  = ((fbdev_pixmap *)hTarget->target_native_pixmap)->width;
	height = ((fbdev_pixmap *)hTarget->target_native_pixmap)->height;
	GL_CHECK(glViewport(x,y,width,height));
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
	GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
	GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[0], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[0]));

    //GL_CHECK(glUniform1f(pStatics->shader[program].iLocOutputHeight, output_height));
	GL_CHECK(glUniform1i(pshader->iLocMainTex[VR_INPUT_MODE_Y], VR_INPUT_MODE_Y));
	GL_CHECK(glUniform1i(pshader->iLocMainTex[VR_INPUT_MODE_U], VR_INPUT_MODE_U));
	GL_CHECK(glUniform1i(pshader->iLocMainTex[VR_INPUT_MODE_V], VR_INPUT_MODE_V));

	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_Y));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_Y]));
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_U));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_U]));
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_V));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_V]));

	VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw(%d) Cvt2Rgba, (%d,%d) %dx%d\n", x, y, width, height );
	GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
	vrWaitForDone();
}

#else

void vrRunDeinterlace( HSURFTARGET hTarget, HSURFSOURCE hSource )
{
	Statics* pStatics = vrGetStatics();
	Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_DEINTERLACE]);	
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	float aSquareTexCoord[VR_INPUT_MODE_YUV_MAX][8] =
	{
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		},
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		},
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		}
	};

	VR_INFO("", VR_GRAPHIC_DBG_RUN, "%s start\n", __FUNCTION__);

#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
	float min_y, max_y, tex_height, tex_total_height_pitch; 
	float y_align_blank_height = 0.f, u_align_blank_height = 0.f;
	{
		unsigned int y_height, u_height;
		//add Y align blank offset
		y_height = hSource->height;
		if(hSource->height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
		{				
			y_align_blank_height = (float)(VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
		}
		//add U align blank offset
		u_height = hSource->height/2;
		if(u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
		{	
			u_align_blank_height = (float)(VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE));
		}
		VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "y_align_blank_height(%f), u_align_blank_height(%f)\n", y_align_blank_height, u_align_blank_height);		
	}
	tex_height = (float)hSource->height * 2.f;
	//set V coord
	tex_total_height_pitch = tex_height + tex_height/2 + y_align_blank_height + u_align_blank_height;
	min_y = tex_height + y_align_blank_height + tex_height/4.f + u_align_blank_height;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "V, min(%f),", min_y);		
	//normalize
	min_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " min nomalize(%f of %f)\n", min_y, tex_total_height_pitch);		
	aSquareTexCoord[0][3] = aSquareTexCoord[0][5] = min_y;
	//aSquareTexCoord[0][1] = aSquareTexCoord[0][7] = 1.f;

	//set U coord
	tex_total_height_pitch = tex_height + y_align_blank_height + tex_height/4;
	min_y = tex_height + y_align_blank_height;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "U, min(%f),", min_y);		
	//normalize
	min_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " min nomalize(%f)\n", min_y);		
	aSquareTexCoord[1][3] = aSquareTexCoord[1][5] = min_y;
	max_y = tex_height + y_align_blank_height + tex_height/4.f;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "U, max(%f),", max_y);		
	//normalize
	max_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " max nomalize(%f)\n", max_y);		
	aSquareTexCoord[1][1] = aSquareTexCoord[1][7] = max_y;
	
#else

	//set V coord
	//min y
	aSquareTexCoord[0][3] = aSquareTexCoord[0][5] = 5.f/6.f;
	//max y
	//aSquareTexCoord[0][1] = aSquareTexCoord[0][7] = 1.f;
	
	//set U coord
	//min y
	aSquareTexCoord[1][3] = aSquareTexCoord[1][5] = 4.f/5.f;
	//max y
	//aSquareTexCoord[1][1] = aSquareTexCoord[1][7] = 1.f;
#endif

	if(hSource->stride != hSource->width)
	{
		float max_x = (float)hSource->width/(float)hSource->stride;
		aSquareTexCoord[2][4] = aSquareTexCoord[2][6] = max_x;
		aSquareTexCoord[1][4] = aSquareTexCoord[1][6] = max_x;
		aSquareTexCoord[0][4] = aSquareTexCoord[0][6] = max_x;
	}	
	
	if( NULL == pStatics || NULL == hTarget || NULL == hSource )
	{
		ErrMsg("Error:  hTarget(0x%x), hSource(0x%x) at %s:%i\n", (unsigned int)hTarget, (unsigned int)hSource, __FILE__, __LINE__);
		return;
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{		
		if(!hTarget->target_pixmap_surfaces[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}			

		/* Make context current. */
		EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, 
										hTarget->target_pixmap_surfaces[i], 
										hTarget->target_pixmap_surfaces[i], 
										pStatics->egl_info.sEGLContext[VR_PROGRAM_DEINTERLACE]);
		if(bResult == EGL_FALSE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
			return; 
		}

		GL_CHECK(glUseProgram(pshader->iProgName));		
		int x=0, y=0, width, height;
		if(2 == i)
		{
			//Y case
			width  = hTarget->width;
			height = hTarget->height;
		}
		else if(1 == i)
		{
			//U case
			width  = hTarget->width/2;
			height = hTarget->height/2;
		}
		else
		{
			//V case
			width  = hTarget->width/2;
			height = hTarget->height/2;
		}		
		GL_CHECK(glViewport(x,y,width/4,height)); /* input Y 4개를 한꺼번에*/
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
		GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
		GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[0], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord[i]));

		GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
		GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[0]));

		GL_CHECK(glUniform1f(pshader->iLocInputHeight, hSource->total_texture_src_height[i]));
		GL_CHECK(glUniform1i(pshader->iLocMainTex[0], VR_INPUT_MODE_DEINTERLACE));
		GL_CHECK(glUniform1i(pshader->iLocRefTex, VR_INPUT_MODE_DEINTERLACE_REF));

		GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_DEINTERLACE));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[i]));

		GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));		

		vrWaitForDone();
		VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw(%d) deinterlace, (%d,%d) %dx%d\n", i, x, y, width, height );
	}	
}

void vrRunScale( HSURFTARGET hTarget, HSURFSOURCE hSource )
{
	Statics* pStatics = vrGetStatics();
	Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_SCALE]);	
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	float aSquareTexCoord[VR_INPUT_MODE_YUV_MAX][8] =
	{
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		},
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		},
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		}
	};
	VR_INFO("", VR_GRAPHIC_DBG_RUN, "%s start\n", __FUNCTION__);
	
#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
	float min_y, max_y, tex_height, tex_total_height_pitch; 
	float y_align_blank_height = 0.f, u_align_blank_height = 0.f;
	{
		unsigned int y_height, u_height;
		//add Y align blank offset
		y_height = hSource->height;
		if(hSource->height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
		{				
			y_align_blank_height = (float)(VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
		}
		//add U align blank offset
		u_height = hSource->height/2;
		if(u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
		{	
			u_align_blank_height = (float)(VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE));
		}
		VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "y_align_blank_height(%f), u_align_blank_height(%f)\n", y_align_blank_height, u_align_blank_height);		
	}
	tex_height = (float)hSource->height * 2.f;
	//set V coord
	tex_total_height_pitch = tex_height + tex_height/2 + y_align_blank_height + u_align_blank_height;
	min_y = tex_height + y_align_blank_height + tex_height/4.f + u_align_blank_height;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "V, min(%f),", min_y);		
	//normalize
	min_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " min nomalize(%f of %f)\n", min_y, tex_total_height_pitch);		
	aSquareTexCoord[0][3] = aSquareTexCoord[0][5] = min_y;
	//aSquareTexCoord[0][1] = aSquareTexCoord[0][7] = 1.f;

	//set U coord
	tex_total_height_pitch = tex_height + y_align_blank_height + tex_height/4;
	min_y = tex_height + y_align_blank_height;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "U, min(%f),", min_y);		
	//normalize
	min_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " min nomalize(%f)\n", min_y);		
	aSquareTexCoord[1][3] = aSquareTexCoord[1][5] = min_y;
	max_y = tex_height + y_align_blank_height + tex_height/4.f;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "U, max(%f),", max_y);		
	//normalize
	max_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " max nomalize(%f)\n", max_y);		
	aSquareTexCoord[1][1] = aSquareTexCoord[1][7] = max_y;

#else
	
	//set V coord
	//min y
	aSquareTexCoord[0][3] = aSquareTexCoord[0][5] = 5.f/6.f;
	//max y
	//aSquareTexCoord[0][1] = aSquareTexCoord[0][7] = 1.f;

	//set U coord
	//min y
	aSquareTexCoord[1][3] = aSquareTexCoord[1][5] = 4.f/5.f;
	//max y
	//aSquareTexCoord[1][1] = aSquareTexCoord[1][7] = 1.f;	
	
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "U(%f), V(%f)\n", 4.f/6.f, 5.f/6.f);		
#endif
	
	if(hSource->stride != hSource->width)
	{
		float max_x = (float)hSource->width/(float)hSource->stride;
		aSquareTexCoord[2][4] = aSquareTexCoord[2][6] = max_x;
		aSquareTexCoord[1][4] = aSquareTexCoord[1][6] = max_x;
		aSquareTexCoord[0][4] = aSquareTexCoord[0][6] = max_x;
	}
	
	if( NULL == pStatics || NULL == hTarget || NULL == hSource )
	{
		ErrMsg("Error:  hTarget(0x%x), hSource(0x%x) at %s:%i\n", (unsigned int)hTarget, (unsigned int)hSource, __FILE__, __LINE__);
		return;
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	for(int i = 0 ; i < VR_YUV_CNT ; i++)
	{		
		if(!hTarget->target_pixmap_surfaces[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}			

		/* Make context current. */
		EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, 
										hTarget->target_pixmap_surfaces[i], 
										hTarget->target_pixmap_surfaces[i], 
										pStatics->egl_info.sEGLContext[VR_PROGRAM_SCALE]);
		if(bResult == EGL_FALSE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
			return; 
		}

		GL_CHECK(glUseProgram(pshader->iProgName));		
		int x=0, y=0, width, height;
		if(2 == i)
		{
			//Y case
			width  = hTarget->width;
			height = hTarget->height;
		}
		else if(1 == i)
		{
			//U case
			width  = (hTarget->width)/2;
			height = (hTarget->height)/2;
		}
		else
		{
			//V case
			width  = (hTarget->width)/2;
			height = (hTarget->height)/2;
		}		
		GL_CHECK(glViewport(x,y,width,height));		
		glClearColor(0.f, 1.f, 0.f, 0.f);
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
		GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
		GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[0], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord[i]));

		GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
		GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[0]));

		GL_CHECK(glUniform1i(pshader->iLocMainTex[0], VR_INPUT_MODE_TEXTURE0));

		GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE0));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[i]));

		GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));		

		vrWaitForDone();
		VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw(%d) scaler, (%d,%d) %dx%d\n", i, x, y, width, height );
	}	
}

void  vrRunCvt2Yuv( HSURFTARGET hTarget, HSURFSOURCE hSource)
{
	Statics* pStatics = vrGetStatics();
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	float aSquareTexCoord[] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	
	if( NULL == pStatics || NULL == hTarget || NULL == hSource )
	{
		ErrMsg("Error:	hTarget(0x%x), hSource(0x%x) at %s:%i\n", (unsigned int)hTarget, (unsigned int)hSource, __FILE__, __LINE__);
		return;
	}
	VR_INFO("", VR_GRAPHIC_DBG_RUN, "%s start\n", __FUNCTION__);

	if(hSource->stride != hSource->width)
	{
		float max_x = (float)hSource->width/(float)hSource->stride;
		aSquareTexCoord[4] = aSquareTexCoord[6] = max_x;
	}
	
	_AUTO_BACKUP_CURRENT_EGL_;

	for(int i = 0 ; i < VR_Y_UV_CNT ; i++)
	{			
		Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_CVT2UV+i]);	
		if(!hTarget->target_pixmap_surfaces[i])
		{
			VR_ASSERT("Error: Y must exist", i != 0);				
			continue;
		}			

		/* Make context current. */
		EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, 
										hTarget->target_pixmap_surfaces[i], 
										hTarget->target_pixmap_surfaces[i], 
										pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2UV+i]);
		if(bResult == EGL_FALSE)
		{
			EGLint iError = eglGetError();
			ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
			ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
			return; 
		}

		GL_CHECK(glUseProgram(pshader->iProgName)); 	
		int x=0, y=0, width, height;
		if(1 == i)
		{
			width  = hTarget->width;
			height = hTarget->height;
		}
		else
		{
			//UV case
			width  = (hTarget->width)/2;
			height = (hTarget->height)/2;
		}
		GL_CHECK(glViewport(x,y,width,height));
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
		GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
		GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[0], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord));

		GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
		GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[0]));

		GL_CHECK(glUniform1i(pshader->iLocMainTex[0], VR_INPUT_MODE_TEXTURE0+i));

		GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_TEXTURE0+i));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[i]));

		GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));		

		vrWaitForDone();
		VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw(%d) Cvt2Yuv, (%d,%d) %dx%d\n", i, x, y, width, height );
	}	
}

void  vrRunCvt2Rgba( HSURFTARGET hTarget, HSURFSOURCE hSource)
{
	Statics* pStatics = vrGetStatics();
	Shader* pshader = &(vrGetStatics()->shader[VR_PROGRAM_CVT2RGBA]);	
	const float aSquareVertex[] =
	{
		-1.0f,	-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
	};	
	float aSquareTexCoord[VR_INPUT_MODE_YUV_MAX][8] =
	{
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		},
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		},
		{
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		}
	};

	VR_INFO("", VR_GRAPHIC_DBG_RUN, "%s start\n", __FUNCTION__);

#if (VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE > 0)
	float min_y, max_y, tex_height, tex_total_height_pitch; 
	float y_align_blank_height = 0.f, u_align_blank_height = 0.f;
	{
		unsigned int y_height, u_height;
		//add Y align blank offset
		y_height = hSource->height;
		if(hSource->height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
		{				
			y_align_blank_height = (float)(VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (y_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)) * 2;
		}
		//add U align blank offset
		u_height = hSource->height/2;
		if(u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE)
		{	
			u_align_blank_height = (float)(VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE - (u_height % VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE));
		}
		VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "y_align_blank_height(%f), u_align_blank_height(%f)\n", y_align_blank_height, u_align_blank_height);		
	}
	tex_height = (float)hSource->height * 2.f;
	//set V coord
	tex_total_height_pitch = tex_height + tex_height/2 + y_align_blank_height + u_align_blank_height;
	min_y = tex_height + y_align_blank_height + tex_height/4.f + u_align_blank_height;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "V, min(%f),", min_y);		
	//normalize
	min_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " min nomalize(%f of %f)\n", min_y, tex_total_height_pitch);		
	aSquareTexCoord[VR_INPUT_MODE_V][3] = aSquareTexCoord[VR_INPUT_MODE_V][5] = min_y;
	//aSquareTexCoord[VR_INPUT_MODE_V][1] = aSquareTexCoord[VR_INPUT_MODE_V][7] = 1.f;

	//set U coord
	tex_total_height_pitch = tex_height + y_align_blank_height + tex_height/4;
	min_y = tex_height + y_align_blank_height;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "U, min(%f),", min_y);		
	//normalize
	min_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " min nomalize(%f)\n", min_y);		
	aSquareTexCoord[VR_INPUT_MODE_U][3] = aSquareTexCoord[VR_INPUT_MODE_U][5] = min_y;
	max_y = tex_height + y_align_blank_height + tex_height/4.f;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, "U, max(%f),", max_y);		
	//normalize
	max_y /= tex_total_height_pitch;
	VR_INFO("", VR_GRAPHIC_DBG_HEIGHT_ALIGN, " max nomalize(%f)\n", max_y);		
	aSquareTexCoord[VR_INPUT_MODE_U][1] = aSquareTexCoord[VR_INPUT_MODE_U][7] = max_y;

#else

	//set U coord
	//min y
	aSquareTexCoord[VR_INPUT_MODE_U][3] = aSquareTexCoord[VR_INPUT_MODE_U][5] = 4.f/5.f;
	//max y
	//aSquareTexCoord[VR_INPUT_MODE_U][1] = aSquareTexCoord[VR_INPUT_MODE_U][7] = 1.f;
	//set V coord
	//min y
	aSquareTexCoord[VR_INPUT_MODE_V][3] = aSquareTexCoord[VR_INPUT_MODE_V][5] = 5.f/6.f;
	//max y
	//aSquareTexCoord[VR_INPUT_MODE_V][1] = aSquareTexCoord[VR_INPUT_MODE_V][7] = 1.f;	
#endif

	if(hSource->stride != hSource->width)
	{
		float max_x = (float)hSource->width/(float)hSource->stride;
		aSquareTexCoord[VR_INPUT_MODE_Y][4] = aSquareTexCoord[VR_INPUT_MODE_Y][6] = max_x;
		aSquareTexCoord[VR_INPUT_MODE_U][4] = aSquareTexCoord[VR_INPUT_MODE_U][6] = max_x;
		aSquareTexCoord[VR_INPUT_MODE_V][4] = aSquareTexCoord[VR_INPUT_MODE_V][6] = max_x;
	}

	if( NULL == pStatics || NULL == hTarget || NULL == hSource )
	{
		ErrMsg("Error: NULL output surface at %s:%i\n", __FILE__, __LINE__);
		return;
	}
	_AUTO_BACKUP_CURRENT_EGL_;

	/* Make context current. */
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
							hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2RGBA]);
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return;	
	}
	GL_CHECK(glUseProgram(pshader->iProgName));
	int x=0, y=0, width, height;
	width  = hTarget->width;
	height = hTarget->height;
	GL_CHECK(glViewport(x,y,width,height));
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); // To optimize for tile-based renderer
	GL_CHECK(glVertexAttribPointer(pshader->iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, aSquareVertex));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocPosition));
	GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[VR_INPUT_MODE_Y], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord[VR_INPUT_MODE_Y]));
	GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[VR_INPUT_MODE_U], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord[VR_INPUT_MODE_U]));
	GL_CHECK(glVertexAttribPointer(pshader->iLocTexCoord[VR_INPUT_MODE_V], 2, GL_FLOAT, GL_FALSE, 0, aSquareTexCoord[VR_INPUT_MODE_V]));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[VR_INPUT_MODE_Y]));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[VR_INPUT_MODE_U]));
	GL_CHECK(glEnableVertexAttribArray(pshader->iLocTexCoord[VR_INPUT_MODE_V]));

    //GL_CHECK(glUniform1f(pStatics->shader[program].iLocOutputHeight, output_height));
	GL_CHECK(glUniform1i(pshader->iLocMainTex[VR_INPUT_MODE_Y], VR_INPUT_MODE_Y));
	GL_CHECK(glUniform1i(pshader->iLocMainTex[VR_INPUT_MODE_U], VR_INPUT_MODE_U));
	GL_CHECK(glUniform1i(pshader->iLocMainTex[VR_INPUT_MODE_V], VR_INPUT_MODE_V));

	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_Y));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_Y]));
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_U));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_U]));
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_V));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, hSource->texture_name[VR_INPUT_MODE_V]));

	GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
	vrWaitForDone();
	VR_INFO("", VR_GRAPHIC_DBG_RUN, "draw Cvt2Rgba, (%d,%d) %dx%d\n", x, y, width, height );
}
#endif

void  vrWaitForDone( void )
{
	EGL_CHECK(eglWaitGL());
}

#ifdef VR_FEATURE_SHADER_FILE_USE
/* loadShader():	Load the shader hSource into memory.
 *
 * sFilename: String holding filename to load.
 */
static char *loadShader(const char *sFilename)
{
	char *pResult = NULL;
	FILE *pFile = NULL;
	long iLen = 0;

	pFile = fopen(sFilename, "r");
	if(pFile == NULL) {
		ErrMsg("Error: Cannot read file '%s'\n", sFilename);
		return NULL;
	}
	fseek(pFile, 0, SEEK_END); /* Seek end of file. */
	iLen = ftell(pFile);
	fseek(pFile, 0, SEEK_SET); /* Seek start of file again. */
	pResult = (char*)CALLOC(iLen+1, sizeof(char));
	if(pResult == NULL)
	{
		ErrMsg("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}
	fread(pResult, sizeof(char), iLen, pFile);
	pResult[iLen] = '\0';
	fclose(pFile);

	return pResult;
}

/* processShader(): Create shader, load in hSource, compile, dump debug as necessary.
 *
 * pShader: Pointer to return created shader ID.
 * sFilename: Passed-in filename from which to load shader hSource.
 * iShaderType: Passed to GL, e.g. GL_VERTEX_SHADER.
 */
static int processShader(GLuint *pShader, const char *sFilename, GLint iShaderType)
{
	GLint iStatus;
	const char *aStrings[1] = { NULL };

	/* Create shader and load into GL. */
	*pShader = GL_CHECK(glCreateShader(iShaderType));
	aStrings[0] = loadShader(sFilename);
	if(aStrings[0] == NULL)
	{
		ErrMsg("Error: wrong shader code %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	GL_CHECK(glShaderSource(*pShader, 1, aStrings, NULL));

	/* Clean up shader hSource. */
	FREE((void *)(aStrings[0]));
	aStrings[0] = NULL;

	/* Try compiling the shader. */
	GL_CHECK(glCompileShader(*pShader));
	GL_CHECK(glGetShaderiv(*pShader, GL_COMPILE_STATUS, &iStatus));

	/* Dump debug info (hSource and log) if compilation failed. */
	if(iStatus != GL_TRUE) {
		GLint iLen;
		char *sDebugSource = NULL;
		char *sErrorLog = NULL;

		/* Get shader hSource. */
		GL_CHECK(glGetShaderiv(*pShader, GL_SHADER_SOURCE_LENGTH, &iLen));
		sDebugSource = (char*)MALLOC(iLen);
		GL_CHECK(glGetShaderSource(*pShader, iLen, NULL, sDebugSource));
		DbgMsg("Debug hSource START:\n%s\nDebug hSource END\n\n", sDebugSource);
		FREE(sDebugSource);

		/* Now get the info log. */
		GL_CHECK(glGetShaderiv(*pShader, GL_INFO_LOG_LENGTH, &iLen));
		sErrorLog = (char*)MALLOC(iLen);
		GL_CHECK(glGetShaderInfoLog(*pShader, iLen, NULL, sErrorLog));
		DbgMsg("Log START:\n%s\nLog END\n\n", sErrorLog);
		FREE(sErrorLog);

		DbgMsg("Compilation FAILED!\n\n");
		return -1;
	}
	return 0;
}
#else
/* processShader(): Create shader, load in hSource, compile, dump debug as necessary.
 *
 * pShader: Pointer to return created shader ID.
 * sFilename: Passed-in filename from which to load shader hSource.
 * iShaderType: Passed to GL, e.g. GL_VERTEX_SHADER.
 */
static int processShader(GLuint *pShader, const char *pString, GLint iShaderType)
{
	GLint iStatus;
	const char *aStrings[1] = { NULL };

	if(pString == NULL)
	{
		ErrMsg("Error: wrong shader code %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	/* Create shader and load into GL. */
	*pShader = GL_CHECK(glCreateShader(iShaderType));
	if(pShader == NULL)
	{
		ErrMsg("Error: wrong shader code %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	aStrings[0] = pString;
	GL_CHECK(glShaderSource(*pShader, 1, aStrings, NULL));

	/* Clean up shader hSource. */
	aStrings[0] = NULL;

	/* Try compiling the shader. */
	GL_CHECK(glCompileShader(*pShader));
	GL_CHECK(glGetShaderiv(*pShader, GL_COMPILE_STATUS, &iStatus));

	/* Dump debug info (hSource and log) if compilation failed. */
	if(iStatus != GL_TRUE) {
		GLint iLen;
		char *sDebugSource = NULL;
		char *sErrorLog = NULL;

		/* Get shader hSource. */
		GL_CHECK(glGetShaderiv(*pShader, GL_SHADER_SOURCE_LENGTH, &iLen));
		sDebugSource = (char*)MALLOC(iLen);
		GL_CHECK(glGetShaderSource(*pShader, iLen, NULL, sDebugSource));
		DbgMsg("Debug hSource START:\n%s\nDebug hSource END\n\n", sDebugSource);
		FREE(sDebugSource);

		/* Now get the info log. */
		GL_CHECK(glGetShaderiv(*pShader, GL_INFO_LOG_LENGTH, &iLen));
		sErrorLog = (char*)MALLOC(iLen);
		GL_CHECK(glGetShaderInfoLog(*pShader, iLen, NULL, sErrorLog));
		DbgMsg("Log START:\n%s\nLog END\n\n", sErrorLog);
		FREE(sErrorLog);

		DbgMsg("Compilation FAILED!\n\n");
		return -1;
	}
	return 0;
}
#endif

/* For Deinterlace. */
//For 32bit context
static int vrInitializeDeinterlace( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	unsigned int program = VR_PROGRAM_DEINTERLACE;
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitializeDeinterlace start\n");	

	if(vrCreateEGLContext(program) != 0)
	{
		ErrMsg("Error: Fail to create context %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}
	
	/* Make context current. */
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
							hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[program]);
	#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
							hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[program]);
	#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}	
	
	/* Load shaders. */
	if(processShader(&pStatics->shader[program].iVertName, VERTEX_SHADER_SOURCE_DEINTERLACE, GL_VERTEX_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	if(processShader(&pStatics->shader[program].iFragName, FRAGMENT_SHADER_SOURCE_DEINTERLACE, GL_FRAGMENT_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}	

	/* Set up shaders. */
	pStatics->shader[program].iProgName = GL_CHECK(glCreateProgram());
	//DbgMsg("Deinterlace iProgName(%d)\n", pStatics->shader[program].iProgName);		
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iVertName));
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iFragName));
	GL_CHECK(glLinkProgram(pStatics->shader[program].iProgName));
	GL_CHECK(glUseProgram(pStatics->shader[program].iProgName));
	
	/* Vertex positions. */
	pStatics->shader[program].iLocPosition = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v4Position"));
	if(pStatics->shader[program].iLocPosition == -1)
	{
		ErrMsg("Error: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocPosition));

	/* Fill texture. */
	pStatics->shader[program].iLocTexCoord[0] = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v2TexCoord"));
	if(pStatics->shader[program].iLocTexCoord[0] == -1)
	{
		ErrMsg("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocTexCoord));

    /* Texture Height. */
    pStatics->shader[program].iLocInputHeight = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "u_fTexHeight"));
    if(pStatics->shader[program].iLocInputHeight == -1)
    {
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
    }

    /* diffuse texture. */
    pStatics->shader[program].iLocMainTex[0] = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "diffuse"));
    if(pStatics->shader[program].iLocMainTex[0] == -1)
    {
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
    }
    else 
    {
        //GL_CHECK(glUniform1i(pStatics->shader[program].iLocMainTex[0], VR_INPUT_MODE_DEINTERLACE));
    }	

	/* ref texture. */
    pStatics->shader[program].iLocRefTex = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "ref_tex"));
    if(pStatics->shader[program].iLocRefTex == -1)
    {
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
    }
    else 
    {
        //GL_CHECK(glUniform1i(pStatics->shader[program].iLocRefTex, VR_INPUT_MODE_DEINTERLACE_REF));
    }
	
	//set texture
	GL_CHECK(glGenTextures(1, &pStatics->tex_deinterlace_ref_id));
	GL_CHECK(glActiveTexture(GL_TEXTURE0 + VR_INPUT_MODE_DEINTERLACE_REF));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, pStatics->tex_deinterlace_ref_id));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)); 
	{
		unsigned int temp_imgbuf[2] = {0x00000000, 0xFFFFFFFF};
		GL_CHECK(glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,
				 1,2,0,
				 GL_RGBA,GL_UNSIGNED_BYTE,temp_imgbuf));	
	}	

	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitializeDeinterlace end\n"); 	
	return 0;			
}				

/* For Scaler. */
//For 8bit context
static int vrInitializeScaler( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	unsigned int program = VR_PROGRAM_SCALE;
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitialize_scale start\n");		

	if(vrCreateEGLContext(program) != 0)
	{
		ErrMsg("Error: Fail to create context %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}

	/* Make context current. */
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
							hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[program]);
	#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
							hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[program]);
	#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}
	
	/* Load shaders. */
	if(processShader(&pStatics->shader[program].iVertName, VERTEX_SHADER_SOURCE_SCALE, GL_VERTEX_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	if(processShader(&pStatics->shader[program].iFragName, FRAGMENT_SHADER_SOURCE_SCALE, GL_FRAGMENT_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	
	/* Set up shaders. */
	pStatics->shader[program].iProgName = GL_CHECK(glCreateProgram());
	//DbgMsg("Scaler iProgName(%d)\n", pStatics->shader[program].iProgName);
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iVertName));
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iFragName));
	GL_CHECK(glLinkProgram(pStatics->shader[program].iProgName));
	GL_CHECK(glUseProgram(pStatics->shader[program].iProgName));

	/* Vertex positions. */
	pStatics->shader[program].iLocPosition = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v4Position"));
	if(pStatics->shader[program].iLocPosition == -1)
	{
		ErrMsg("Error: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocPosition));

	/* Fill texture. */
	pStatics->shader[program].iLocTexCoord[0] = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v2TexCoord"));
	if(pStatics->shader[program].iLocTexCoord[0] == -1)
	{
		ErrMsg("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocTexCoord));

	/* diffuse texture. */
	pStatics->shader[program].iLocMainTex[0] = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "diffuse"));
	if(pStatics->shader[program].iLocMainTex[0] == -1)
	{
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
	}
	else 
	{
		//GL_CHECK(glUniform1i(pStatics->shader[program].iLocMainTex[0], VR_INPUT_MODE_TEXTURE0));
	}
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitialize_scale end\n"); 		
	return 0;			
}

/* For Cvt2Y. */
//For 8bit context		
static int vrInitializeCvt2Y( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	unsigned int program = VR_PROGRAM_CVT2Y;
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitializeCvt2Y start\n");			

	if(vrCreateEGLContext(program) != 0)
	{
		ErrMsg("Error: Fail to create context %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}

	/* Make context current. */
	#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
								hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[program]);
	#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
								hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[program]);
	#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}
	
	/* Load shaders. */
	if(processShader(&pStatics->shader[program].iVertName, VERTEX_SHADER_SOURCE_CVT2Y, GL_VERTEX_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	if(processShader(&pStatics->shader[program].iFragName, FRAGMENT_SHADER_SOURCE_CVT2Y, GL_FRAGMENT_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	
	/* Set up shaders. */
	pStatics->shader[program].iProgName = GL_CHECK(glCreateProgram());
	//DbgMsg("Scaler iProgName(%d)\n", pStatics->shader[program].iProgName);
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iVertName));
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iFragName));
	GL_CHECK(glLinkProgram(pStatics->shader[program].iProgName));
	GL_CHECK(glUseProgram(pStatics->shader[program].iProgName));

	/* Vertex positions. */
	pStatics->shader[program].iLocPosition = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v4Position"));
	if(pStatics->shader[program].iLocPosition == -1)
	{
		ErrMsg("Error: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocPosition));

	/* Fill texture. */
	pStatics->shader[program].iLocTexCoord[0] = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v2TexCoord"));
	if(pStatics->shader[program].iLocTexCoord[0] == -1)
	{
		ErrMsg("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocTexCoord));

	/* diffuse texture. */
	pStatics->shader[program].iLocMainTex[0] = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "diffuse"));
	if(pStatics->shader[program].iLocMainTex[0] == -1)
	{
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
	}
	else 
	{
		//GL_CHECK(glUniform1i(pStatics->shader[program].iLocMainTex[0], VR_INPUT_MODE_TEXTURE0));
	}
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitializeCvt2Y end\n"); 			
	return 0;
}			


/* For Cvt2UV. */
//For 16bit context
static int vrInitializeCvt2UV( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	unsigned int program = VR_PROGRAM_CVT2UV;
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitializeCvt2UV start\n");	

	if(vrCreateEGLContext(program) != 0)
	{
		ErrMsg("Error: Fail to create context %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}

	/* Make context current. */
#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
								hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[program]);
#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
								hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[program]);
#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}
	
	/* Load shaders. */
	if(processShader(&pStatics->shader[program].iVertName, VERTEX_SHADER_SOURCE_CVT2UV, GL_VERTEX_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	if(processShader(&pStatics->shader[program].iFragName, FRAGMENT_SHADER_SOURCE_CVT2UV, GL_FRAGMENT_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	
	/* Set up shaders. */
	pStatics->shader[program].iProgName = GL_CHECK(glCreateProgram());
	//DbgMsg("Scaler iProgName(%d)\n", pStatics->shader[program].iProgName);
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iVertName));
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iFragName));
	GL_CHECK(glLinkProgram(pStatics->shader[program].iProgName));
	GL_CHECK(glUseProgram(pStatics->shader[program].iProgName));

	/* Vertex positions. */
	pStatics->shader[program].iLocPosition = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v4Position"));
	if(pStatics->shader[program].iLocPosition == -1)
	{
		ErrMsg("Error: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocPosition));

	/* Fill texture. */
	pStatics->shader[program].iLocTexCoord[0] = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v2TexCoord"));
	if(pStatics->shader[program].iLocTexCoord[0] == -1)
	{
		ErrMsg("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocTexCoord));

	/* diffuse texture. */
	pStatics->shader[program].iLocMainTex[0] = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "diffuse"));
	if(pStatics->shader[program].iLocMainTex[0] == -1)
	{
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
	}
	else 
	{
		//GL_CHECK(glUniform1i(pStatics->shader[program].iLocMainTex[0], VR_INPUT_MODE_TEXTURE0));
	}
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitializeCvt2UV end\n"); 		
	return 0;
}

/* For Cvt2Rgba. */		
//For 32bit context
static int vrInitializeCvt2Rgba( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	unsigned int program = VR_PROGRAM_CVT2RGBA;
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitialize_cvt2rgb start\n");		

	if(vrCreateEGLContext(program) != 0)
	{
		ErrMsg("Error: Fail to create context %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}

	/* Make context current. */
#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
								hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[program]);
#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
								hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[program]);
#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1;	
	}
	
	/* Load shaders. */
	if(processShader(&pStatics->shader[program].iVertName, VERTEX_SHADER_SOURCE_CVT2RGBA, GL_VERTEX_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	if(processShader(&pStatics->shader[program].iFragName, FRAGMENT_SHADER_SOURCE_CVT2RGBA, GL_FRAGMENT_SHADER) < 0)
	{
		ErrMsg("Error: wrong shader %s:%i\n", __FILE__, __LINE__);
		return -1;
	}

	/* Set up shaders. */
	pStatics->shader[program].iProgName = GL_CHECK(glCreateProgram());
	//DbgMsg("Deinterlace iProgName(%d)\n", pStatics->shader[program].iProgName);		
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iVertName));
	GL_CHECK(glAttachShader(pStatics->shader[program].iProgName, pStatics->shader[program].iFragName));
	GL_CHECK(glLinkProgram(pStatics->shader[program].iProgName));
	GL_CHECK(glUseProgram(pStatics->shader[program].iProgName));

	/* Vertex positions. */
	pStatics->shader[program].iLocPosition = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v4Position"));
	if(pStatics->shader[program].iLocPosition == -1)
	{
		ErrMsg("Error: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocPosition));

	/* Fill texture. */
	pStatics->shader[program].iLocTexCoord[VR_INPUT_MODE_Y] = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v2TexCoordY"));
	if(pStatics->shader[program].iLocTexCoord[VR_INPUT_MODE_Y] == -1)
	{
		ErrMsg("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocTexCoord));
	pStatics->shader[program].iLocTexCoord[VR_INPUT_MODE_U] = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v2TexCoordU"));
	if(pStatics->shader[program].iLocTexCoord[VR_INPUT_MODE_U] == -1)
	{
		ErrMsg("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocTexCoord));
	pStatics->shader[program].iLocTexCoord[VR_INPUT_MODE_V] = GL_CHECK(glGetAttribLocation(pStatics->shader[program].iProgName, "a_v2TexCoordV"));
	if(pStatics->shader[program].iLocTexCoord[VR_INPUT_MODE_V] == -1)
	{
		ErrMsg("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
		return -1;
	}
	//else GL_CHECK(glEnableVertexAttribArray(pStatics->shader[program].iLocTexCoord));
		
	/* Y texture. */
	pStatics->shader[program].iLocMainTex[VR_INPUT_MODE_Y] = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "diffuseY"));
	if(pStatics->shader[program].iLocMainTex[VR_INPUT_MODE_Y] == -1)
	{
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
	}
	else 
	{
		//GL_CHECK(glUniform1i(pStatics->shader[program].iLocMainTex[0], VR_INPUT_MODE_DEINTERLACE));
	}
		
	/* U texture. */
	pStatics->shader[program].iLocMainTex[VR_INPUT_MODE_U] = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "diffuseU"));
	if(pStatics->shader[program].iLocMainTex[VR_INPUT_MODE_U] == -1)
	{
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
	}
	else 
	{
		//GL_CHECK(glUniform1i(pStatics->shader[program].iLocMainTex[0], VR_INPUT_MODE_DEINTERLACE));
	}
		
	/* V texture. */
	pStatics->shader[program].iLocMainTex[VR_INPUT_MODE_V] = GL_CHECK(glGetUniformLocation(pStatics->shader[program].iProgName, "diffuseV"));
	if(pStatics->shader[program].iLocMainTex[VR_INPUT_MODE_V] == -1)
	{
		ErrMsg("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
		//return -1;
	}
	else 
	{
		//GL_CHECK(glUniform1i(pStatics->shader[program].iLocMainTex[0], VR_INPUT_MODE_DEINTERLACE));
	}	
	VR_INFO("", VR_GRAPHIC_DBG_CTX, "vrInitialize_cvt2rgb end\n"); 		
	return 0;
}

static int vrDeinitializeDeinterlace( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	int ret = 0;

#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
								hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_DEINTERLACE]);
#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
								hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_DEINTERLACE]);
#endif
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1; 
	}

	GL_CHECK(glDeleteTextures(1,&pStatics->tex_deinterlace_ref_id));
	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_DEINTERLACE].iVertName	));
	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_DEINTERLACE].iFragName	));
	GL_CHECK(glDeleteProgram(pStatics->shader[VR_PROGRAM_DEINTERLACE].iProgName ));
	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "Deinterlace eglDestroyContext start, 32ctx\n");		
	vrResetShaderInfo(&pStatics->shader[VR_PROGRAM_DEINTERLACE]);

	ret = vrDestroyEGLContext(VR_PROGRAM_DEINTERLACE);	
	return ret;
}

static int vrDeinitializeScaler( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	int ret = 0;

#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
								hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_SCALE]);
#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
								hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_SCALE]);
#endif	
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1; 
	}

	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_SCALE].iVertName ));
	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_SCALE].iFragName ));
	GL_CHECK(glDeleteProgram(pStatics->shader[VR_PROGRAM_SCALE].iProgName ));
	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "Scale eglDestroyContext start, 8ctx\n");		
	vrResetShaderInfo(&pStatics->shader[VR_PROGRAM_SCALE]);
	
	ret = vrDestroyEGLContext(VR_PROGRAM_SCALE);	
	return ret;
}

static int vrDeinitializeCvt2Y( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	int ret = 0;

#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
								hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2Y]);
#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
								hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2Y]);
#endif		
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1; 
	}

	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_CVT2Y].iVertName	));
	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_CVT2Y].iFragName	));
	GL_CHECK(glDeleteProgram(pStatics->shader[VR_PROGRAM_CVT2Y].iProgName ));
	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "Cvt2Y eglDestroyContext start, 8ctx\n");	
	vrResetShaderInfo(&pStatics->shader[VR_PROGRAM_CVT2Y]);
	
	ret = vrDestroyEGLContext(VR_PROGRAM_CVT2Y);	
	return ret;
}

static int vrDeinitializeCvt2UV( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	int ret = 0;

#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
								hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2UV]);
#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
								hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2UV]);
#endif			
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1; 
	}
	
	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_CVT2UV].iVertName	));
	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_CVT2UV].iFragName	));
	GL_CHECK(glDeleteProgram(pStatics->shader[VR_PROGRAM_CVT2UV].iProgName ));
	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "Cvt2UV eglDestroyContext start, 16ctx\n");	
	vrResetShaderInfo(&pStatics->shader[VR_PROGRAM_CVT2UV]);

	ret = vrDestroyEGLContext(VR_PROGRAM_CVT2UV);	
	return ret;
}

static int vrDeinitializeCvt2Rgba( HSURFTARGET hTarget)
{
	Statics* pStatics = vrGetStatics();
	int ret = 0;
	
#ifdef VR_FEATURE_SEPERATE_FD_USE
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surface, 
								hTarget->target_pixmap_surface, pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2RGBA]);
#else
	EGLBoolean bResult = eglMakeCurrent(pStatics->egl_info.sEGLDisplay, hTarget->target_pixmap_surfaces[0], 
								hTarget->target_pixmap_surfaces[0], pStatics->egl_info.sEGLContext[VR_PROGRAM_CVT2RGBA]);
#endif			
	if(bResult == EGL_FALSE)
	{
		EGLint iError = eglGetError();
		ErrMsg("eglGetError(): %i (0x%.4x)\n", (int)iError, (int)iError);
		ErrMsg("Error: Failed to make context current at %s:%i\n", __FILE__, __LINE__);
		return -1; 
	}
	
	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_CVT2RGBA].iVertName	));
	GL_CHECK(glDeleteShader(pStatics->shader[VR_PROGRAM_CVT2RGBA].iFragName	));
	GL_CHECK(glDeleteProgram(pStatics->shader[VR_PROGRAM_CVT2RGBA].iProgName ));
	
	VR_INFO("", VR_GRAPHIC_DBG_TARGET, "Cvt2Rgba eglDestroyContext start, 32ctx\n");
	vrResetShaderInfo(&pStatics->shader[VR_PROGRAM_CVT2RGBA]);

	ret = vrDestroyEGLContext(VR_PROGRAM_CVT2RGBA);	
	return ret;
}
