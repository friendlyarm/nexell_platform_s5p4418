#ifndef __VR_EGL_RUNTIME__
#define __VR_EGL_RUNTIME__

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "vr_common_def.h"

#ifdef VR_DEBUG
#define EGL_CHECK(x) \
	x; \
	{ \
		EGLint eglError = eglGetError(); \
		if(eglError != EGL_SUCCESS) { \
			VR_PRINTF("eglGetError() = %i (0x%.8x) at %s:%i\n", eglError, eglError, __FILE__, __LINE__); \
			exit(1); \
		} \
	}

#define GL_CHECK(x) \
	x; \
	{ \
		GLenum glError = glGetError(); \
		if(glError != GL_NO_ERROR) { \
			VR_PRINTF( "glGetError() = %i (0x%.8x) at %s:%i\n", glError, glError, __FILE__, __LINE__); \
			exit(1); \
		} \
	}
#else
#define EGL_CHECK(x) 	x
#define GL_CHECK(x)		x
#endif

typedef struct
{
    EGLDisplay sEGLDisplay;
	EGLContext sEGLContext[VR_PROGRAM_MAX];
	EGLConfig sEGLConfig[VR_PROGRAM_MAX];
	unsigned int sEGLContextRef[VR_PROGRAM_MAX];
} EGLInfo;

extern PFNEGLCREATEIMAGEKHRPROC _eglCreateImageKHR;
extern PFNEGLDESTROYIMAGEKHRPROC _eglDestroyImageKHR;
extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC _glEGLImageTargetTexture2DOES;
extern PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC _glEGLImageTargetRenderbufferStorageOES;

/**
 * Initializes extensions required for EGLImage
 *
 * @return 0 on success, non-0 on failure
 */
int vrInitEglExtensions(void);
int vrInitializeEGLConfig(void);
int vrCreateEGLContext(unsigned int program);
int vrDestroyEGLContext(unsigned int program);
int vrTerminateEGL(void);
EGLNativePixmapType vrCreatePixmap(unsigned int uiWidth, unsigned int uiHeight, void* pData, int is_video_dma_buf, 
									unsigned int pixel_bits, VRImageFormatMode format);
void vrDestroyPixmap(EGLNativePixmapType pPixmap);
void __vr_base_dbg_halt( void );
VR_ULONG base_util_time_get_usec(void);

#endif /* __VR_EGL_RUNTIME__ */

