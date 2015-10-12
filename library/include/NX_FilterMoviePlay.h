#ifndef __NX_FilterMoviePlay__
#define __NX_FilterMoviePlay__

typedef struct MOVIE_TYPE	*MP_HANDLE;
#define  MP_RESULT           int32_t

//	uri type
enum{
	URI_TYPE_FILE,
	URI_TYPE_URL
};

//	disply port
enum{
	DISPLAY_LCD,
	DISPLAY_HDMI,
	DISPLAY_DUAL
};

//	disply module
enum{
	DISPLAY_MLC0,
	DISPLAY_MLC1
};

enum{
	CALLBACK_MSG_EOS			= 0x1000,
	CALLBACK_MSG_PLAY_ERR		= 0x8001,
};


//ErrorCode
enum{
	ERROR_NONE 				                    = 0,
	ERROR										= -1,
	ERROR_HANDLE		                 		= -2,
	ERROR_NOT_SUPPORT_CONTENTS          		= -3
}; 

typedef struct _Audio_Info {
	int32_t		AudioTrackNum;
	int32_t		ACodecID;
	int32_t		samplerate;
	int32_t		channels;
} Audio_Info;

typedef struct _Video_Info {
	int32_t		VideoTrackNum;
	int32_t		VCodecID;
	int32_t		Width;
	int32_t		Height;
} Video_Info;

#define MEDIA_MAX		5
typedef struct _Media_Info{
	int32_t		AudioTrackTotNum;
	Audio_Info	AudioInfo[MEDIA_MAX];
	int32_t		VideoTrackTotNum;
	Video_Info	VideoInfo[MEDIA_MAX];
	int32_t		DataTrackTotNum;
	int64_t		Duration;
} Media_Info;


#ifdef __cplusplus
extern "C" {
#endif	//	__cplusplus

	MP_RESULT NX_MPSetFileName(MP_HANDLE *phandle, const char *uri, Media_Info *media_info);
#ifdef ANDROID
	MP_RESULT NX_MPOpen(MP_HANDLE handle, int32_t select_audio_track_num, int select_video_track_num, int pip,
		void *pSurface_1, void *pSurface_2, void *pNull,
		void(*cbEvent)(void *Obj, unsigned int EventType, unsigned int EventData, unsigned int parm), void *cbPrivate
		);
#else
	MP_RESULT NX_MPOpen(MP_HANDLE handle, int audio_track_num, int video_track_num, int display,
		void *volumem, void *dspModule, void *dspPort,
		void(*cbEvent)(void *Obj, unsigned int EventType, unsigned int EventData, unsigned int parm), void *cbPrivate);
#endif

	void NX_MPClose(MP_HANDLE handle);
	MP_RESULT NX_MPGetMediaInfo(MP_HANDLE handle, int index, void *pInfo);
	MP_RESULT NX_MPPlay(MP_HANDLE handle, float speed);
	MP_RESULT NX_MPPause(MP_HANDLE hande);
	MP_RESULT NX_MPStop(MP_HANDLE hande);
	MP_RESULT NX_MPSeek(MP_HANDLE hande, unsigned int seekTime);				//seekTime : msec
	MP_RESULT NX_MPGetCurDuration(MP_HANDLE handle, unsigned int *duration);	//duration : msec
	MP_RESULT NX_MPGetCurPosition(MP_HANDLE handle, unsigned int *position);	//position : msec	
#ifdef ANDROID

#else
	MP_RESULT NX_MPSetDspPosition(MP_HANDLE handle, int dspModule, int dsPport, int x, int y, int width, int height);
	MP_RESULT NX_MPSetVolume(MP_HANDLE handle, int volume);						//volume range : 0 ~ 100
#endif

#ifdef __cplusplus
}
#endif

#endif // __NX_FilterMoviePlay_H__
