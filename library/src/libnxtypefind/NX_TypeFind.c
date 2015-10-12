 #pragma GCC diagnostic ignored "-Wcast-align"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gst/gst.h>
#include <glib.h>
#include <NX_TypeFind.h>


//	Debug Flags
#define	DBG_MSG				1
#define	VBS_MSG				0
#define	DBG_FUNCTION		0
#define	DEB_MESSAGE			0
#define	ERR_BREAK			0

#if VBS_MSG
#define VbsMsg				g_print
#else
#define	VbsMsg(FORMAT,...)	do{}while(0)
#endif	//	VBS_MSG


#ifndef	DTAG
#define	DTAG	"[TypeFind] "
#endif

#if	DBG_MSG

//#define	DbgMsg				g_print
#define DbgMsg(fmt...)		if(1) do{					\
										g_print("%s" , DTAG);		\
										g_print(fmt);				\
									}while(0)

#if ERR_BREAK
#define	ErrMsg				g_error			//	critical error
#else
#define	ErrMsg				g_printerr			//	critical error
#endif


#if	DBG_FUNCTION
#define	FUNC_IN()			g_print("%s() In\n", __func__)
#define	FUNC_OUT()			g_print("%s() Out\n", __func__)
#else
#define	FUNC_IN()			do{}while(0)
#define	FUNC_OUT()			do{}while(0)
#endif	//	DBG_FUNCTION

#else	//	!DBG_MSG

#define	ErrMsg				g_printerr			//	critical error

#define	DbgMsg(FORMAT,...)	do{}while(0)
#define	VbsMsg(FORMAT,...)	do{}while(0)
#define	FUNC_IN()			do{}while(0)
#define	FUNC_OUT()			do{}while(0)

#endif	//	DBG_MSG


//	uri type
//static const char *URITypeString[] =
//{
//	"URI_TYPE_FILE",
//	"URI_TYPE_URL"
//};

#define DEMUX_TYPE_NUM	20
static const char *DemuxTypeString[DEMUX_TYPE_NUM] =
{
	"video/mpegts",						//mpegtsdemux
	"video/quicktime",					//qtdemux
	"application/ogg",					//oggdemux
	"application/vnd.rn-realmedia",		//rmdemux
	"video/x-msvideo",					//avidemux
	"video/x-ms-asf",					//asfdemux
	"video/x-matroska",					//matroskademux
	"video/x-flv",						//flvdemux
	"video/mpeg",						//mpegpsdemux	
	"application/x-id3",				//audio mp3
	"audio/x-flac",						//audio flac
	"audio/x-m4a",						//audio m4a
	"audio/x-wav",						//audio wav
	"audio/mpeg",						//audio mpeg
	"audio/x-ac3",						//audio ac3
	"audio/x-dts",						//audio dts
	"application/x-3gp",
	"application/x-annodex",			//oggdemux
	"NULL",
	"NULL",
};

#define AUDIO_TYPE_NUM	15
static const char *AudioTypeString[AUDIO_TYPE_NUM] =
{
	"audio/mpeg",					//AUDIO_TYPE_MPEG
	"audio/mp3",					//AUDIO_TYPE_MP3
	"audio/aac",					//AUDIO_TYPE_AAC	 mpeg4 lc
	"audio/x-wma",					//AUDIO_TYPE_WMA
	"audio/x-vorbis",				//AUDIO_TYPE_OGG
	"audio/x-ac3",					//AUDIO_TYPE_AC3
	"audio/x-private1-ac3",			//AUDIO_TYPE_AC3_PRI
	"audio/x-flac",					//AUDIO_TYPE_FLAC
	"audio/x-pn-realaudio",			//AUDIO_TYPE_RA		realvideo
	"audio/x-dts",
	"audio/x-private1-dts",
	"audio/x-wav",
	"NULL",
	"NULL",
	"NULL",
};


#define VIDEO_TYPE_NUM	12
static const char *VideoTypeString[VIDEO_TYPE_NUM] =
{
	"video/x-h264",					//VIDEO_TYPE_H264
	"video/x-h263",					//VIDEO_TYPE_H263
	"video/mpeg",					//VIDEO_TYPE_MP4V	mpeg4 video
	"video/mpeg",					//VIDEO_TYPE_MP2V	mpeg2 video
	"video/x-flash-video",			//VIDEO_TYPE_FLV
	"video/x-pn-realvideo",			//VIDEO_TYPE_RV		realvideo
	"video/x-divx",					//VIDEO_TYPE_DIVX
	"video/x-ms-asf",				//VIDEO_TYPE_ASF
	"video/x-wmv",					//VIDEO_TYPE_WMV
	"video/x-theora",				//VIDEO_TYPE_THEORA
	"video/x-xvid",
	"NULL"
};

typedef struct TypeFindSt {
	GMainLoop *loop;
	GstBus *bus;
	GstElement *pipeline;
	GstElement *filesrc;
	GstElement *filesrc_typefind;
	GstElement *demux;
	GstElement *decode;
	GstElement *audio_decode;
	GstElement *video_decode;
	GstElement *video_parse;
	GstElement *audio_parse;
	GstElement *video_queue;
	GstElement *audio_queue;
	GstElement *temp_queue;
	GstElement *video_typefind;
	GstElement *video_fakesink;
	GstElement *audio_typefind;
	GstElement *audio_fakesink;
	gint		codec_type;		//0:video, 1:audio
	gint		audio_track_num;
	gint		video_track_num;
	gint		audio_track_on;
	gint		video_track_on;
	TYMEDIA_INFO *TymediaInfo;
} TypeFindSt;


static gboolean idle_exit_loop (gpointer data);
static gboolean bus_callback( GstBus *bus, GstMessage *msg, TypeFindSt *ty_handle );
static void on_demux_pad_added_typefind(GstElement *element, GstPad *pad, TypeFindSt *ty_handle);
static void cb_typefind_demux (GstElement *typefind, guint probability, GstCaps *caps, gpointer data);
static void cb_typefind_audio (GstElement *typefind, guint probability,	GstCaps *caps, gpointer data);
static void cb_typefind_video (GstElement *typefind, guint probability,	GstCaps *caps, gpointer data);
static int typefind_demux( TYMEDIA_INFO *ty_media_handle, const char *uri );
static int typefind_codec_info( TYMEDIA_INFO *ty_media_handle, const char *uri, gint codec_type, gint track_num );
static void on_demux_pad_added_num(GstElement *element, GstPad *pad, TypeFindSt *ty_handle);
static int find_avcodec_num( TYMEDIA_INFO *ty_media_handle, const char *uri );
static void on_audio_decodebin_pad_added(GstElement *element, GstPad *pad, gpointer data);
static void on_video_decodebin_pad_added(GstElement *element, GstPad *pad, gpointer data);
static void cb_typefind_audio_only (GstElement *typefind, guint probability, GstCaps *caps, gpointer data);
static int typefind_audio_codec_info( TYMEDIA_INFO *ty_media_handle, const char *uri, gint track_num );

static void on_pad_added (GstElement *element, GstPad *pad, gpointer data);
static void dump_descriptors (GValueArray *descriptors);
static void dump_languages (GValueArray *languages);
static void demuxer_notify_pat_info (GObject *obj, GParamSpec *pspec, gpointer data);
static void demuxer_notify_pmt_info (GObject *obj, GParamSpec *pspec, gpointer data);


//mpegts
static void on_pad_added (GstElement *element,
						  GstPad     *pad,
						  gpointer    data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *) data;

  sinkpad = gst_element_get_static_pad (decoder, "sink");
  if (!gst_pad_is_linked (sinkpad))
    gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}


static void dump_descriptors (GValueArray *descriptors)
{
  GValue * value;
  gint i;
  for (i = 0 ; i < (gint)descriptors->n_values; i++) {
    GString *string;
    value = g_value_array_get_nth (descriptors, (guint)i);
    string = (GString *) g_value_get_boxed (value);
    
    if (string->len > 2) {
//      g_print ("descriptor # %d tag %02x len %d\n",
//          i + 1, (guint8)string->str[0], (guint8)string->str[1]);
      gst_util_dump_mem ((guint8*)string->str + 2, string->len - 2);    
    }
  }  
//  g_print ("\n");
}

static void dump_languages (GValueArray *languages)
{
  GValue * value;
  gint i;
  if (!languages->n_values)
    return;
 
//  g_print ("languages: ");
  for (i = 0 ; i < (gint)languages->n_values; i++) {
    const gchar *string;
    value = g_value_array_get_nth (languages, (guint)i);
    string = g_value_get_string (value);    
//    g_print ("%s,", string);
  }  
//  g_print ("\n");
}

static void demuxer_notify_pat_info (GObject *obj, GParamSpec *pspec, gpointer data)
{
  GValueArray *patinfo = NULL;
  GValue * value = NULL;
  GObject *entry = NULL;
  guint program, pid;
  gint i;
  TypeFindSt *ty_handle = data;

  g_object_get (obj, "pat-info", &patinfo, NULL);
  
  ty_handle->TymediaInfo->program_tot_no = patinfo->n_values;

//  g_print ("PAT: entries: %d\n", patinfo->n_values);  
  for (i = 0; i < (gint)patinfo->n_values; i++) {
    value = g_value_array_get_nth (patinfo, (guint)i);
    entry = (GObject*) g_value_get_object (value);
    g_object_get (entry, "program-number", &program, NULL);
    g_object_get (entry, "pid", &pid, NULL);
//    g_print ("    program: %04x pid: %04x\n", program, pid);
	ty_handle->TymediaInfo->program_no[i] = program;
  }

  ty_handle->TymediaInfo->VideoTrackTotNum = ty_handle->TymediaInfo->program_tot_no;
}

static void demuxer_notify_pmt_info (GObject *obj, GParamSpec *pspec, gpointer data)
{
  GObject *pmtinfo = NULL, *streaminfo = NULL;
  GValueArray *streaminfos = NULL;
  GValueArray *descriptors = NULL;
  GValueArray *languages = NULL;
  gint i;
  GValue * value;
  guint program, version, pcr_pid, es_pid, es_type;
  GMainLoop *loop = NULL;
  TypeFindSt *ty_handle = data;

  loop = ty_handle->loop;
  
  g_object_get (obj, "pmt-info", &pmtinfo, NULL);
  g_object_get (pmtinfo, "program-number", &program, NULL);
  g_object_get (pmtinfo, "version-number", &version, NULL);
  g_object_get (pmtinfo, "pcr-pid", &pcr_pid, NULL);
  g_object_get (pmtinfo, "stream-info", &streaminfos, NULL);
  g_object_get (pmtinfo, "descriptors", &descriptors, NULL);

//  g_print ("PMT: program: %04x version: %d pcr: %04x streams: %d "
//      "descriptors: %d\n",
//      (guint16)program, version, (guint16)pcr_pid, streaminfos->n_values,
//      descriptors->n_values);

  dump_descriptors (descriptors);
  for (i = 0 ; i < (gint)streaminfos->n_values; i++) {
    value = g_value_array_get_nth (streaminfos, (guint)i);
    streaminfo = (GObject*) g_value_get_object (value);
    g_object_get (streaminfo, "pid", &es_pid, NULL);
    g_object_get (streaminfo, "stream-type", &es_type, NULL);
    g_object_get (streaminfo, "languages", &languages, NULL);
    g_object_get (streaminfo, "descriptors", &descriptors, NULL);
//    g_print ("pid: %04x type: %x languages: %d descriptors: %d\n",
//        (guint16)es_pid, (guint8) es_type, languages->n_values,
//        descriptors->n_values);
    dump_languages (languages);
    dump_descriptors (descriptors);
  }

  g_idle_add (idle_exit_loop, loop);
}

//


static gboolean idle_exit_loop (gpointer data)
{
  g_main_loop_quit ((GMainLoop *) data);

  /* once */
  return FALSE;
}

static gboolean bus_callback( GstBus *bus, GstMessage *msg, TypeFindSt *ty_handle )
{
	gchar  *debug;
	GError *err;

	switch (GST_MESSAGE_TYPE (msg))
	{
		case GST_MESSAGE_EOS:
			g_print ("End of stream\n");
			break;

		case GST_MESSAGE_WARNING:
			gst_message_parse_warning(msg, &err, &debug);
			g_warning("GST warning: %s", err->message);
			g_error_free(err);
			g_free(debug);
			break;

		case GST_MESSAGE_STATE_CHANGED:
		{
			GstState old_state, new_state;
			gst_message_parse_state_changed (msg, &old_state, &new_state, NULL);
			break;
		}
		case GST_MESSAGE_ERROR:
			gst_message_parse_error (msg, &err, &debug);
			g_free (debug);
			ErrMsg ("Error: domain=%d message:%s\n", err->domain, err->message);
			g_error_free (err);
			break;
		default:
#if DEB_MESSAGE
			DbgMsg("default : message type:0x%08x(%d)\n", GST_MESSAGE_TYPE (msg), GST_MESSAGE_TYPE (msg));
#endif
			break;
	}

	FUNC_OUT();

	return TRUE;
}

static void on_demux_pad_added_typefind(GstElement *element, GstPad *pad, TypeFindSt *ty_handle)
{
	GMainLoop *loop = NULL;
	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPadLinkReturn rc;
	GstElement *targetqueue;
	GstPad *targetsink;
	gint i = 0, len = 0;
	gint tot_num = 0;
//	gint video_track_num = 0;
//	gint audio_track_num = 0;
	gint audio_on = OFF;
	gint video_on = OFF;
	gint codec_type = ty_handle->codec_type;

	FUNC_IN();

	loop = ty_handle->loop;

	//DbgMsg("===on_demux_pad_added_typefind ++\n");

//	video_track_num = ty_handle->video_track_num ;
//	audio_track_num = ty_handle->audio_track_num ;

	caps = gst_pad_get_caps(pad);
	g_assert(caps != NULL);
	name = gst_pad_get_name(pad);
	//DbgMsg("new demux pad %s\n", name);

	if( (0 == strcasecmp( name, "private_2" ) ) )
	{
		goto EXIT;
	}

	str = gst_caps_get_structure(caps, 0);
	g_assert(str != NULL);

	//DbgMsg("compare string %s\n", gst_structure_get_name(str));

	targetqueue = NULL;
	if (g_strrstr(gst_structure_get_name(str), "video")) {
		ty_handle->video_track_on = ON;
	}
	else if (g_strrstr(gst_structure_get_name(str), "audio")) {
		ty_handle->audio_track_on = ON;
	}

	// TODO: is this the right way to match video/audio pads

	if(CODEC_TYPE_VIDEO == codec_type)
	{ 
		if (g_strrstr(gst_structure_get_name(str), "video")) {
			//DbgMsg("--Linking %s to %s\n", name, gst_structure_get_name(str) );
			targetqueue = ty_handle->video_queue;
			ty_handle->TymediaInfo->VideoTrackTotNum++;
			tot_num = ty_handle->TymediaInfo->VideoTrackTotNum - 1;
			len = strlen(name);
			memcpy(ty_handle->TymediaInfo->VideoInfo[tot_num].VideoPadName, name, len);
			ty_handle->TymediaInfo->VideoInfo[tot_num].VideoPadName[len] = 0;
			video_on = ON;
			ty_handle->TymediaInfo->VideoOn = ON;

			ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType = -1;

			for(i = 0; i <VIDEO_TYPE_NUM ; i++)
			{
				len = strlen(VideoTypeString[i]);

				if( 0 == strncmp(gst_structure_get_name(str), VideoTypeString[i], len) ){
					ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType = i;
					break;
				}			
			}

			if(ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType == -1)
			{
				g_print("Not Support Video Codec\n");
			}
			//DbgMsg("=video_type = %d,  %s\n",ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType, VideoTypeString[ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType]);
			//DbgMsg("=VideoTrackTotNum = %d\n",ty_handle->TymediaInfo->VideoTrackTotNum);

		}
	}
	else
	{  
		if (g_strrstr(gst_structure_get_name(str), "audio")) {
			gchar *type = NULL;
			const GValue *value;
			
			//DbgMsg("--Linking %s to %s\n", name, gst_structure_get_name(str) );
			targetqueue = ty_handle->audio_queue;
			ty_handle->TymediaInfo->AudioTrackTotNum++;
			tot_num = ty_handle->TymediaInfo->AudioTrackTotNum - 1;
			len = strlen(name);
			memcpy(ty_handle->TymediaInfo->AudioInfo[tot_num].AudioPadName, name, len);
			ty_handle->TymediaInfo->AudioInfo[tot_num].AudioPadName[len] = 0;
			audio_on = ON;
			ty_handle->TymediaInfo->AudioOn = ON;

			type = gst_caps_to_string (caps);
			//DbgMsg("Media type %s found\n", type);
//			g_free (type);


			ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = -1;

			for(i = 0; i <AUDIO_TYPE_NUM ; i++)
			{
				len = strlen(AudioTypeString[i]);
				if( 0 == strncmp(gst_structure_get_name(str), AudioTypeString[i], len) ){
					ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = i;
					break;
				}			
			}

			ty_handle->TymediaInfo->AudioInfo[tot_num].bitrate = -1;
			if (!gst_structure_has_field (str, "bitrate")) {
  					//DbgMsg ("------No bitrate\n");	
  				} else {
    				value = gst_structure_get_value (str, "bitrate");
					ty_handle->TymediaInfo->AudioInfo[tot_num].bitrate = g_value_get_int (value);
			}
			
			if(ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType == AUDIO_TYPE_AC3_PRI) 
			{
				
				ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = AUDIO_TYPE_AC3;
			}
			else if(ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType == AUDIO_TYPE_WMA)
			{
				ty_handle->TymediaInfo->AudioInfo[tot_num].wmaversion = -1;
				if (!gst_structure_has_field (str, "wmaversion")) {
  					//DbgMsg ("------No wmaversion\n");	
  				} else {
    				value = gst_structure_get_value (str, "wmaversion");
					ty_handle->TymediaInfo->AudioInfo[tot_num].wmaversion = g_value_get_int (value);
				}
				
			}

			g_free (type);
			
			if(ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType == -1)
			{
				g_print("Not Support Audio Coden\n");
			}


			//DbgMsg("=audio_type = %d,  %s\n",ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType, AudioTypeString[ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType] );
			//DbgMsg("=AudioTrackTotNum = %d\n",ty_handle->TymediaInfo->AudioTrackTotNum);
		}
	}
	
	//DbgMsg("audio_on = %d, video_on = %d\n",audio_on, video_on);
	if(ON == video_on)
	{ 
		if(ty_handle->TymediaInfo->DemuxType == DEMUX_TYPE_MPEGTSDEMUX)	
		{
			if (targetqueue) {
				targetsink = gst_element_get_pad(targetqueue, "sink");
				g_assert(targetsink != NULL);			
				rc = gst_pad_link(pad, targetsink);			
				if (rc) {
					g_critical("demux pad link failed(%s): %d\n", __func__, rc);
				}
				gst_object_unref(targetsink);
				//DbgMsg("demux pad video link succeed\n");
			}
		}
		else //if(0 == strcmp(name,(char *)ty_handle->TymediaInfo->VideoInfo[video_track_num].VideoPadName))	
		{  
			if (targetqueue) {
				targetsink = gst_element_get_pad(targetqueue, "sink");
				g_assert(targetsink != NULL);			
				rc = gst_pad_link(pad, targetsink);			
				if (rc) {
					g_critical("demux pad link failed(%s): %d\n", __func__, rc);
				}
				//DbgMsg("demux pad video link succeed\n");
				gst_object_unref(targetsink);
			}
		}
	}
	else if(ON == audio_on)
	{ 		
		if(ty_handle->TymediaInfo->DemuxType == DEMUX_TYPE_MPEGTSDEMUX)	
		{  
			if (targetqueue) {
				targetsink = gst_element_get_pad(targetqueue, "sink");
				g_assert(targetsink != NULL);			
				rc = gst_pad_link(pad, targetsink);			
				if (rc) {
					g_critical("demux pad link failed(%s): %d\n", __func__, rc);
				}
				gst_object_unref(targetsink);
				//DbgMsg("demux pad audio link succeed\n");
			}
		}
		else //if(0 == strcmp(name,(char *)ty_handle->TymediaInfo->AudioInfo[audio_track_num].AudioPadName))	
		{  
			
			if (targetqueue) {
				targetsink = gst_element_get_pad(targetqueue, "sink");
				g_assert(targetsink != NULL);			
				rc = gst_pad_link(pad, targetsink);			
				if (rc) {
					g_critical("demux pad link failed(%s): %d\n", __func__, rc);
				}
				gst_object_unref(targetsink);
				//DbgMsg("demux pad audio link succeed\n");
			}
		}
	}

EXIT:

	g_free(name);	
	gst_caps_unref(caps);

	if( (ty_handle->TymediaInfo->DemuxType == DEMUX_TYPE_MPEGTSDEMUX)
		&& (CODEC_TYPE_AUDIO == codec_type)
		&& (audio_on == OFF) 
		) {
			g_idle_add (idle_exit_loop, loop);	
	}
	//DbgMsg("===on_demux_pad_added_typefind --\n");

	FUNC_OUT();

}

static void cb_typefind_demux (GstElement *typefind,
	      					guint       probability,
	      					GstCaps    *caps,
	      					gpointer    data)
{
	GMainLoop *loop = NULL;
	TypeFindSt *ty_handle = data;
	gchar *type = NULL;
	gint i = 0;
	gint len = 0;

	FUNC_IN();

	//DbgMsg("===cb_typefind_demux ++\n");

	loop = ty_handle->loop;

	type = gst_caps_to_string (caps);
	//g_print("Media type %s found, probability %d%%\n", type, probability);

	ty_handle->TymediaInfo->DemuxType = -1;

	for(i = 0; i <DEMUX_TYPE_NUM ; i++)
	{
		len = strlen(DemuxTypeString[i]);
		if( 0 == strncmp(type, DemuxTypeString[i], len) ){

			ty_handle->TymediaInfo->DemuxType = i;
			break; 
		}
	}

	//DbgMsg("=== ty_handle->TymediaInfo->DemuxType %d\n",ty_handle->TymediaInfo->DemuxType);

	if(	ty_handle->TymediaInfo->DemuxType == TYPE_MP3
		|| ty_handle->TymediaInfo->DemuxType == TYPE_FLAC
		|| ty_handle->TymediaInfo->DemuxType == TYPE_M4A
		|| ty_handle->TymediaInfo->DemuxType == TYPE_WAV
		|| ty_handle->TymediaInfo->DemuxType == TYPE_MPEG
		|| ty_handle->TymediaInfo->DemuxType == TYPE_AC3
		|| ty_handle->TymediaInfo->DemuxType == TYPE_DTS
		)
	{
		ty_handle->TymediaInfo->AudioOnly = ON;
		if(	ty_handle->TymediaInfo->DemuxType == TYPE_MP3)
			ty_handle->TymediaInfo->AudioInfo[0].ACodecType = AUDIO_TYPE_MP3;
		else if(	ty_handle->TymediaInfo->DemuxType == TYPE_FLAC)
			ty_handle->TymediaInfo->AudioInfo[0].ACodecType = AUDIO_TYPE_FLAC;
		else if(	ty_handle->TymediaInfo->DemuxType == TYPE_M4A)
			ty_handle->TymediaInfo->AudioInfo[0].ACodecType = AUDIO_TYPE_AAC;
		else if(	ty_handle->TymediaInfo->DemuxType == TYPE_WAV)
			ty_handle->TymediaInfo->AudioInfo[0].ACodecType = AUDIO_TYPE_WAV;
		else if(	ty_handle->TymediaInfo->DemuxType == TYPE_MPEG)
			ty_handle->TymediaInfo->AudioInfo[0].ACodecType = AUDIO_TYPE_MPEG;
		else if(	ty_handle->TymediaInfo->DemuxType == TYPE_AC3)
			ty_handle->TymediaInfo->AudioInfo[0].ACodecType = AUDIO_TYPE_AC3;
		else if(	ty_handle->TymediaInfo->DemuxType == TYPE_DTS)
			ty_handle->TymediaInfo->AudioInfo[0].ACodecType = AUDIO_TYPE_DTS;
	}

	//DbgMsg("=== ty_handle->TymediaInfo->AudioOnly %d, audio_type=%d\n",ty_handle->TymediaInfo->AudioOnly, ty_handle->TymediaInfo->AudioInfo[0].ACodecType);

	if(ty_handle->TymediaInfo->DemuxType == -1)
	{
		g_print("Not Support Demux\n");
	}

	g_free (type);

  /* since we connect to a signal in the pipeline thread context, we need
   * to set an idle handler to exit the main loop in the mainloop context.
   * Normally, your app should not need to worry about such things. */
	g_idle_add (idle_exit_loop, loop);

	//DbgMsg("===cb_typefind_demux --\n");

	FUNC_OUT();
}

static void cb_typefind_audio (GstElement *typefind,
	      					guint       probability,
	      					GstCaps    *caps,
	      					gpointer    data)
{
	GMainLoop *loop = NULL;
	TypeFindSt *ty_handle = data;
	const gchar *type = NULL;

	GstStructure *structure;
	gint tmp = 0;
	const GValue *value;
	const gchar *profile_type = NULL;
	gchar *media_type = NULL;
	gint track_num = 0;

	FUNC_IN();

	//DbgMsg("===cb_typefind_audio ++\n");

	track_num = ty_handle->audio_track_num;

	loop = ty_handle->loop;

	structure = gst_caps_get_structure (caps, 0);

	type = gst_structure_get_name (structure);

	if( 0 == strcmp(type, "audio/mpeg") )
  	{
		if (!gst_structure_has_field (structure, "mpegversion")) {
  			//DbgMsg ("------No mpegversion\n");	
  		} else {
    		value = gst_structure_get_value (structure, "mpegversion");
      		tmp = g_value_get_int (value);
	  		//DbgMsg ("--------mpegversion = %d\n", tmp);

	  		if(4 == tmp)	//mpegversion
	  		{
	  			if (!gst_structure_has_field (structure, "profile")) {
  					//DbgMsg ("------No profile\n");	
  				} else {
    				profile_type = gst_structure_get_string (structure, "profile");
	  				//DbgMsg ("--------profile = %s\n", profile_type);
					if( 0 == strcmp(profile_type, "lc") )
					{
						//aal lc
						ty_handle->TymediaInfo->AudioInfo[track_num].ACodecType = AUDIO_TYPE_AAC;
					}
  				}
	  		}
			else if(1 == tmp)	//mpegversion
	  		{
	  			if (!gst_structure_has_field (structure, "layer")) {
  					//DbgMsg ("------No layer\n");	
  				} else {
    				value = gst_structure_get_value (structure, "layer");
      				tmp = g_value_get_int (value);
	  				//DbgMsg ("--------layer = %d\n", tmp);
					if(tmp == 3)
					{
						//mp3
						ty_handle->TymediaInfo->AudioInfo[track_num].ACodecType = AUDIO_TYPE_MP3;
					}
  				}
	  		}
  		}  	
  	}


  	if (!gst_structure_has_field (structure, "rate")) {
  		//DbgMsg ("------No rate\n");	
  	} else {
    	value = gst_structure_get_value (structure, "rate");
      	tmp = g_value_get_int (value);
		ty_handle->TymediaInfo->AudioInfo[track_num].samplerate = tmp;
	  	//DbgMsg ("--------rate = %d\n", tmp);
  	}

  	if (!gst_structure_has_field (structure, "channels")) {
  		//DbgMsg ("------No channels\n");	
  	} else {
    	value = gst_structure_get_value (structure, "channels");
      	tmp = g_value_get_int (value);
		ty_handle->TymediaInfo->AudioInfo[track_num].channels = tmp;
	  	//DbgMsg ("--------channels = %d\n", tmp);
  	}


	media_type = gst_caps_to_string (caps);
	//DbgMsg ("Media type %s found, probability %d%%\n", media_type, probability);
	g_free(media_type);

  /* since we connect to a signal in the pipeline thread context, we need
   * to set an idle handler to exit the main loop in the mainloop context.
   * Normally, your app should not need to worry about such things. */
	g_idle_add (idle_exit_loop, loop);

	//DbgMsg("===cb_typefind_audio --\n");

	FUNC_OUT();
}


static void cb_typefind_video (
								GstElement *typefind,
	      						guint       probability,
	      						GstCaps    *caps,
	      						gpointer    data
								)
{
	GMainLoop *loop = NULL;
	TypeFindSt *ty_handle = data;
	const gchar *type = NULL;

	GstStructure *structure;
	gint tmp = 0;
	const GValue *value;
	gchar *media_type = NULL;
	gint value_numerator = -1;
    gint value_denominator = -1;
	gint track_num = 0;

	FUNC_IN();

	//DbgMsg("===cb_typefind_video ++\n");

	track_num = ty_handle->video_track_num;

	loop = ty_handle->loop;

	structure = gst_caps_get_structure (caps, 0);

	type = gst_structure_get_name (structure);
	//DbgMsg("=video_type = %s\n", type);
	
	if( 0 == strcmp(type, "video/mpeg") )
  	{
		if (!gst_structure_has_field (structure, "mpegversion")) {
  			//DbgMsg ("------No mpegversion\n");	
  		} else {
    		value = gst_structure_get_value (structure, "mpegversion");
      		tmp = g_value_get_int (value);
	  		//DbgMsg ("--------mpegversion = %d\n", tmp);

	  		if(2 == tmp)	//mpegversion
	  		{
				//mpeg2 video
				ty_handle->TymediaInfo->VideoInfo[track_num].VCodecType = VIDEO_TYPE_MP2V;
				//DbgMsg("=============video_type = MP2V\n");
				
	  		}
			else if(4 == tmp)	//mpegversion
	  		{
				//mpeg4 video
				ty_handle->TymediaInfo->VideoInfo[track_num].VCodecType = VIDEO_TYPE_MP4V;
				//DbgMsg("=============video_type = MP4V\n");

	  		}
  		}  	
  	}

	if (!gst_structure_has_field (structure, "width")) {
		//DbgMsg ("------No width\n");	
	} else {
		value = gst_structure_get_value (structure, "width");
		tmp = g_value_get_int (value);
		ty_handle->TymediaInfo->VideoInfo[track_num].Width = tmp;
		//DbgMsg ("--------width = %d\n", tmp);
	}

	if (!gst_structure_has_field (structure, "height")) {
		//DbgMsg ("------No height\n");	
	} else {
		value = gst_structure_get_value (structure, "height");
		tmp = g_value_get_int (value);
		ty_handle->TymediaInfo->VideoInfo[track_num].Height = tmp;
		//DbgMsg ("--------height = %d\n", tmp);
	}

	if (!gst_structure_has_field (structure, "framerate")) {
		//DbgMsg ("------No framerate\n");	
	} else {
		gst_structure_get_fraction (structure, "framerate",&value_numerator,&value_denominator);
		ty_handle->TymediaInfo->VideoInfo[track_num].Framerate.value_numerator = value_numerator;
		ty_handle->TymediaInfo->VideoInfo[track_num].Framerate.value_denominator = value_denominator;
		//DbgMsg ("--------value_numerator = %d, value_denominator = %d\n", value_numerator, value_denominator);
	}
             
	
	media_type = gst_caps_to_string (caps);
	//DbgMsg ("Media type %s found, probability %d%%\n", media_type, probability);
	g_free(media_type);

  /* since we connect to a signal in the pipeline thread context, we need
   * to set an idle handler to exit the main loop in the mainloop context.
   * Normally, your app should not need to worry about such things. */
	g_idle_add (idle_exit_loop, loop);

	//DbgMsg("===cb_typefind_video --\n");

	FUNC_OUT();
}


static int typefind_demux( TYMEDIA_INFO *ty_media_handle, const char *uri )
{

	TypeFindSt ty_handle ;

	FUNC_IN();

  /* init GStreamer */  
	memset(&ty_handle, 0, sizeof(TypeFindSt));	
	ty_handle.loop = g_main_loop_new (NULL, FALSE);
	ty_handle.TymediaInfo = ty_media_handle;

	/* create a new pipeline to hold the elements */
	ty_handle.pipeline = gst_pipeline_new ("pipe");

	ty_handle.bus = gst_pipeline_get_bus ( (ty_handle.pipeline));
	gst_bus_add_watch (ty_handle.bus, bus_callback, NULL);
	gst_object_unref (ty_handle.bus);
	

	/* create file source and typefind element */
	ty_handle.filesrc = gst_element_factory_make ("filesrc", "source");
	g_object_set (G_OBJECT (ty_handle.filesrc), "location", uri, NULL);
	ty_handle.filesrc_typefind = gst_element_factory_make ("typefind", "typefinder_filesrc");

	ty_handle.video_fakesink = gst_element_factory_make ("fakesink", "sink");
      
	g_signal_connect (ty_handle.filesrc_typefind, "have-type", G_CALLBACK (cb_typefind_demux), &ty_handle);

	/* setup */
	gst_bin_add_many(	(ty_handle.pipeline),
						ty_handle.filesrc,																
						ty_handle.filesrc_typefind,																	
						ty_handle.video_fakesink,					
						NULL	);

    gst_element_link_many (ty_handle.filesrc, ty_handle.filesrc_typefind, ty_handle.video_fakesink, NULL);
		
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_PLAYING);
	
	g_main_loop_run (ty_handle.loop);

	/* unset */
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (ty_handle.pipeline));

	FUNC_OUT();

	return 0;
}


static int typefind_codec_info( TYMEDIA_INFO *ty_media_handle, const char *uri, gint codec_type, gint track_num )
{
	TypeFindSt ty_handle ;
	
	FUNC_IN();

	gint ret = 0;
	gint demux_type = 0;

	ty_handle.loop = g_main_loop_new (NULL, FALSE);
	ty_handle.TymediaInfo = ty_media_handle;
	demux_type = ty_handle.TymediaInfo->DemuxType;

	if(codec_type == CODEC_TYPE_VIDEO)
		ty_handle.video_track_num = track_num;
	else
		ty_handle.audio_track_num = track_num;

	/* create a new pipeline to hold the elements */
	ty_handle.pipeline = gst_pipeline_new ("pipe");

	ty_handle.bus = gst_pipeline_get_bus ( (ty_handle.pipeline));
	gst_bus_add_watch (ty_handle.bus, bus_callback, NULL);
	gst_object_unref (ty_handle.bus);

	/* create file source and typefind element */
	ty_handle.filesrc = gst_element_factory_make ("filesrc", "source");
	g_object_set (G_OBJECT (ty_handle.filesrc), "location", uri, NULL);

	if(demux_type == DEMUX_TYPE_MPEGPSDEMUX) {
		if( CODEC_TYPE_AUDIO == codec_type){
			if(ty_handle.TymediaInfo->AudioInfo[ty_handle.audio_track_num].ACodecType == AUDIO_TYPE_AC3){
				ty_handle.demux = gst_element_factory_make ("dvddemux", "demux");
				//g_printf("=======dvd demux====\n");
			}
			else{
				ty_handle.demux = gst_element_factory_make ("mpegpsdemux", "demux");
			}
		}
		else{
				ty_handle.demux = gst_element_factory_make ("mpegpsdemux", "demux");
		}
		
		
		ty_handle.video_parse = gst_element_factory_make ("mpegvideoparse", "parse_video");
		if( CODEC_TYPE_AUDIO == codec_type)
		{ 
			if(ty_handle.TymediaInfo->AudioInfo[ty_handle.audio_track_num].ACodecType == AUDIO_TYPE_MPEG)
				ty_handle.audio_parse = gst_element_factory_make ("mpegaudioparse", "parse_audio");
			else if(ty_handle.TymediaInfo->AudioInfo[ty_handle.audio_track_num].ACodecType == AUDIO_TYPE_AC3)
				ty_handle.audio_parse = gst_element_factory_make ("ac3parse", "parse_audio");
		}
	}else if( (demux_type == DEMUX_TYPE_QTDEMUX) || (demux_type == TYPE_3GP)) {
		ty_handle.demux = gst_element_factory_make ("qtdemux", "demux");
	}else if( (demux_type == DEMUX_TYPE_OGGDEMUX) || (demux_type == DEMUX_TYPE_ANNODEX) ) {
		ty_handle.demux = gst_element_factory_make ("oggdemux", "demux");
	}else if(demux_type == DEMUX_TYPE_RMDEMUX) {
		ty_handle.demux = gst_element_factory_make ("rmdemux", "demux");
	}else if(demux_type == DEMUX_TYPE_AVIDEMUX) {
		ty_handle.demux = gst_element_factory_make ("avidemux", "demux");
	}else if(demux_type == DEMUX_TYPE_ASFDEMUX) {
		ty_handle.demux = gst_element_factory_make ("asfdemux", "demux");
	}else if(demux_type == DEMUX_TYPE_MATROSKADEMUX) {
		ty_handle.demux = gst_element_factory_make ("matroskademux", "demux");
	}else if(demux_type == DEMUX_TYPE_FLVDEMUX) {
		ty_handle.demux = gst_element_factory_make ("flvdemux", "demux");
	}else if(demux_type == DEMUX_TYPE_MPEGTSDEMUX) {
		ty_handle.demux = gst_element_factory_make ("mpegtsdemux", "demux"); //mpegtsparse 
		g_object_set (G_OBJECT(ty_handle.demux), "program-number", ty_handle.TymediaInfo->program_no[track_num], NULL); 
	}else {

	}	

	ty_handle.codec_type = codec_type;

	if( CODEC_TYPE_VIDEO == codec_type)
	{
		if(demux_type == DEMUX_TYPE_MPEGTSDEMUX)
		{
			ty_handle.decode = gst_element_factory_make ("decodebin", "videodecoder");
		}
		ty_handle.video_queue = gst_element_factory_make ("queue2", "video_queue");
		ty_handle.video_typefind = gst_element_factory_make ("typefind", "typefinder_video");
		ty_handle.video_fakesink = gst_element_factory_make ("fakesink", "sink_video");
		g_signal_connect (ty_handle.video_typefind, "have-type", G_CALLBACK (cb_typefind_video), &ty_handle);
		
	}
	else	//AUDIO
	{
		if(demux_type == DEMUX_TYPE_MPEGTSDEMUX)
		{
			ty_handle.decode = gst_element_factory_make ("decodebin", "audiodecoder");
		}

		ty_handle.audio_queue = gst_element_factory_make ("queue2", "audio_queue");
		ty_handle.audio_typefind = gst_element_factory_make ("typefind", "typefinder_audio");
		ty_handle.audio_fakesink = gst_element_factory_make ("fakesink", "sink_audio");
		g_signal_connect (ty_handle.audio_typefind, "have-type", G_CALLBACK (cb_typefind_audio), &ty_handle);
	}

	g_signal_connect( ty_handle.demux, "pad-added", G_CALLBACK(on_demux_pad_added_typefind), &ty_handle );
	 	
	/* setup */
	//-------------------
	if( demux_type == DEMUX_TYPE_MPEGPSDEMUX )
	{
		if( CODEC_TYPE_VIDEO == codec_type)
		{
			gst_bin_add_many(	(ty_handle.pipeline),
								ty_handle.filesrc,																	
								ty_handle.demux,																	
								ty_handle.video_queue, ty_handle.video_parse, ty_handle.video_typefind, ty_handle.video_fakesink,		
								NULL	);
			//	link
			ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
			if(ret == 0)				
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret  = gst_element_link_many ( ty_handle.video_queue, ty_handle.video_parse, ty_handle.video_typefind, ty_handle.video_fakesink, NULL );	
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		}
		else
		{	//CODEC_TYPE_AUDIO 
			gst_bin_add_many(	(ty_handle.pipeline),
								ty_handle.filesrc,																	
								ty_handle.demux,																	
								ty_handle.audio_queue, ty_handle.audio_parse, ty_handle.audio_typefind, ty_handle.audio_fakesink,		
								NULL	);
			//	link
			ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret  = gst_element_link_many (ty_handle.audio_queue, ty_handle.audio_parse, ty_handle.audio_typefind, ty_handle.audio_fakesink, NULL);
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		}		
	}
	else if( demux_type == DEMUX_TYPE_MPEGTSDEMUX )
	{
		if( CODEC_TYPE_VIDEO == codec_type)
		{
//decodebin
			gst_bin_add_many(	(ty_handle.pipeline),
								ty_handle.filesrc,																	
								ty_handle.demux,																	
								ty_handle.video_queue, ty_handle.decode, ty_handle.video_typefind, ty_handle.video_fakesink,		
								NULL	);

			
//	link
//decodebin
			g_signal_connect( ty_handle.decode, "pad-added", G_CALLBACK(on_video_decodebin_pad_added), &ty_handle );
			ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret  = gst_element_link ( ty_handle.video_queue, ty_handle.decode);	
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret  = gst_element_link_many ( /*ty_handle.audio_queue,*/ ty_handle.video_typefind, ty_handle.video_fakesink, NULL );	
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		}
		else
		{	//CODEC_TYPE_AUDIO			
			gst_bin_add_many(	(ty_handle.pipeline),
								ty_handle.filesrc,																	
								ty_handle.demux,																	
								ty_handle.audio_queue, ty_handle.decode, ty_handle.audio_typefind, ty_handle.audio_fakesink,		
								NULL	);
			//	link
			g_signal_connect( ty_handle.decode, "pad-added", G_CALLBACK(on_audio_decodebin_pad_added), &ty_handle );
			
			ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret  = gst_element_link ( ty_handle.audio_queue, ty_handle.decode);	
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret  = gst_element_link_many (  ty_handle.audio_typefind, ty_handle.audio_fakesink, NULL );	
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		}		
	}
	else
	{
		if( CODEC_TYPE_VIDEO == codec_type)
		{
			gst_bin_add_many(	(ty_handle.pipeline),
								ty_handle.filesrc,																	
								ty_handle.demux,																	
								ty_handle.video_queue, ty_handle.video_typefind, ty_handle.video_fakesink,		
								NULL	);
			//	link
			ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret  = gst_element_link_many ( ty_handle.video_queue,ty_handle.video_typefind, ty_handle.video_fakesink, NULL );	
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		}
		else	
		{	//CODEC_TYPE_AUDIO			
						gst_bin_add_many(	(ty_handle.pipeline),
								ty_handle.filesrc,																	
								ty_handle.demux,																	
								ty_handle.audio_queue, ty_handle.audio_typefind, ty_handle.audio_fakesink,		
								NULL	);
			//	link
			ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret  = gst_element_link_many (ty_handle.audio_queue, ty_handle.audio_typefind, ty_handle.audio_fakesink, NULL);
			if(ret == 0)
				g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		}
	}

	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_PLAYING);

	g_main_loop_run (ty_handle.loop);

	/* unset */
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (ty_handle.pipeline));

	FUNC_OUT();

	return ret;
}



//--------------------

static void on_demux_pad_added_num(GstElement *element, GstPad *pad, TypeFindSt *ty_handle)
{
	GMainLoop *loop = NULL;
	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPadLinkReturn rc;
	GstElement *targetqueue;
	GstPad *targetsink;
	gint	tot_num = 0;
	gint	len = 0, i = 0;

	FUNC_IN();
	
	//DbgMsg("===on_demux_pad_added_num ++\n");

	loop = ty_handle->loop;

	caps = gst_pad_get_caps(pad);
	g_assert(caps != NULL);
	name = gst_pad_get_name(pad);
	//DbgMsg("new demux pad add %s\n", name);

	str = gst_caps_get_structure(caps, 0);
	g_assert(str != NULL);

	//DbgMsg("compare string %s\n", gst_structure_get_name(str));

	targetqueue = NULL;

	// TODO: is this the right way to match video/audio pads
	if (g_strrstr(gst_structure_get_name(str), "video")) {
		//DbgMsg("Linking %s to %s\n", name, gst_structure_get_name(str) );
		targetqueue = ty_handle->video_queue;
		tot_num = ty_handle->TymediaInfo->VideoTrackTotNum;
		ty_handle->TymediaInfo->VideoTrackTotNum++;
		ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType = -1;
		
		for(i = 0; i <VIDEO_TYPE_NUM ; i++)
		{
			len = strlen(VideoTypeString[i]);

			if( 0 == strncmp(gst_structure_get_name(str), VideoTypeString[i], len) ){
				ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType = i;
				break;
			}			
		}
		
	}

	if (g_strrstr(gst_structure_get_name(str), "audio")) {
		//DbgMsg("Linking %s to %s\n", name, gst_structure_get_name(str) );
		targetqueue = ty_handle->audio_queue;
		tot_num = ty_handle->TymediaInfo->AudioTrackTotNum;
		ty_handle->TymediaInfo->AudioTrackTotNum++;
		ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = -1;
		
		for(i = 0; i <AUDIO_TYPE_NUM ; i++)
		{
			len = strlen(AudioTypeString[i]);
			if( 0 == strncmp(gst_structure_get_name(str), AudioTypeString[i], len) ){
				ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = i;
				break;
			}			
		}
		if(ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType == AUDIO_TYPE_AC3_PRI) 				
			ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = AUDIO_TYPE_AC3;
	}

	if (targetqueue) {
		targetsink = gst_element_get_pad(targetqueue, "sink");
		g_assert(targetsink != NULL);			
		rc = gst_pad_link(pad, targetsink);			
		if (rc) {
			g_critical("demux pad link failed(%s): %d\n", __func__, rc);
		}
		gst_object_unref(targetsink);
	}



	g_free(name);	

	gst_caps_unref(caps);

	g_idle_add (idle_exit_loop, loop);

	//DbgMsg("===on_demux_pad_added_num --\n");

	FUNC_OUT();
}
static int find_avcodec_num( TYMEDIA_INFO *ty_media_handle, const char *uri )
{
	TypeFindSt ty_handle ;

	FUNC_IN();

	gint ret = 0;
	gint demux_type = 0;

	ty_handle.loop = g_main_loop_new (NULL, FALSE);
	ty_handle.TymediaInfo = ty_media_handle;
	demux_type = ty_handle.TymediaInfo->DemuxType;

	/* create a new pipeline to hold the elements */
	ty_handle.pipeline = gst_pipeline_new ("pipe");

	ty_handle.bus = gst_pipeline_get_bus (GST_PIPELINE (ty_handle.pipeline));
	gst_bus_add_watch (ty_handle.bus, bus_callback, NULL);
	gst_object_unref (ty_handle.bus);

	/* create file source and typefind element */
	ty_handle.filesrc = gst_element_factory_make ("filesrc", "source");
	g_object_set (G_OBJECT (ty_handle.filesrc), "location", uri, NULL);

	if(demux_type == DEMUX_TYPE_MPEGPSDEMUX) {
		ty_handle.demux = gst_element_factory_make ("mpegpsdemux", "demux");
		ty_handle.video_parse = gst_element_factory_make ("mpegvideoparse", "parse_video");
		ty_handle.audio_parse = gst_element_factory_make ("mpegaudioparse", "parse_audio");
	}else if( (demux_type == DEMUX_TYPE_QTDEMUX) || (demux_type == TYPE_3GP)) {
		ty_handle.demux = gst_element_factory_make ("qtdemux", "demux");
	}else if( (demux_type == DEMUX_TYPE_OGGDEMUX) || (demux_type == DEMUX_TYPE_ANNODEX)) {
		ty_handle.demux = gst_element_factory_make ("oggdemux", "demux");
	}else if(demux_type == DEMUX_TYPE_RMDEMUX) {
		ty_handle.demux = gst_element_factory_make ("rmdemux", "demux");
	}else if(demux_type == DEMUX_TYPE_AVIDEMUX) {
		ty_handle.demux = gst_element_factory_make ("avidemux", "demux");
	}else if(demux_type == DEMUX_TYPE_ASFDEMUX) {
		ty_handle.demux = gst_element_factory_make ("asfdemux", "demux");
	}else if(demux_type == DEMUX_TYPE_MATROSKADEMUX) {
		ty_handle.demux = gst_element_factory_make ("matroskademux", "demux");
	}else if(demux_type == DEMUX_TYPE_FLVDEMUX) {
		ty_handle.demux = gst_element_factory_make ("flvdemux", "demux");
	}else if(demux_type == DEMUX_TYPE_MPEGTSDEMUX) {
		ty_handle.demux = gst_element_factory_make ("mpegtsdemux", "demux"); 
	}else {

	}	

	ty_handle.video_queue = gst_element_factory_make ("queue2", "video_queue");
	ty_handle.video_fakesink = gst_element_factory_make ("fakesink", "sink_video");

	ty_handle.audio_queue = gst_element_factory_make ("queue2", "audio_queue");
	ty_handle.audio_fakesink = gst_element_factory_make ("fakesink", "sink_audio");

	if(demux_type == DEMUX_TYPE_MPEGTSDEMUX)
	{
		gst_bin_add_many(	(ty_handle.pipeline),
			ty_handle.filesrc,																	
			ty_handle.demux,																	
			ty_handle.video_queue, ty_handle.video_fakesink,		
			NULL	);

		//	link
		ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
		if(ret == 0)				
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		ret  = gst_element_link_many ( ty_handle.video_queue, ty_handle.video_fakesink, NULL );	
		if(ret == 0)
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);

		g_signal_connect (ty_handle.demux, "pad-added", G_CALLBACK (on_pad_added), ty_handle.video_fakesink);
		g_signal_connect (G_OBJECT(ty_handle.demux), "notify::pat-info", (GCallback)demuxer_notify_pat_info, &ty_handle);
		g_signal_connect (G_OBJECT(ty_handle.demux), "notify::pmt-info", (GCallback)demuxer_notify_pmt_info, &ty_handle);

	}
	else
	{
		g_signal_connect( ty_handle.demux, "pad-added", G_CALLBACK(on_demux_pad_added_num), &ty_handle );

		gst_bin_add_many(	(ty_handle.pipeline),
			ty_handle.filesrc,																	
			ty_handle.demux,																	
			ty_handle.video_queue, ty_handle.video_fakesink,		
			ty_handle.audio_queue, ty_handle.audio_fakesink,		
			NULL	);

		//	link
		ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
		if(ret == 0)				
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		ret  = gst_element_link_many ( ty_handle.video_queue, ty_handle.video_fakesink, NULL );	
		if(ret == 0)
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		ret  = gst_element_link_many (ty_handle.audio_queue, ty_handle.audio_fakesink, NULL);
		if(ret == 0)
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
	}


	/* setup */
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_PLAYING);

	g_main_loop_run (ty_handle.loop);


	/* unset */
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (ty_handle.pipeline));

	FUNC_OUT();

	return ret;
}


static void cb_typefind_video_ps (
								GstElement *typefind,
	      						guint       probability,
	      						GstCaps    *caps,
	      						gpointer    data
								)
{
	GMainLoop *loop = NULL;
	TypeFindSt *ty_handle = data;

	FUNC_IN();

//	DbgMsg("===cb_typefind_video ++\n");

	loop = ty_handle->loop;


  /* since we connect to a signal in the pipeline thread context, we need
   * to set an idle handler to exit the main loop in the mainloop context.
   * Normally, your app should not need to worry about such things. */
	g_idle_add (idle_exit_loop, loop);

//	DbgMsg("===cb_typefind_video --\n");

	FUNC_OUT();
}

								
static void on_demux_pad_added_num_ps(GstElement *element, GstPad *pad, TypeFindSt *ty_handle)
{
//	GMainLoop *loop = NULL;
	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPadLinkReturn rc;
	GstElement *targetqueue;
	GstPad *targetsink;
	gint	tot_num = 0;
	gint	len = 0, i = 0;

	FUNC_IN();
	
	//DbgMsg("===on_demux_pad_added_num ++\n");

//	loop = ty_handle->loop;

	caps = gst_pad_get_caps(pad);
	g_assert(caps != NULL);
	name = gst_pad_get_name(pad);
	//DbgMsg("new demux pad add %s\n", name);

	str = gst_caps_get_structure(caps, 0);
	g_assert(str != NULL);

	//DbgMsg("compare string %s\n", gst_structure_get_name(str));

	targetqueue = NULL;

	// TODO: is this the right way to match video/audio pads
	if (g_strrstr(gst_structure_get_name(str), "video")) {
		//DbgMsg("Linking %s to %s\n", name, gst_structure_get_name(str) );
		targetqueue = ty_handle->video_queue;
		tot_num = ty_handle->TymediaInfo->VideoTrackTotNum;
		ty_handle->TymediaInfo->VideoTrackTotNum++;
		ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType = -1;
		
		for(i = 0; i <VIDEO_TYPE_NUM ; i++)
		{
			len = strlen(VideoTypeString[i]);

			if( 0 == strncmp(gst_structure_get_name(str), VideoTypeString[i], len) ){
				ty_handle->TymediaInfo->VideoInfo[tot_num].VCodecType = i;
				break;
			}			
		}
		
	}

	if (g_strrstr(gst_structure_get_name(str), "audio")) {
		//DbgMsg("Linking %s to %s\n", name, gst_structure_get_name(str) );
		targetqueue = ty_handle->audio_queue;
		tot_num = ty_handle->TymediaInfo->AudioTrackTotNum;
		ty_handle->TymediaInfo->AudioTrackTotNum++;
		ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = -1;
		
		for(i = 0; i <AUDIO_TYPE_NUM ; i++)
		{
			len = strlen(AudioTypeString[i]);
			if( 0 == strncmp(gst_structure_get_name(str), AudioTypeString[i], len) ){
				ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = i;
				break;
			}			
		}
		if(ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType == AUDIO_TYPE_AC3_PRI) 				
			ty_handle->TymediaInfo->AudioInfo[tot_num].ACodecType = AUDIO_TYPE_AC3;
	}

	if (targetqueue) {
		targetsink = gst_element_get_pad(targetqueue, "sink");
		g_assert(targetsink != NULL);			
		rc = gst_pad_link(pad, targetsink);			
		if (rc) {
			g_critical("demux pad link failed(%s): %d\n", __func__, rc);
		}
		gst_object_unref(targetsink);
	}

	g_free(name);	

	gst_caps_unref(caps);

	//DbgMsg("===on_demux_pad_added_num --\n");

	FUNC_OUT();
}

static int find_avcodec_num_ps( TYMEDIA_INFO *ty_media_handle, const char *uri )
{
	TypeFindSt ty_handle ;

	FUNC_IN();

	gint ret = 0;
//	gint demux_type = 0;

	ty_handle.loop = g_main_loop_new (NULL, FALSE);
	ty_handle.TymediaInfo = ty_media_handle;
//	demux_type = ty_handle.TymediaInfo->DemuxType;

	/* create a new pipeline to hold the elements */
//	g_print("=======find_avcodec_num_ps====\n");
	ty_handle.pipeline = gst_pipeline_new ("pipe");

	ty_handle.bus = gst_pipeline_get_bus (GST_PIPELINE (ty_handle.pipeline));
	gst_bus_add_watch (ty_handle.bus, bus_callback, NULL);
	gst_object_unref (ty_handle.bus);

	/* create file source and typefind element */
	ty_handle.filesrc = gst_element_factory_make ("filesrc", "source");
	g_object_set (G_OBJECT (ty_handle.filesrc), "location", uri, NULL);

	ty_handle.demux = gst_element_factory_make ("mpegpsdemux", "demux");
	ty_handle.video_decode = gst_element_factory_make ("decodebin", "video_decode");
	ty_handle.video_typefind = gst_element_factory_make ("typefind", "typefinder_video");
	ty_handle.audio_decode = gst_element_factory_make ("decodebin", "audio_decode");
		
	ty_handle.video_queue = gst_element_factory_make ("queue2", "video_queue");
	ty_handle.temp_queue = gst_element_factory_make ("queue2", "temp_queue");
	ty_handle.video_fakesink = gst_element_factory_make ("fakesink", "sink_video");

	ty_handle.audio_queue = gst_element_factory_make ("queue2", "audio_queue");
	ty_handle.audio_fakesink = gst_element_factory_make ("fakesink", "sink_audio");

	g_signal_connect( ty_handle.demux, "pad-added", G_CALLBACK(on_demux_pad_added_num_ps), &ty_handle );
	g_signal_connect (ty_handle.video_typefind, "have-type", G_CALLBACK (cb_typefind_video_ps), &ty_handle);

	gst_bin_add_many(	(ty_handle.pipeline),
						ty_handle.filesrc,																	
						ty_handle.demux,																	
						ty_handle.video_queue, /*ty_handle.video_parse,*/ ty_handle.video_decode,  ty_handle.video_typefind, ty_handle.video_fakesink,		
						ty_handle.audio_queue, /*ty_handle.audio_decode,*/  ty_handle.audio_fakesink,		
						NULL	
					);

	//	link
	g_signal_connect( ty_handle.video_decode, "pad-added", G_CALLBACK(on_video_decodebin_pad_added), &ty_handle );
	ret  = gst_element_link( ty_handle.filesrc, ty_handle.demux );
	if(ret == 0)				
		g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);

	ret  = gst_element_link ( ty_handle.video_queue, ty_handle.video_decode);
	if(ret == 0)
		g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
	ret  = gst_element_link_many ( ty_handle.video_typefind, ty_handle.video_fakesink, NULL );	
	if(ret == 0)
		g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
	ret  = gst_element_link_many ( ty_handle.audio_queue, /*ty_handle.audio_decode,*/ ty_handle.audio_fakesink, NULL );	
	if(ret == 0)
		g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);


	/* setup */
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_PLAYING);

	g_main_loop_run (ty_handle.loop);


	/* unset */
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (ty_handle.pipeline));

	FUNC_OUT();

	return ret;
}


//video only
static void on_video_decodebin_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPadLinkReturn rc;
	GstElement *targetqueue;
	GstPad *targetsink;
	TypeFindSt *ty_handle = data; 

	//DbgMsg("===on_video_decodebin_pad_added ++\n");

	caps = gst_pad_get_caps(pad);
	g_assert(caps != NULL);
	name = gst_pad_get_name(pad);
	//DbgMsg("new video decodebin pad %s\n", name);

	str = gst_caps_get_structure(caps, 0);
	g_assert(str != NULL);

//	DbgMsg("Media type %s found\n", str);
	//DbgMsg("compare string %s\n", gst_structure_get_name(str));

	targetqueue = NULL;
	// TODO: is this the right way to match /audio pads

	if (g_strrstr(gst_structure_get_name(str), "video")) {
		//DbgMsg("Linking %s to %s\n", name, gst_structure_get_name(str) );
		//targetqueue = ty_handle->audio_queue;
		targetqueue = ty_handle->video_typefind;
		
	}

	g_free(name);

	if (targetqueue) {
		targetsink = gst_element_get_pad(targetqueue, "sink");
		g_assert(targetsink != NULL);
		rc = gst_pad_link(pad, targetsink);
		if (rc) {
			g_critical("video decodebin pad link failed(%s): %d\n", __func__, rc);
		}
		gst_object_unref(targetsink);
	}
	
	gst_caps_unref(caps);

	//DbgMsg("===on_video_decodebin_pad_added --\n");

}


//audio only
static void on_audio_decodebin_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPadLinkReturn rc;
	GstElement *targetqueue;
	GstPad *targetsink;
	TypeFindSt *ty_handle = data; 

	//DbgMsg("===on_audio_decodebin_pad_added ++\n");

	caps = gst_pad_get_caps(pad);
	g_assert(caps != NULL);
	name = gst_pad_get_name(pad);
	//DbgMsg("new decodebin pad %s\n", name);

	str = gst_caps_get_structure(caps, 0);
	g_assert(str != NULL);

	//DbgMsg("compare string %s\n", gst_structure_get_name(str));

	targetqueue = NULL;
	// TODO: is this the right way to match /audio pads

	if (g_strrstr(gst_structure_get_name(str), "audio")) {
		//DbgMsg("Linking %s to %s\n", name, gst_structure_get_name(str) );		
		if(ty_handle->TymediaInfo->DemuxType == DEMUX_TYPE_MPEGTSDEMUX)
			targetqueue = ty_handle->audio_typefind;
		else
			targetqueue = ty_handle->audio_queue;
	}

	g_free(name);

	if (targetqueue) {
		targetsink = gst_element_get_pad(targetqueue, "sink");
		g_assert(targetsink != NULL);
		rc = gst_pad_link(pad, targetsink);
		if (rc) {
			g_critical("decodebin pad link failed(%s): %d\n", __func__, rc);
		}
		gst_object_unref(targetsink);
	}

	gst_caps_unref(caps);

	//DbgMsg("===on_audio_decodebin_pad_added --\n");
}



static void cb_typefind_audio_only (GstElement *typefind,
	      					guint       probability,
	      					GstCaps    *caps,
	      					gpointer    data)
{
	GMainLoop *loop = NULL;
	TypeFindSt *ty_handle = data;
	const gchar *type = NULL;

	GstStructure *structure;
	gint tmp = 0;
	const GValue *value;
	const gchar *profile_type = NULL;
	gchar *media_type = NULL;
	gint track_num = 0;

	//DbgMsg("===cb_typefind_audio_only ++\n");
	
	track_num = ty_handle->audio_track_num;
	ty_handle->TymediaInfo->AudioTrackTotNum = 1;

	loop = ty_handle->loop;

	structure = gst_caps_get_structure (caps, 0);

	type = gst_structure_get_name (structure);

	//DbgMsg("==type = %s\n",type);

	if( 0 == strcmp(type, "audio/mpeg") )
  	{
		if (!gst_structure_has_field (structure, "mpegversion")) {
  			//DbgMsg ("--No mpegversion\n");	
  		} else {
    		value = gst_structure_get_value (structure, "mpegversion");
      		tmp = g_value_get_int (value);
	  		//DbgMsg ("--------mpegversion = %d\n", tmp);

	  		if(4 == tmp)	//mpegversion
	  		{
	  			if (!gst_structure_has_field (structure, "profile")) {
  					//g_print ("------No profile\n");	
  				} else {
    				profile_type = gst_structure_get_string (structure, "profile");
	  				//DbgMsg ("--------profile = %s\n", profile_type);
					if( 0 == strcmp(profile_type, "lc") )
					{
						//aal lc
						ty_handle->TymediaInfo->AudioInfo[track_num].ACodecType = AUDIO_TYPE_AAC;
						//DbgMsg("============%d\n",ty_handle->TymediaInfo->AudioInfo[track_num].ACodecType);
						//DbgMsg("=============audio_type = AAC\n");
					}
  				}
	  		}
			else if(1 == tmp)	//mpegversion
	  		{
	  			if (!gst_structure_has_field (structure, "layer")) {
  					DbgMsg ("--No layer\n");	
  				} else {
    				value = gst_structure_get_value (structure, "layer");
      				tmp = g_value_get_int (value);
	  				//DbgMsg ("--------layer = %d\n", tmp);
					if(tmp == 3)
					{
						//mp3
						ty_handle->TymediaInfo->AudioInfo[track_num].ACodecType = AUDIO_TYPE_MP3;
						//DbgMsg("=============audio_type = MP3\n");
					}
  				}
	  		}
  		}  	
  	}

  	if (!gst_structure_has_field (structure, "rate")) {
  		//DbgMsg ("------No rate\n");	
  	} else {
    	value = gst_structure_get_value (structure, "rate");
      	tmp = g_value_get_int (value);
		ty_handle->TymediaInfo->AudioInfo[track_num].samplerate = tmp;
	  	//DbgMsg ("--------rate = %d\n", tmp);
  	}

  	if (!gst_structure_has_field (structure, "channels")) {
  		//DbgMsg ("------No channels\n");	
  	} else {
    	value = gst_structure_get_value (structure, "channels");
      	tmp = g_value_get_int (value);
		ty_handle->TymediaInfo->AudioInfo[track_num].channels = tmp;
	  	//DbgMsg ("--------channels = %d\n", tmp);
  	}


	media_type = gst_caps_to_string (caps);
	//DbgMsg ("Media type %s found, probability %d%%\n", media_type, probability);
	g_free(media_type);

  /* since we connect to a signal in the pipeline thread context, we need
   * to set an idle handler to exit the main loop in the mainloop context.
   * Normally, your app should not need to worry about such things. */
	g_idle_add (idle_exit_loop, loop);

	//DbgMsg("===cb_typefind_audio_only --\n");
		
}

static int typefind_audio_codec_info( TYMEDIA_INFO *ty_media_handle, const char *uri, gint track_num )
{
	TypeFindSt ty_handle ;
	
	gint ret = 0;

	ty_handle.loop = g_main_loop_new (NULL, FALSE);
	ty_handle.TymediaInfo = ty_media_handle;

	ty_handle.audio_track_num = track_num;

	/* create a new pipeline to hold the elements */
	ty_handle.pipeline = gst_pipeline_new ("pipe");

	ty_handle.bus = gst_pipeline_get_bus ( (ty_handle.pipeline));
	gst_bus_add_watch (ty_handle.bus, (GstBusFunc)bus_callback, NULL);
	gst_object_unref (ty_handle.bus);

	/* create file source and typefind element */
	ty_handle.filesrc = gst_element_factory_make ("filesrc", "source");
	g_object_set (G_OBJECT (ty_handle.filesrc), "location", uri, NULL);

	ty_handle.codec_type = CODEC_TYPE_AUDIO;

	ty_handle.audio_queue = gst_element_factory_make ("queue", "audio_queue");

	ty_handle.decode = gst_element_factory_make ("decodebin", "audio_decode");

	if( ty_handle.TymediaInfo->DemuxType == TYPE_AC3)
	{
		ty_handle.audio_parse = gst_element_factory_make ("ac3parse", "audio_parse");
	}
	else if( ty_handle.TymediaInfo->DemuxType == TYPE_DTS)
	{
		ty_handle.audio_parse = gst_element_factory_make ("dcaparse", "audio_parse");
	}
	
	ty_handle.audio_typefind = gst_element_factory_make ("typefind", "typefinder_audio");
	ty_handle.audio_fakesink = gst_element_factory_make ("fakesink", "sink_audio");
	g_signal_connect (ty_handle.audio_typefind, "have-type", G_CALLBACK (cb_typefind_audio_only), &ty_handle);

	if(	ty_handle.filesrc == NULL 
		||ty_handle.audio_queue == NULL 
		||ty_handle.audio_typefind == NULL 
		||ty_handle.audio_fakesink == NULL )
	{
		DbgMsg("===Element NULL!!\n");
	}	
 	
	/* setup */
	//-------------------
	//CODEC_TYPE_AUDIO
	if( ty_handle.TymediaInfo->DemuxType == TYPE_AC3 || ty_handle.TymediaInfo->DemuxType == TYPE_DTS)
	{
		gst_bin_add_many(	(ty_handle.pipeline),
									ty_handle.filesrc,																																										
									ty_handle.audio_parse, 
									ty_handle.audio_queue, 
									ty_handle.audio_typefind, 
									ty_handle.audio_fakesink,		
									NULL	
						);
	}
	else
	{
		gst_bin_add_many(	(ty_handle.pipeline),
									ty_handle.filesrc,																																										
									ty_handle.decode, 
									ty_handle.audio_queue, 
									ty_handle.audio_typefind, 
									ty_handle.audio_fakesink,		
									NULL	
						);
	
	}

	if(    ty_handle.TymediaInfo->DemuxType != TYPE_AC3
		&& ty_handle.TymediaInfo->DemuxType != TYPE_DTS)
	{
		g_signal_connect( ty_handle.decode, "pad-added", G_CALLBACK(on_audio_decodebin_pad_added), &ty_handle );
	}
	
	//	link
	if( ty_handle.TymediaInfo->DemuxType == TYPE_AC3 || ty_handle.TymediaInfo->DemuxType == TYPE_DTS)
	{		
		ret  = gst_element_link_many ( ty_handle.filesrc, ty_handle.audio_parse, ty_handle.audio_queue, ty_handle.audio_typefind, ty_handle.audio_fakesink, NULL);
		if(ret == 0)
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
	}
	else
	{
		ret  = gst_element_link( ty_handle.filesrc, ty_handle.decode );
		ret  = gst_element_link_many ( ty_handle.audio_queue, ty_handle.audio_typefind, ty_handle.audio_fakesink, NULL);
		if(ret == 0)
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
	}
	
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_PLAYING);
	
	g_main_loop_run (ty_handle.loop);
		
	/* unset */
	gst_element_set_state ( (ty_handle.pipeline), GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (ty_handle.pipeline));
		

	return ret;
}

MP_RESULT NX_TypeFind_Open(TYMEDIA_INFO **ty_handle)
{
	FUNC_IN();

	TYMEDIA_INFO *handle = NULL;

	handle = (TYMEDIA_INFO *)malloc( sizeof(TYMEDIA_INFO) );

	if(handle == NULL)
	{
		g_print("handle == NULL !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
		return -1;
	}

	memset(handle, 0, sizeof(TYMEDIA_INFO) );

	if( !gst_is_initialized() ){
		gst_init(NULL, NULL);
	}

	*ty_handle = handle;

	FUNC_OUT();

	return 0;
}

MP_RESULT NX_TypeFind( TYMEDIA_INFO *ty_media_handle,  const char *uri)
{
	FUNC_IN();

	gint ret = 0;
	gint i = 0;
	gint VideoTotNum = 0;
	gint AudioTotNum = 0;

	typefind_demux( ty_media_handle, uri );

	if(ty_media_handle->DemuxType == -1)
	{
		ret = ERROR_NOT_SUPPORT_CONTENTS;
		return ret;
	}

	if( ty_media_handle->AudioOnly == ON)
	{
		ret = typefind_audio_codec_info( ty_media_handle, uri, 0 );
		if(ty_media_handle->AudioInfo[0].ACodecType == -1)
		{
			ret = ERROR_NOT_SUPPORT_CONTENTS;
			DbgMsg("===ERROR_NOT_SUPPORT_CONTENTS\n");
			return ret;
		}
		if(ty_media_handle->AudioTrackTotNum > 0) 
			ty_media_handle->AudioInfo[0].AudioTrackNum = 1; 
	}
	else
	{
		find_avcodec_num( ty_media_handle, uri);
		if(ty_media_handle->DemuxType == DEMUX_TYPE_MPEGPSDEMUX){
			if(ty_media_handle->AudioTrackTotNum <= 0 ){
				ty_media_handle->VideoTrackTotNum = 0;
				find_avcodec_num_ps( ty_media_handle, uri);
			}
		}

		if(ty_media_handle->DemuxType == DEMUX_TYPE_MPEGTSDEMUX){
			VideoTotNum = ty_media_handle->program_tot_no;
			AudioTotNum = ty_media_handle->program_tot_no;
		}
		else{
			VideoTotNum = ty_media_handle->VideoTrackTotNum;
			AudioTotNum = ty_media_handle->AudioTrackTotNum;
		}
		ty_media_handle->VideoTrackTotNum = 0;
		ty_media_handle->AudioTrackTotNum = 0;

		if(ty_media_handle->DemuxType == DEMUX_TYPE_MPEGTSDEMUX){
#if 0
			g_print("\n");
			g_print("============================================\n");
			g_print("=========== MpegTs Program Total Number : %d\n",ty_media_handle->program_tot_no);		
			for(i = 0; i < (gint)ty_media_handle->program_tot_no; i++)
				g_print("=========== program : %d \n", ty_media_handle->program_no[i] );		
			g_print("============================================\n");
			g_print("\n");
#endif
		
		}
		else{
#if 0
			g_print("\n");
			g_print("============================================\n");
			g_print("===========VideoTotNum = %d==================\n",VideoTotNum);		
			for(i = 0; i < VideoTotNum; i++)
				g_print("===========VCodecType : %s \n", VideoTypeString[ty_media_handle->VideoInfo[i].VCodecType] );		
			g_print("===========AudioTotNum = %d==================\n",AudioTotNum);
			for(i = 0; i < AudioTotNum; i++)
				g_print("===========ACodecType : %s \n", AudioTypeString[ty_media_handle->AudioInfo[i].ACodecType] );
			g_print("============================================\n");
			g_print("\n");
#endif
		}

		if(0 < VideoTotNum)
		{
			ty_media_handle->VideoTrackTotNum = 0;
			for(i = 0; i < VideoTotNum; i++)
			{
//				ty_media_handle->VideoTrackTotNum = 0;
				ty_media_handle->VideoInfo[i].VideoTrackNum = i + 1;	
				ret = typefind_codec_info( ty_media_handle, uri, CODEC_TYPE_VIDEO, i );
				if(ty_media_handle->VideoInfo[i].VCodecType == -1)
				{
					ret = ERROR_NOT_SUPPORT_CONTENTS;
					return ret;
				}
			}
			ty_media_handle->VideoTrackTotNum  = VideoTotNum;	
		}

		if(0 < AudioTotNum)
		{
			for(i = 0; i < AudioTotNum; i++)
			{
				ty_media_handle->AudioTrackTotNum = 0;
				ty_media_handle->AudioInfo[i].AudioTrackNum = i + 1;	
				ret = typefind_codec_info( ty_media_handle, uri, CODEC_TYPE_AUDIO, i );
				if(ty_media_handle->AudioInfo[i].ACodecType == -1)
				{
					ret = ERROR_NOT_SUPPORT_CONTENTS;
					return ret;
				}
			}
			ty_media_handle->AudioTrackTotNum  = AudioTotNum; 
		}

		if( ty_media_handle->VideoTrackTotNum <= 0 && ty_media_handle->AudioTrackTotNum >= 1) 
		{
			ty_media_handle->AudioOnly = ON;			
		}
		else if( ty_media_handle->VideoTrackTotNum >= 1 && ty_media_handle->AudioTrackTotNum <= 0) 
		{
			ty_media_handle->VideoOnly = ON;
		}
	}


	FUNC_OUT();

	return ERROR_NONE;
}


void NX_TypeFind_Close(TYMEDIA_INFO *ty_handle)
{
	FUNC_IN();

	free(ty_handle);
	
	ty_handle = NULL;

	FUNC_OUT();

}

