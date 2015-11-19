#ifndef __VR_COMMON_DEF__
#define __VR_COMMON_DEF__

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 
//	debugging mode
//
//#define VR_DEBUG

#define VR_TRUE 	1
#define VR_FALSE	0
#define VR_FAIL		-1

#define MALLOC_FUNC malloc
#define CALLOC_FUNC calloc
#define REALLOC_FUNC realloc
#define FREE_FUNC	free
#define MEMSET_FUNC memset
#define MEMCPY_FUNC memcpy

#define MALLOC(a) MALLOC_FUNC((a))
#define CALLOC(a,b) CALLOC_FUNC((a), (b))
#define REALLOC(a,b) REALLOC_FUNC((a), (b))
#define FREE(a) FREE_FUNC((a))
#define MEMSET(dst,val,size) MEMSET_FUNC((dst),(val),(size))
#define MEMCPY(dst,src,size) MEMCPY_FUNC((dst),(src),(size))

typedef unsigned long long VR_ULONG;


// 
//	debugging info: conditional run, not halt  
//
#if defined( __STDC_WANT_SECURE_LIB__ ) && ( 0 != __STDC_WANT_SECURE_LIB__ )
#define VSNPRINTF           vsnprintf_s
#else
#define VSNPRINTF           vsnprintf
#endif
#define VR_PRINTF			printf

#if defined(_MSC_VER)
#define VR_EXPLICIT_FALSE (1,0)
#else
#define VR_EXPLICIT_FALSE 0
#endif
#define VR_ASSERT(info, expr)			do { \
											if(!(expr)){ \
												VR_PRINTF("*ASSERT(%s): %s\n\n", info, #expr); \
												VR_PRINTF("func: %s(%d)\n\n", __FUNCTION__, __LINE__); \
												assert(!"VR_HALT!"); \
												__vr_base_dbg_halt(); \
											} \
										} while(VR_EXPLICIT_FALSE)
											
#if defined(VR_DEBUG)
#define VR_INFO(prefix, info_enable, msg, ...) 	\
										do { \
											if(info_enable ){	\
												VR_PRINTF(prefix "*I:"); \
												VR_PRINTF(msg, ##__VA_ARGS__); \
											} \
										} while(VR_EXPLICIT_FALSE)
#else
#define VR_INFO(prefix, info_enable, msg, ...) 	{}
#endif


#define VR_FEATURE_INPUT_EGLIMAGE_USE
#define VR_FEATURE_ION_ALLOC_USE
//#define VR_FEATURE_SHADER_FILE_USE
//#define VR_FEATURE_SEPERATE_FD_USE
#define VR_FEATURE_YCRCB_NV21_USE
#define VR_FEATURE_HEIGHT_ALIGN16_USE
#if !defined( VR_FEATURE_SEPERATE_FD_USE )
	#define VR_FEATURE_STRIDE_USE
	#define VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE	16
	
	#if 0 /* temp test */
	#define VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE 0
	#else /* org */
	#define VR_FEATURE_OUTPUT_HEIGHT_ALIGN_BYTE	VR_FEATURE_INPUT_HEIGHT_ALIGN_BYTE
	#endif
#endif

#ifdef VR_FEATURE_SHADER_FILE_USE
#define VERTEX_SHADER_SOURCE_DEINTERLACE		"res/deinterlace.vert"
#define FRAGMENT_SHADER_SOURCE_DEINTERLACE		"res/deinterlace.frag"
#define VERTEX_SHADER_SOURCE_SCALE				"res/scale.vert"
#define FRAGMENT_SHADER_SOURCE_SCALE			"res/scale.frag"
#define VERTEX_SHADER_SOURCE_CVT2Y				"res/cvt2y.vert"
#define FRAGMENT_SHADER_SOURCE_CVT2Y			"res/cvt2y.frag"
#define VERTEX_SHADER_SOURCE_CVT2UV				"res/cvt2uv.vert"
#define FRAGMENT_SHADER_SOURCE_CVT2UV			"res/cvt2uv.frag"
#define VERTEX_SHADER_SOURCE_CVT2RGBA			"res/cvt2rgba.vert"
#define FRAGMENT_SHADER_SOURCE_CVT2RGBA			"res/cvt2rgba.frag"
#else
#define VERTEX_SHADER_SOURCE_DEINTERLACE		deinterace_vertex_shader
#define FRAGMENT_SHADER_SOURCE_DEINTERLACE		deinterace_frag_shader
#define VERTEX_SHADER_SOURCE_SCALE				scaler_vertex_shader
#define FRAGMENT_SHADER_SOURCE_SCALE			scaler_frag_shader
#define VERTEX_SHADER_SOURCE_CVT2Y				cvt2y_vertex_shader
#define FRAGMENT_SHADER_SOURCE_CVT2Y			cvt2y_frag_shader
#define VERTEX_SHADER_SOURCE_CVT2UV				cvt2uv_vertex_shader
#define FRAGMENT_SHADER_SOURCE_CVT2UV			cvt2uv_frag_shader
#define VERTEX_SHADER_SOURCE_CVT2RGBA			cvt2rgba_vertex_shader
#define FRAGMENT_SHADER_SOURCE_CVT2RGBA			cvt2rgba_frag_shader
#endif

//program
enum{
	VR_PROGRAM_DEINTERLACE,
	VR_PROGRAM_SCALE,
	VR_PROGRAM_CVT2UV,
	VR_PROGRAM_CVT2Y,
	VR_PROGRAM_CVT2RGBA,
	VR_PROGRAM_MAX
};

//texture unit
typedef enum{
	VR_INPUT_MODE_DEINTERLACE,
	VR_INPUT_MODE_DEINTERLACE_REF,
	VR_INPUT_MODE_TEXTURE0,
	VR_INPUT_MODE_TEXTURE1,
	VR_INPUT_MODE_MAX
}VRInputMode;

//YUV texture unit 
typedef enum{
	VR_INPUT_MODE_Y,
	VR_INPUT_MODE_U,
	VR_INPUT_MODE_V,
	VR_INPUT_MODE_YUV_MAX
}VRInputYUVMode;

//format 
typedef enum{
	VR_IMAGE_FORMAT_RGBA,
	VR_IMAGE_FORMAT_Y,
	VR_IMAGE_FORMAT_U,
	VR_IMAGE_FORMAT_V,
	VR_IMAGE_FORMAT_UV,
	VR_IMAGE_FORMAT_YUV,
	VR_IMAGE_FORMAT_MAX
}VRImageFormatMode;

extern int VR_GRAPHIC_DBG_OPEN_CLOSE;
extern int VR_GRAPHIC_DBG_TARGET;
extern int VR_GRAPHIC_DBG_CTX;

#endif /* __VR_COMMON_DEF__ */

