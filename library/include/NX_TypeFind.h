#ifndef __NX_TypeFind_H__
#define __NX_TypeFind_H__

#define  MP_RESULT           int

#define	ON				1
#define	OFF				0


//	uri type
enum{
	URI_TYPE_FILE,
	URI_TYPE_URL
};


//	contents type
enum{
	CONTENTS_TYPE_MP4,
	CONTENTS_TYPE_H264,
	CONTENTS_TYPE_H263,
	CONTENTS_TYPE_FLV,
	CONTENTS_TYPE_RMVB,
	CONTENTS_TYPE_DIVX,
	CONTENTS_TYPE_WMV,
	CONTENTS_TYPE_MKV,
	CONTENTS_TYPE_THEROA,
	CONTENTS_TYPE_MP2,
};

//	demux type
enum{
	DEMUX_TYPE_MPEGTSDEMUX,	
	DEMUX_TYPE_QTDEMUX,
	DEMUX_TYPE_OGGDEMUX,
	DEMUX_TYPE_RMDEMUX,
	DEMUX_TYPE_AVIDEMUX,
	DEMUX_TYPE_ASFDEMUX,
	DEMUX_TYPE_MATROSKADEMUX,
	DEMUX_TYPE_FLVDEMUX,
	DEMUX_TYPE_MPEGPSDEMUX,
	TYPE_MP3,
	TYPE_FLAC,
	TYPE_M4A,
	TYPE_WAV,
	TYPE_MPEG,
	TYPE_AC3,
	TYPE_DTS,
	TYPE_3GP,
	DEMUX_TYPE_ANNODEX,		//oggdemux
};

//	audio codec type
enum{
	AUDIO_TYPE_MPEG,
	AUDIO_TYPE_MP3,
	AUDIO_TYPE_AAC,	//	AAC RAW Format
	AUDIO_TYPE_WMA,	//	
	AUDIO_TYPE_OGG,
	AUDIO_TYPE_AC3,
	AUDIO_TYPE_AC3_PRI,
	AUDIO_TYPE_FLAC,
	AUDIO_TYPE_RA,
	AUDIO_TYPE_DTS,
	AUDIO_TYPE_DTS_PRI,
	AUDIO_TYPE_WAV,
};

//	video codec type
enum{
	VIDEO_TYPE_H264,
	VIDEO_TYPE_H263,
	VIDEO_TYPE_MP4V,	//mpeg4 video
	VIDEO_TYPE_MP2V,	//mpeg2 video
	VIDEO_TYPE_FLV,
	VIDEO_TYPE_RV,		//realvideo
	VIDEO_TYPE_DIVX,
	VIDEO_TYPE_ASF,
	VIDEO_TYPE_WMV,
	VIDEO_TYPE_THEORA,
	VIDEO_TYPE_XVID
};

//	codec type
enum{
	CODEC_TYPE_VIDEO,
	CODEC_TYPE_AUDIO
};


//ErrorCode
enum{
	ERROR_NONE 				                    = 0,
	ERROR										= -1,
	ERROR_HANDLE		                 		= -2,
	ERROR_NOT_SUPPORT_CONTENTS          		= -3
}; 


typedef struct AUDIO_INFO {
	unsigned long		AudioTrackNum;		
	unsigned char       AudioPadName[20];
	signed long			ACodecType;	
	unsigned long		wmaversion;
	unsigned long		ADuration;	
	unsigned long		samplerate;	
	unsigned long		channels;	
	unsigned long		bitrate;	
} AUDIO_INFO;

typedef struct FRAMERATE_INFO {
	int value_numerator ;
	int value_denominator ;
} FRAMERATE_INFO;

typedef struct VIDEO_INFO {
	unsigned long		VideoTrackNum;		
	unsigned char       VideoPadName[20];
	signed long			VCodecType;			
	unsigned long		VDuration;
	FRAMERATE_INFO      Framerate;
	unsigned long		Width;	
	unsigned long		Height;	
} VIDEO_INFO;

#define MEDIA_MAX		6
typedef struct TYMEDIA_INFO {
	int					AudioOnly;
	int					VideoOnly;
	int					AudioOn;
	int					VideoOn;
	int					DemuxType;
	unsigned long		AudioTrackTotNum;		
	AUDIO_INFO			AudioInfo[MEDIA_MAX];
	unsigned long		VideoTrackTotNum;		
	VIDEO_INFO			VideoInfo[MEDIA_MAX];	
	//mpegts
	unsigned int program_tot_no;
	unsigned int program_no[MEDIA_MAX];
} TYMEDIA_INFO;

#ifdef __cplusplus
extern "C" {
#endif	//	__cplusplus

MP_RESULT NX_TypeFind_Open(TYMEDIA_INFO **ty_handle);
MP_RESULT NX_TypeFind( TYMEDIA_INFO *ty_media_handle,  const char *uri);
void NX_TypeFind_Close(TYMEDIA_INFO *ty_handle);
#ifdef __cplusplus
}
#endif

#endif // __NX_TypeFind_H__
