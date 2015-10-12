#include "vr_common_def.h"
#include "vr_deinterlace_private.h"


const char deinterace_vertex_shader[] = {
"	#version 100													\n\
																	\n\
	precision highp float;											\n\
																	\n\
	attribute vec4 a_v4Position;									\n\
	attribute vec2 a_v2TexCoord;									\n\
	uniform float u_fTexHeight;										\n\
																	\n\
	varying vec2 v_tex0;											\n\
	varying vec2 v_tex1;											\n\
	varying vec2 v_tex2;											\n\
	varying vec2 v_tex3;											\n\
	varying vec2 v_tex4;											\n\
	varying vec2 v_offset_y;										\n\
																	\n\
	void main()														\n\
	{																\n\
		float size_of_texel = 1.0/u_fTexHeight;						\n\
																	\n\
		v_offset_y = a_v2TexCoord * vec2(1.0,(u_fTexHeight/2.0));	\n\
																	\n\
		v_tex0 = a_v2TexCoord + vec2(0.0,(size_of_texel*-2.0));		\n\
		v_tex1 = a_v2TexCoord + vec2(0.0,(size_of_texel*-1.0));		\n\
		v_tex2 = a_v2TexCoord;										\n\
		v_tex3 = a_v2TexCoord + vec2(0.0,(size_of_texel* 1.0));		\n\
		v_tex4 = a_v2TexCoord + vec2(0.0,(size_of_texel* 2.0));		\n\
																	\n\
		gl_Position = a_v4Position;									\n\
	}																\n\
"
};

#if 1
const char deinterace_frag_shader[] = {
" 	#version 100																	\n\
																					\n\
	//precision mediump float;														\n\
	precision highp float;															\n\
																					\n\
	uniform float u_fTexHeight;														\n\
	uniform sampler2D diffuse;														\n\
	uniform sampler2D ref_tex;														\n\
																					\n\
	varying vec2 v_tex0;															\n\
	varying vec2 v_tex1;															\n\
	varying vec2 v_tex2;															\n\
	varying vec2 v_tex3;															\n\
	varying vec2 v_tex4;															\n\
	varying vec2 v_offset_y;														\n\
																					\n\
	void main()																		\n\
	{																				\n\
		vec4 tval0 = vec4(0.0, 0.0, 0.0, 0.0), tval1 = vec4(0.0, 0.0, 0.0, 0.0);	\n\
																					\n\
		//	 deinterface without scaling											\n\
		vec4 y0_frac = texture2D(ref_tex, v_offset_y);								\n\
		if( y0_frac.x == 1.0 )														\n\
		{																			\n\
			tval0 += texture2D(diffuse, v_tex0) * (-1.0/8.0);						\n\
			tval0 += texture2D(diffuse, v_tex1) * ( 4.0/8.0);						\n\
			tval0 += texture2D(diffuse, v_tex2) * ( 2.0/8.0);						\n\
			tval0 += texture2D(diffuse, v_tex3) * ( 4.0/8.0);						\n\
			tval0 += texture2D(diffuse, v_tex4) * (-1.0/8.0);						\n\
		}																			\n\
		else //even																	\n\
		{																			\n\
			tval0 = texture2D(diffuse, v_tex2);										\n\
		}																			\n\
																					\n\
		gl_FragColor = tval0;														\n\
																					\n\
		//for debugging																\n\
		//gl_FragColor = texture2D(diffuse, v_v2TexCoord.xy);						\n\
	}																				\n\
"
};
#else
const char deinterace_frag_shader[] = {
" 	#version 100																	\n\
																					\n\
	//precision mediump float;														\n\
	precision highp float;															\n\
																					\n\
	uniform float u_fTexHeight;														\n\
	uniform sampler2D diffuse;														\n\
	uniform sampler2D ref_tex;														\n\
																					\n\
	varying vec2 v_tex0;															\n\
	varying vec2 v_tex1;															\n\
	varying vec2 v_tex2;															\n\
	varying vec2 v_tex3;															\n\
	varying vec2 v_tex4;															\n\
	varying vec2 v_offset_y;														\n\
																					\n\
	void main()																		\n\
	{																				\n\
		vec4 tval0 = vec4(0.0, 0.0, 0.0, 0.0), tval1 = vec4(0.0, 0.0, 0.0, 0.0);	\n\
																					\n\
		//	 deinterface without scaling											\n\
		vec4 y0_frac = texture2D(ref_tex, v_offset_y);								\n\
		if( y0_frac.x == 1.0 )														\n\
		{																			\n\
			tval0 += texture2D(diffuse, v_tex1) * ( 1.0/2.0);						\n\
			tval0 += texture2D(diffuse, v_tex3) * ( 1.0/2.0);						\n\
		}																			\n\
		else //even																	\n\
		{																			\n\
			tval0 = texture2D(diffuse, v_tex2);										\n\
		}																			\n\
																					\n\
		gl_FragColor = tval0;														\n\
																					\n\
		//for debugging																\n\
		//gl_FragColor = texture2D(diffuse, v_v2TexCoord.xy);						\n\
	}																				\n\
"
};
#endif

const char scaler_vertex_shader[] = {
"									\n\
	#version 100					\n\
									\n\
	precision highp float;			\n\
									\n\
	attribute vec4 a_v4Position;	\n\
	attribute vec2 a_v2TexCoord;	\n\
	varying vec2 v_tex;				\n\
									\n\
	void main()						\n\
	{								\n\
		v_tex = a_v2TexCoord;		\n\
		gl_Position = a_v4Position;	\n\
	}								\n\
"
};

const char scaler_frag_shader[] = {
"													\n\
	#version 100									\n\
													\n\
	precision highp float;							\n\
	uniform sampler2D diffuse;						\n\
	varying vec2 v_tex;								\n\
													\n\
	void main()										\n\
	{												\n\
		gl_FragColor = texture2D(diffuse, v_tex);	\n\
		//vec4 tval0 = vec4(1.0, 1.0, 1.0, 1.0); \n\
		//gl_FragColor = tval0; \n\
	}												\n\
"
};

const char cvt2y_vertex_shader[] = {
"									\n\
	#version 100					\n\
									\n\
	precision highp float;			\n\
									\n\
	attribute vec4 a_v4Position;	\n\
	attribute vec2 a_v2TexCoord;	\n\
	varying vec2 v_tex;				\n\
									\n\
	void main()						\n\
	{								\n\
		v_tex = a_v2TexCoord;		\n\
		gl_Position = a_v4Position;	\n\
	}								\n\
"
};

//compile error =>precision highp float;
const char cvt2y_frag_shader[] = {
"													\n\
	#version 100									\n\
													\n\
	precision mediump float;						\n\
	uniform sampler2D diffuse;						\n\
	varying vec2 v_tex;								\n\
													\n\
	void main()										\n\
	{												\n\
		vec4 tval;									\n\
		float color;									\n\
		tval = texture2D(diffuse, v_tex);	\n\
		color = (0.257*tval.x) + (0.504*tval.y) + (0.098*tval.z) + 0.0625; \n\
		gl_FragColor = vec4(color, color, color, color); \n\
	}												\n\
"
};

const char cvt2uv_vertex_shader[] = {
"									\n\
	#version 100					\n\
									\n\
	precision highp float;			\n\
									\n\
	attribute vec4 a_v4Position;	\n\
	attribute vec2 a_v2TexCoord;	\n\
	varying vec2 v_tex;				\n\
									\n\
	void main()						\n\
	{								\n\
		v_tex = a_v2TexCoord;		\n\
		gl_Position = a_v4Position;	\n\
	}								\n\
"
};

#ifdef VR_FEATURE_YCRCB_NV21_USE
//compile error =>precision highp float;
//NV12 : Y,U/V
//color.b = Cb(U)
//color.g = Cr(V)
const char cvt2uv_frag_shader[] = {
"													\n\
	#version 100									\n\
													\n\
	precision mediump float;						\n\
	uniform sampler2D diffuse;						\n\
	varying vec2 v_tex;								\n\
													\n\
	void main()										\n\
	{												\n\
		vec4 tval;	\n\
		vec4 color = vec4(0.0, 0.0, 0.0, 0.0);	\n\
		tval = texture2D(diffuse, v_tex);	\n\
		color.b = -(0.148*tval.x) - (0.291*tval.y) + (0.439*tval.z) + 0.5; \n\
		color.g = (0.439*tval.x) - (0.368*tval.y) - (0.071*tval.z) + 0.5; \n\
		gl_FragColor = color; \n\
	}												\n\
"
};
#else
//NV21 : Y,V/U
//color.g = Cr(V)
//color.b = Cb(U)
const char cvt2uv_frag_shader[] = {
"													\n\
	#version 100									\n\
													\n\
	precision mediump float;						\n\
	uniform sampler2D diffuse;						\n\
	varying vec2 v_tex;								\n\
													\n\
	void main()										\n\
	{												\n\
		vec4 tval;	\n\
		vec4 color = vec4(0.0, 0.0, 0.0, 0.0);	\n\
		tval = texture2D(diffuse, v_tex);	\n\
		color.g = -(0.148*tval.x) - (0.291*tval.y) + (0.439*tval.z) + 0.5; \n\
		color.b = (0.439*tval.x) - (0.368*tval.y) - (0.071*tval.z) + 0.5; \n\
		gl_FragColor = color; \n\
	}												\n\
"
};
#endif

const char cvt2rgba_vertex_shader[] = {
"									\n\
	#version 100					\n\
									\n\
	precision highp float;			\n\
									\n\
	attribute vec4 a_v4Position;	\n\
	attribute vec2 a_v2TexCoordY;	\n\
	attribute vec2 a_v2TexCoordU;	\n\
	attribute vec2 a_v2TexCoordV;	\n\
	varying vec2 v_texY;			\n\
	varying vec2 v_texU; 			\n\
	varying vec2 v_texV; 			\n\
									\n\
	void main()						\n\
	{								\n\
		v_texY = a_v2TexCoordY;		\n\
		v_texU = a_v2TexCoordU;		\n\
		v_texV = a_v2TexCoordV;		\n\
		gl_Position = a_v4Position;	\n\
	}								\n\
"
};

//compile error =>precision highp float;
const char cvt2rgba_frag_shader[] = {
"													\n\
	#version 100									\n\
													\n\
	precision mediump float;						\n\
	uniform sampler2D diffuseY;						\n\
	uniform sampler2D diffuseU;						\n\
	uniform sampler2D diffuseV;						\n\
	varying vec2 v_texY;							\n\
	varying vec2 v_texU;							\n\
	varying vec2 v_texV;							\n\
													\n\
	void main()										\n\
	{												\n\
		float tvalY, tvalU, tvalV;	\n\
		vec4 color = vec4(0.0, 0.0, 0.0, 0.0);	\n\
		tvalY = texture2D(diffuseY, v_texY).x;	\n\
		tvalU = texture2D(diffuseU, v_texU).x;	\n\
		tvalV = texture2D(diffuseV, v_texV).x;	\n\
		color.r = 1.164 * (tvalY - 0.0625) + 1.596 * (tvalV - 0.5); \n\
		color.g = 1.164 * (tvalY - 0.0625) - 0.813 * (tvalV - 0.5) - 0.391 * (tvalU - 0.5); \n\
		color.b = 1.164 * (tvalY - 0.0625) + 2.018 * (tvalU - 0.5); \n\
		color.a = 0.0; \n\
		gl_FragColor = color; \n\
	}												\n\
"
};


