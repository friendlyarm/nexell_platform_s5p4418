#pragma GCC diagnostic ignored "-Wcast-align"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gst/gst.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <unistd.h>

#include "NX_DbgMsg.h"
#include <NX_MoviePlay.h>
#include <NX_TypeFind.h>

#define Audio_on	1
#define Video_on	1


typedef struct MOVIE_TYPE
{
	GstElement *pipeline;

	GstElement *input_src;

	GstElement *audio_queue;
	GstElement *video_queue;

	GstElement *demuxer;
	GstElement *video_parse;
	GstElement *audio_parse;

	GstElement *audio_decoder;
	GstElement *video_decoder;

	GstElement *volume;
	GstElement *typefind;

	GstElement *dec_queue;
	GstElement *audio_resample;
	GstElement *audio_convert;
	GstElement *audio_sink;
	GstElement *video_sink;

	GMainLoop *loop;

	GThread *thread;

	GstBus *bus;

	gboolean pipeline_is_linked;

	TYMEDIA_INFO TymediaInfo;

	gint contents_type;
	gint demux_type;
	gint video_type;
	gint audio_type;
	gint uri_type;
	gint audio_select_track_num;
	gint video_select_track_num;
	gchar *uri;

	gint is_buffering_paused;
	gint seek_state;	
	gint pause_state;
	gint play_state;	
	gint stop_state;

	gint display;

	void (*callback)(void*, guint, guint, guint );
	void *owner;

}MOVIE_TYPE;

typedef struct AppData{
	MP_HANDLE hPlayer;
}AppData;

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


static gpointer loop_func (gpointer data);
static void start_loop_thread (MP_HANDLE handle);
static void stop_my_thread (MP_HANDLE handle);
static gboolean bus_callback( GstBus *bus, GstMessage *msg, MP_HANDLE handle );
static void on_demux_pad_added(GstElement *element, GstPad *pad, MP_HANDLE handle);
static void on_decodebin_pad_added(GstElement *element, GstPad *pad, MP_HANDLE handle);
static int demux_element_search(MP_HANDLE handle);
static int audio_element_set(MP_HANDLE handle, int volumem, int dspPort);
static int audio_only_element_set(MP_HANDLE handle, int volumem, int dspPort);
static int audio_only_bin_add_link(MP_HANDLE handle);
static int video_only_bin_add_link(MP_HANDLE handle);
static int video_audio_bin_add_link(MP_HANDLE handle);
static int video_element_set(MP_HANDLE handle, int dspModule, int dspPort, int display, int priority);
static void typefind_debug(TYMEDIA_INFO *ty_handle);


static gpointer loop_func (gpointer data)
{
	FUNC_IN();

	MP_HANDLE handle = (MP_HANDLE )data;	
	g_main_loop_run (handle->loop); 
	
	FUNC_OUT();

	return NULL;
}

static void start_loop_thread (MP_HANDLE handle)
{
	FUNC_IN();

	handle->loop = g_main_loop_new (NULL, FALSE);
	handle->thread = g_thread_create (loop_func, handle, TRUE, NULL);

	FUNC_OUT();
}


static void stop_my_thread (MP_HANDLE handle)
{
	FUNC_IN();

	g_main_loop_quit (handle->loop);
	g_thread_join (handle->thread);
	g_main_loop_unref (handle->loop);

	FUNC_OUT();
}

static gboolean bus_callback( GstBus *bus, GstMessage *msg, MP_HANDLE handle )
{
	gchar  *debug;
	GError *err;
	FUNC_IN();

	switch (GST_MESSAGE_TYPE (msg))
	{
		case GST_MESSAGE_EOS:
			g_print ("End of stream(%p)\n", handle->callback);
			if( handle->callback )
			{
				g_print("Callback ++\n");
				handle->callback(handle->owner, CALLBACK_MSG_EOS, 0, 0);
				g_print("Callback --\n");
			}
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

static void on_demux_pad_added(GstElement *element, GstPad *pad, MP_HANDLE handle)
{
	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPadLinkReturn rc;
	GstElement *targetqueue;
	GstPad *targetsink;
//	gint video_track_num = 0;
//	gint audio_track_num = 0;
	gint audio_on = -1;
	gint video_on = -1;

	FUNC_IN();

//	video_track_num = handle->video_select_track_num;
//	audio_track_num = handle->audio_select_track_num ;

	caps = gst_pad_get_caps(pad);
	g_assert(caps != NULL);
	name = gst_pad_get_name(pad);
	//DbgMsg("-new demux pad %s\n", name);

	//
	if( (0 == strcasecmp( name, "private_2" ) ) )
	{
		goto EXIT;
	}
	//

	str = gst_caps_get_structure(caps, 0);
	g_assert(str != NULL);

	//DbgMsg("-compare string %s\n", gst_structure_get_name(str));

	targetqueue = NULL;
	// TODO: is this the right way to match video/audio pads

	if (g_strrstr(gst_structure_get_name(str), "video")) {
		//DbgMsg("-Linking %s to %s\n", name, gst_structure_get_name(str) );
		targetqueue = handle->video_queue;
		video_on = 0;
	}
#if Audio_on	
	else if (g_strrstr(gst_structure_get_name(str), "audio")) {
		//DbgMsg("-Linking %s to %s\n", name, gst_structure_get_name(str) );
		targetqueue = handle->audio_queue;
		audio_on = 0;
	}
#endif	

	if(0 == video_on)
	{
#if Video_on
//		if(0 == strcmp(name,(char *)handle->TymediaInfo.VideoInfo[video_track_num].VideoPadName))	
		{  
			if (targetqueue) {
				targetsink = gst_element_get_pad(targetqueue, "sink");
				g_assert(targetsink != NULL);			
				rc = gst_pad_link(pad, targetsink);			
				if (rc) {
					g_critical("demux pad link failed(%s): %d\n", __func__, rc);
				}
				gst_object_unref(targetsink);
			}
		}
#endif
	}
	else if(0 == audio_on)
	{
		if(handle->TymediaInfo.DemuxType == DEMUX_TYPE_MPEGTSDEMUX)	
		{
			if (targetqueue) {
				targetsink = gst_element_get_pad(targetqueue, "sink");
				g_assert(targetsink != NULL);			
				rc = gst_pad_link(pad, targetsink);			
				if (rc) {
					g_critical("demux pad link failed(%s): %d\n", __func__, rc);
				}
				else{
					g_print("demux pad link success\n");
				}
				gst_object_unref(targetsink);
			}
		}
//		else if(0 == strcmp(name,(char *)handle->TymediaInfo.AudioInfo[audio_track_num].AudioPadName))		
		else 
		{  
			if (targetqueue) {
				targetsink = gst_element_get_pad(targetqueue, "sink");
				g_assert(targetsink != NULL);			
				rc = gst_pad_link(pad, targetsink);			
				if (rc) {
					g_critical("demux pad link failed(%s): %d\n", __func__, rc);
				}
				gst_object_unref(targetsink);
			}
		}
	}

EXIT:

	g_free(name);	

	gst_caps_unref(caps);

	FUNC_OUT();

}

static void on_decodebin_pad_added(GstElement *element, GstPad *pad, MP_HANDLE handle)
{
	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPadLinkReturn rc;
	GstElement *targetqueue;
	GstPad *targetsink;

	FUNC_IN();

	caps = gst_pad_get_caps(pad);
	g_assert(caps != NULL);
	name = gst_pad_get_name(pad);
	//DbgMsg("-new decodebin pad %s\n", name);

	str = gst_caps_get_structure(caps, 0);
	g_assert(str != NULL);

	//DbgMsg("-compare string %s\n", gst_structure_get_name(str));

	targetqueue = NULL;
	// TODO: is this the right way to match /audio pads

	if (g_strrstr(gst_structure_get_name(str), "audio")) {
		//DbgMsg("-Linking %s to %s\n", name, gst_structure_get_name(str) );
		targetqueue = handle->volume;
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

	FUNC_OUT();

}

static int demux_element_search(MP_HANDLE handle)
{
	int ret = 1;

	handle->demux_type = handle->TymediaInfo.DemuxType;

	switch( handle->demux_type )
	{
	case DEMUX_TYPE_MPEGPSDEMUX:
		{
			if ( handle->audio_type == AUDIO_TYPE_AC3 )
				handle->demuxer = gst_element_factory_make ("dvddemux", "demuxer" );		//dvddemux				
			else
				handle->demuxer = gst_element_factory_make ("mpegpsdemux", "demuxer" );		//mpeg2
			handle->video_parse = gst_element_factory_make ("mpegvideoparse", "parse" );		//mpegvideoparse
			//g_print("==========dvddemux======\n");
			break;
		}
	case TYPE_3GP:
	case DEMUX_TYPE_QTDEMUX:
		{
			handle->demuxer = gst_element_factory_make ("qtdemux", "demuxer" );			//h264, mp4v, h263
			handle->video_parse = gst_element_factory_make ("h264parse", "parse" );					
			break;
		}
	case DEMUX_TYPE_OGGDEMUX:
		{
			handle->demuxer = gst_element_factory_make ("oggdemux", "demuxer" );		//theroa  .ogg
			break;
		}
	case DEMUX_TYPE_ANNODEX:
		{
			handle->demuxer = gst_element_factory_make ("oggdemux", "demuxer" );		//theroa  .ogg
			break;
		}
	case DEMUX_TYPE_RMDEMUX:
		{
			handle->demuxer = gst_element_factory_make ("rmdemux", "demuxer" );			//rmvb	
			break;
		}
	case DEMUX_TYPE_AVIDEMUX:
		{
			handle->demuxer = gst_element_factory_make ("avidemux", "demuxer" );		//divx			
			break;
		}
	case DEMUX_TYPE_ASFDEMUX:
		{
			handle->demuxer = gst_element_factory_make ("asfdemux", "demuxer" );		//vc1,wmv			
			break;
		}
	case DEMUX_TYPE_MATROSKADEMUX:
		{
			handle->demuxer = gst_element_factory_make ("matroskademux", "demuxer" );	//.mkv 
			break;
		}
	case DEMUX_TYPE_FLVDEMUX:
		{
			handle->demuxer = gst_element_factory_make ("flvdemux", "demuxer" );		//flv				
			break;
		}
	case DEMUX_TYPE_MPEGTSDEMUX:
		{
			handle->demuxer = gst_element_factory_make ("mpegtsdemux", "demuxer" );		//mpegtsdemux				
			handle->video_parse = gst_element_factory_make ("h264parse", "parse" );
			g_object_set (G_OBJECT(handle->demuxer), "program-number", handle->TymediaInfo.program_no[handle->video_select_track_num], NULL);
			break;
		}

	}

	if(	handle->demuxer == NULL ){
			g_printerr("Element NULL:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret = 0;
	}

	return ret;
}

static int audio_element_set(MP_HANDLE handle, int volumem, int dspPort)
{
	int ret = 1;
	double vol = 0;
	
	//	add audio queue,decoder,sink,volume
	handle->audio_queue = gst_element_factory_make ("queue2", "AudioQueue");
		
	if( handle->audio_type == AUDIO_TYPE_AC3 )
	{
		handle->audio_parse = gst_element_factory_make ("ac3parse", "audio_parse");
		handle->audio_decoder = gst_element_factory_make ("ffdec_ac3", "AudioDec");
		//DbgMsg("=====AUDIO_TYPE_AC3\n");
	}
	else if( handle->audio_type == AUDIO_TYPE_FLAC )
	{
		handle->audio_parse = gst_element_factory_make ("flacparse", "AudioParse");
		handle->audio_decoder = gst_element_factory_make ("ffdec_flac", "AudioDec");
		//DbgMsg("=====AUDIO_TYPE_FLAC\n");
	}
	else if( handle->audio_type == AUDIO_TYPE_DTS )
	{
		handle->audio_decoder = gst_element_factory_make ("ffdec_dca", "AudioDec");
		//DbgMsg("=====AUDIO_TYPE_DTS\n");
	}
	else
	{
		handle->audio_decoder = gst_element_factory_make ("decodebin", "AudioDec");
		//DbgMsg("=====decodebin\n");
	}

	handle->audio_convert  = gst_element_factory_make ("audioconvert", "audio-convert");	

	handle->audio_sink  = gst_element_factory_make ("alsasink", "audio-sink");	

	g_object_set (G_OBJECT (handle->audio_sink ), "device", "plug:dmix", NULL);

	handle->volume = gst_element_factory_make ("volume", "volume"); 
	g_assert (handle->volume != NULL);
	vol = (double)volumem/100.;
	g_object_set (G_OBJECT (handle->volume), "volume", vol, NULL);	

		//	HDMI
	if(DISPLAY_PORT_LCD == dspPort )	//Lcd
		g_object_set(G_OBJECT(handle->audio_sink), "device", "default", NULL);				
	else 								//Hdmi 
		g_object_set(G_OBJECT(handle->audio_sink), "device", "default:CARD=SPDIFTranscieve", NULL);	

	if(   handle->audio_queue == NULL 
		||handle->audio_decoder == NULL 
		||handle->volume == NULL 
		||handle->audio_convert == NULL 
		||handle->audio_sink == NULL )	{
			g_printerr("Element NULL:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret = 0;
	}

	return ret;
}

static int audio_only_element_set(MP_HANDLE handle, int volumem, int dspPort)
{
	int ret = 1;
	double vol = 0;
	
	//	add audio queue,decoder,sink,volume
	handle->audio_queue = gst_element_factory_make ("queue2", "AudioQueue");
		
	if( handle->audio_type == AUDIO_TYPE_AC3 )
	{
		handle->audio_parse = gst_element_factory_make ("ac3parse", "audio_parse");
		handle->audio_decoder = gst_element_factory_make ("ffdec_ac3", "AudioDec");
		//DbgMsg("=====AUDIO_TYPE_AC3\n");
	}
	else if( handle->audio_type == AUDIO_TYPE_FLAC )
	{
		handle->audio_parse = gst_element_factory_make ("flacparse", "AudioParse");
		handle->audio_decoder = gst_element_factory_make ("ffdec_flac", "AudioDec");
		//DbgMsg("=====AUDIO_TYPE_FLAC\n");
	}
	else if( handle->audio_type == AUDIO_TYPE_DTS )
	{
		handle->audio_parse = gst_element_factory_make ("dcaparse", "AudioParse");
		handle->audio_decoder = gst_element_factory_make ("ffdec_dca", "AudioDec");		
		//DbgMsg("=====AUDIO_TYPE_DTS\n");
	}
	else
	{
		handle->audio_decoder = gst_element_factory_make ("decodebin", "AudioDec");
		//DbgMsg("=====decodebin\n");
	}
	handle->audio_convert  = gst_element_factory_make ("audioconvert", "audio-convert");	

	handle->audio_sink  = gst_element_factory_make ("alsasink", "audio-sink");	

	g_object_set (G_OBJECT (handle->audio_sink ), "device", "plug:dmix", NULL);

	handle->volume = gst_element_factory_make ("volume", "volume"); 
	g_assert (handle->volume != NULL);
	vol = (double)volumem/100.;
	g_object_set (G_OBJECT (handle->volume), "volume", vol, NULL);	

	//	HDMI
	if(DISPLAY_PORT_LCD == dspPort )	//Lcd
		g_object_set(G_OBJECT(handle->audio_sink), "device", "default", NULL);				
	else 								//Hdmi 
		g_object_set(G_OBJECT(handle->audio_sink), "device", "default:CARD=SPDIFTranscieve", NULL);	

	if(	  handle->audio_queue == NULL 
		||handle->audio_decoder == NULL 
		||handle->volume == NULL 
		||handle->audio_convert == NULL 
		||handle->audio_sink == NULL )	{
			g_printerr("Element NULL:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret = 0;
		}

	return ret;
}

static int audio_only_bin_add_link(MP_HANDLE handle)
{
	int ret = 1;

	//		input-source | queue | audiodecoder	| volume | audio_convert | alsasink
	if( handle->audio_type == AUDIO_TYPE_AC3 )
	{
		gst_bin_add_many(	(handle->pipeline),
			handle->input_src,																	
			handle->audio_queue, 
			handle->audio_parse, 
			handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink,	
			NULL	);
	}
	else if( handle->audio_type == AUDIO_TYPE_FLAC || handle->audio_type ==  AUDIO_TYPE_DTS )
	{
		gst_bin_add_many(	(handle->pipeline),
			handle->input_src,																	
			handle->audio_queue, 
			handle->audio_parse, 
			handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink,	
			NULL	);
	}
	else
	{ 
		gst_bin_add_many(	(handle->pipeline),
			handle->input_src,			
			handle->audio_queue,
			handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink,		
			NULL	);

	}

	//audio link
	if(	   handle->audio_type == AUDIO_TYPE_AC3 )
	{
		ret = gst_element_link_many( handle->input_src,handle->audio_queue, handle->audio_parse, handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink, NULL );
		if(ret == 0){
			g_printerr("Error not link:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
	}
	else if( (handle->audio_type == AUDIO_TYPE_FLAC) || (handle->audio_type ==  AUDIO_TYPE_DTS) ) 
	{
		ret = gst_element_link_many( handle->input_src,handle->audio_queue, handle->audio_parse, handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink, NULL );
		if(ret == 0){
			g_printerr("Error not link:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
	}
	else {	
		ret = gst_element_link_many(  handle->input_src,handle->audio_queue, handle->audio_decoder, NULL  );
		if(ret == 0){
			g_printerr("Error not link:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}

		ret = gst_element_link_many(  handle->volume, handle->audio_convert, handle->audio_sink, NULL );
		if(ret == 0){
			g_printerr("Error not link:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
	}


	if(	   handle->audio_type != AUDIO_TYPE_AC3 
		&& handle->audio_type != AUDIO_TYPE_FLAC
		&& handle->audio_type !=  AUDIO_TYPE_DTS
		) {
			g_signal_connect( handle->audio_decoder, "pad-added", G_CALLBACK(on_decodebin_pad_added), handle );
	}

	return ret;
}

static int video_only_bin_add_link(MP_HANDLE handle)
{
	int ret = 1;
	
	if( handle->demux_type == DEMUX_TYPE_MPEGPSDEMUX || handle->demux_type == DEMUX_TYPE_MPEGTSDEMUX)
	{
		//		input-source | demux	| queue	| parse | videodecoder	| 			 videosink
		//								| queue			| audiodecoder	| volume    |alsasink
		gst_bin_add_many(	(handle->pipeline),
							handle->input_src,																	//	input_src 
							handle->demuxer,																	//	demux
							handle->video_queue, handle->video_parse, handle->video_decoder, handle->video_sink,		//	video						
							NULL	);
		//	link
		ret  = gst_element_link( handle->input_src, handle->demuxer );
		if(ret == 0){
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
		ret  = gst_element_link_many ( handle->video_queue, handle->video_parse, handle->video_decoder, handle->video_sink, NULL );	
		if(ret == 0){
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
	}
	else
	{
		//		input-source | demux	| queue	| videodecoder	| 			 videosink
		//								| queue	| audiodecoder	| volume    |alsasink
		gst_bin_add_many(	(handle->pipeline),
							handle->input_src,																	//	input_src 
							handle->demuxer,																	//	demux
							handle->video_queue,  handle->video_decoder, handle->video_sink,					//	video						
							NULL	);


		//link
		ret  = gst_element_link( handle->input_src, handle->demuxer );
		if(ret == 0){
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}

		//video	link
		ret  = gst_element_link_many ( handle->video_queue, handle->video_decoder, handle->video_sink, NULL );	
		if(ret == 0){
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
	}

	g_signal_connect( handle->demuxer, "pad-added", G_CALLBACK(on_demux_pad_added), handle );

	return ret;
}

static int video_audio_bin_add_link(MP_HANDLE handle)
{
	int ret = 1;
	
	if( handle->demux_type == DEMUX_TYPE_MPEGPSDEMUX || handle->demux_type == DEMUX_TYPE_MPEGTSDEMUX)
	{
		//		input-source | demux	| queue	| parse | videodecoder	| 			 videosink
		//								| queue			| audiodecoder	| volume    |alsasink
		if( handle->audio_type == AUDIO_TYPE_AC3)
		{  
			gst_bin_add_many(	(handle->pipeline),
								handle->input_src,																	//	input_src 
								handle->demuxer,																	//	demux
#if Video_on
								handle->video_queue, handle->video_parse, handle->video_decoder, handle->video_sink,		//	video						
#endif
#if Audio_on						
								handle->audio_queue, handle->audio_parse, handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink,		//	audio
#endif						
								NULL	);
		}
		else
		{
			gst_bin_add_many(	(handle->pipeline),
								handle->input_src,																	//	input_src 
								handle->demuxer,																	//	demux
#if Video_on
								handle->video_queue, handle->video_parse, handle->video_decoder, handle->video_sink,		//	video						
#endif
#if Audio_on						
								handle->audio_queue, handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink,		//	audio
#endif						
								NULL	);			
		}
		//	link
		ret  = gst_element_link( handle->input_src, handle->demuxer );
		if(ret == 0){
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
		ret  = gst_element_link_many ( handle->video_queue, handle->video_parse, handle->video_decoder, handle->video_sink, NULL );	
		if(ret == 0){
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
	}
	else
	{
		//		input-source | demux	| queue	| videodecoder	| 			 videosink
		//								| queue	| audiodecoder	| volume    |alsasink
		if(handle->audio_type == AUDIO_TYPE_AC3 
			|| handle->audio_type == AUDIO_TYPE_FLAC
			|| handle->audio_type ==  AUDIO_TYPE_DTS
			)
		{
			gst_bin_add_many(	(handle->pipeline),
								handle->input_src,																	//	input_src 
								handle->demuxer,																	//	demux
#if Video_on
								handle->video_queue, handle->video_decoder, handle->video_sink,						//	video
#endif
#if Audio_on						
								handle->audio_queue, handle->audio_parse, handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink,		//	audio
#endif	
								NULL	);
		}
		else
		{
			gst_bin_add_many(	(handle->pipeline),
								handle->input_src,																	//	input_src 
								handle->demuxer,																	//	demux
#if Video_on
								handle->video_queue,  handle->video_decoder, handle->video_sink,					//	video						
#endif
#if Audio_on						
								handle->audio_queue, handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink,		//	audio
#endif						
								NULL	);
		}


		//link
		ret  = gst_element_link( handle->input_src, handle->demuxer );
		if(ret == 0){
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
#if Video_on
		//video	link
		ret  = gst_element_link_many ( handle->video_queue, handle->video_decoder, handle->video_sink, NULL );	
		if(ret == 0){
			g_print("Error not link !!!:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			return ret;
		}
#endif

	}

#if Audio_on	
	//audio link
	if(	handle->audio_type == AUDIO_TYPE_AC3 
		|| handle->audio_type == AUDIO_TYPE_FLAC
		|| handle->audio_type ==  AUDIO_TYPE_DTS
		)
	{
		gst_element_link_many( handle->audio_queue, handle->audio_parse, handle->audio_decoder, handle->volume, handle->audio_convert, handle->audio_sink );
	}
	else
	{
		gst_element_link( handle->audio_queue, handle->audio_decoder );
		gst_element_link_many( handle->volume, handle->audio_convert, handle->audio_sink, NULL );
	}
	
#endif	
		
	g_signal_connect( handle->demuxer, "pad-added", G_CALLBACK(on_demux_pad_added), handle );
#if Audio_on
	if( handle->audio_type != AUDIO_TYPE_AC3 
		&& handle->audio_type != AUDIO_TYPE_FLAC
		&& handle->audio_type !=  AUDIO_TYPE_DTS
		)
		g_signal_connect( handle->audio_decoder, "pad-added", G_CALLBACK(on_decodebin_pad_added), handle );
#endif	


	return ret;
}

static int video_element_set(MP_HANDLE handle, int dspModule, int dspPort, int display, int priority)
{
	int ret = 1;

	//add video queue,decoder,sink
	if( (handle->demux_type == DEMUX_TYPE_MPEGPSDEMUX) && (handle->audio_type == AUDIO_TYPE_AC3) )
		handle->video_queue = gst_element_factory_make ("queue", "VideoQueue");
	else
		handle->video_queue = gst_element_factory_make ("queue2", "VideoQueue");

	handle->video_decoder = gst_element_factory_make ("nxvideodecoder", "VideoDec");

	handle->video_sink = gst_element_factory_make ("nxvideosink", "video-sink");
	g_object_set(G_OBJECT(handle->video_sink), "dsp-module", dspModule, NULL);
	g_object_set(G_OBJECT(handle->video_sink), "dsp-port", dspPort, NULL);
	g_object_set(G_OBJECT(handle->video_sink), "dsp-priority", priority, NULL);

	if( ( dspModule == 0) && (dspPort == 0) && (display == DISPLAY_PORT_LCD) )	//lcd
	{
		g_object_set(G_OBJECT(handle->video_sink), "lcd-display", ON, NULL);
		g_object_set(G_OBJECT(handle->video_sink), "hdmi-display", OFF, NULL);
//		g_printf("====( dspModule == 0) && (dspPort == 0)\n");
	}
	else if( ( dspModule == 1) && (dspPort == 1) && (display == DISPLAY_PORT_HDMI) )	//hdmi
	{
		g_object_set(G_OBJECT(handle->video_sink), "lcd-display", OFF, NULL);
		g_object_set(G_OBJECT(handle->video_sink), "hdmi-display", ON, NULL);
//		g_printf("====( dspModule == 1) && (dspPort == 1)\n");
	}
	else if( ( dspModule == 0) && (dspPort == 0) && (display == DISPLAY_PORT_DUAL) )	//dual
	{
		g_object_set(G_OBJECT(handle->video_sink), "lcd-display",  ON, NULL);
		g_object_set(G_OBJECT(handle->video_sink), "hdmi-display", ON, NULL);
//		g_object_set(G_OBJECT(handle->video_sink), "dual-display", ON, NULL);
	}

	

	if(handle->demux_type == DEMUX_TYPE_MPEGTSDEMUX)
	{
		handle->video_parse = gst_element_factory_make ("h264parse", "VideoParse");
		g_object_set(G_OBJECT(handle->video_sink), "src-width", handle->TymediaInfo.VideoInfo[handle->video_select_track_num].Width, NULL);			
		g_object_set(G_OBJECT(handle->video_sink), "src-height",handle->TymediaInfo.VideoInfo[handle->video_select_track_num].Height, NULL);
		
	}

	if(	handle->video_queue == NULL 
		||handle->video_decoder == NULL 
		||handle->video_sink == NULL )	{
			g_printerr("Element NULL:%s:%s:Line(%d) \n", __FILE__, __func__, __LINE__);
			ret = 0;
		}

	return ret;
}

static void typefind_debug(TYMEDIA_INFO *ty_handle)
{
	gint i = 0;

	g_print("\n");
	g_print("===============================================================================\n");
	g_print("    DemuxType : %d, %s \n", ty_handle->DemuxType, DemuxTypeString[ty_handle->DemuxType]);
	if(ty_handle->VideoTrackTotNum > 0)
	{
		g_print("-------------------------------------------------------------------------------\n");
		g_print("                       Video Information \n");
		g_print("-------------------------------------------------------------------------------\n");
		g_print("    VideoTrackTotNum: %d  \n",  (int)ty_handle->VideoTrackTotNum);
		g_print("\n");
		for(i = 0; i < (int)ty_handle->VideoTrackTotNum; i++)
		{
			g_print("    VideoTrackNum	: %d  \n",  (int)ty_handle->VideoInfo[i].VideoTrackNum);
			g_print("    VCodecType		: %d, %s \n", (int)ty_handle->VideoInfo[i].VCodecType, VideoTypeString[ty_handle->VideoInfo[i].VCodecType] );
			g_print("    Width		: %d  \n",  (int)ty_handle->VideoInfo[i].Width);
			g_print("    Height		: %d  \n",(int)ty_handle->VideoInfo[i].Height);
			if(ty_handle->VideoInfo[i].Framerate.value_numerator == 0 || ty_handle->VideoInfo[i].Framerate.value_denominator == 0)
				g_print("    Framerate		: %f  \n", 0.0); 
			else
				g_print("    Framerate		: %f  \n", (float)ty_handle->VideoInfo[i].Framerate.value_numerator/ty_handle->VideoInfo[i].Framerate.value_denominator); 
			g_print("\n");
			g_print("\n");
		}
	}
	if(ty_handle->AudioTrackTotNum > 0)
	{
		g_print("-------------------------------------------------------------------------------\n");
		g_print("                       Audio Information \n");
		g_print("-------------------------------------------------------------------------------\n");
		g_print("    AudioTrackTotNum: %d  \n",  (int)ty_handle->AudioTrackTotNum);
		g_print("\n");
		for(i = 0; i < (int)ty_handle->AudioTrackTotNum; i++)
		{
			g_print("    AudioTrackNum	: %d  \n",  (int)ty_handle->AudioInfo[i].AudioTrackNum);
			g_print("    ACodecType		: %d, %s \n", (int)ty_handle->AudioInfo[i].ACodecType, AudioTypeString[ty_handle->AudioInfo[i].ACodecType] );
			g_print("    samplerate		: %d  \n",  (int)ty_handle->AudioInfo[i].samplerate);
			g_print("    channels		: %d  \n",(int)ty_handle->AudioInfo[i].channels);
			g_print("\n");
			g_print("\n");
		}
	}
	g_print("===============================================================================\n\n");
}

MP_RESULT NX_MPSetFileName(MP_HANDLE *handle_s, const char *uri, char *media_info)
{
	MP_HANDLE handle = NULL;
	TYMEDIA_INFO *ty_handle = (TYMEDIA_INFO*)media_info;
	gint uri_len = 0;
	int ret = 0;
	gint i = 0;

	FUNC_IN();
	if( NULL == (handle = (MP_HANDLE)malloc( sizeof(MOVIE_TYPE) ))) {
		return ERROR_HANDLE;
	}

	memset( handle, 0, sizeof(MOVIE_TYPE) );
	if( ( strncmp( uri, "http://", 7 ) == 0 ) || ( strncmp( uri, "https://", 8 ) == 0 ) ) {
		handle->uri_type = URI_TYPE_URL;
	}
	else {
		handle->uri_type = URI_TYPE_FILE;
	}

	if( 4 >= (uri_len = strlen( uri ) )) {
		goto ERROR_EXIT;
	}
	handle->uri = strdup( uri );

	// check uri
	if( access(uri, F_OK) ) {
		goto ERROR_EXIT;
	}

	//typefind
	if( (ty_handle->AudioTrackTotNum <=0 && ty_handle->VideoTrackTotNum <=0) )
	{
		ret  = NX_TypeFind_Open( &ty_handle );
		if(ret != ERROR_NONE)
		{
			g_error ("Error NX_TypeFind_Open!! ");
			goto ERROR_EXIT;
		}

		ret = NX_TypeFind( ty_handle, uri );

		memcpy(&handle->TymediaInfo, ty_handle, sizeof(TYMEDIA_INFO) );

		typefind_debug( ty_handle);

		NX_TypeFind_Close(ty_handle);

		if(ret != ERROR_NONE)
		{
			g_error ("ERROR Not Support Contents!! \n ");
			goto ERROR_EXIT;
		}
	}
	else
	{
		memcpy(&handle->TymediaInfo, media_info, sizeof(TYMEDIA_INFO) );
	}

	//typefind end

	*handle_s = handle;

	FUNC_OUT();
	return ERROR_NONE;

ERROR_EXIT:

	if( handle ){
		free( handle );
	}

	return ERROR;
}

MP_RESULT NX_MPOpen( MP_HANDLE handle, int volumem, int dspModule, int dspPort, int audio_track_num, int video_track_num, int display,
						void (*cb)(void *owner, unsigned int msg, unsigned int param1, unsigned int param2), void *cbPrivate)
{
	int ret = 0;
	int priority = 0;

	if( !handle )
		goto ERROR_EXIT;
	
	if( audio_track_num < 0 )
		audio_track_num = 0;

	if( video_track_num < 0 )
		video_track_num = 0;

	FUNC_IN();

	handle->owner 					= cbPrivate;
	handle->callback 				= cb;

	handle->display 				= display;
	handle->audio_select_track_num 	= audio_track_num;
	handle->video_select_track_num 	= video_track_num;

	handle->audio_type =handle->TymediaInfo.AudioInfo[handle->audio_select_track_num].ACodecType;
	handle->video_type =handle->TymediaInfo.VideoInfo[handle->video_select_track_num].VCodecType;

	if( !gst_is_initialized() ){
		gst_init(NULL, NULL);
	}

	if( handle->pipeline_is_linked ) {
		DbgMsg("%s() : Already Initialized\n",__func__);
		return ERROR_NONE;
	}

	handle->pipeline = gst_pipeline_new ("Movie Player");
	if( NULL==handle->pipeline ) {
		g_print("NULL==handle->pipeline\n");
		goto ERROR_EXIT;
	}

	//	Add bus watch
	handle->bus = gst_pipeline_get_bus( (GstPipeline*)(handle->pipeline) );
	gst_bus_add_watch ( handle->bus, (GstBusFunc)bus_callback, handle );
	gst_object_unref ( GST_OBJECT(handle->bus) );

	/* Create gstreamer elements */
	switch( handle->uri_type )
	{
	case URI_TYPE_FILE:
		handle->input_src = gst_element_factory_make ("filesrc", "file-source");
		break;
	case URI_TYPE_URL :
		handle->input_src = gst_element_factory_make ("souphttpsrc", NULL);
		break;
	}

	if( NULL==handle->input_src )
	{
		g_print ("NULL, Could not create 'input_src' element\n");
		goto ERROR_EXIT;
	}

	// we set the input filename to the source element
	g_object_set (G_OBJECT (handle->input_src), "location", handle->uri, NULL);

	if(handle->TymediaInfo.AudioOnly == ON){		
		//DbgMsg("TymediaInfo.AudioOnly == ON)\n");
		//	add audio queue,decoder,sink,volume
		ret = audio_only_element_set(handle, volumem, dspPort);
		if(ret == 0){
			goto ERROR_EXIT;
		}		

		ret = audio_only_bin_add_link(handle);
		if(ret == 0){
			goto ERROR_EXIT;
		}		

	}
	else if(handle->TymediaInfo.VideoOnly == ON){
		//add video queue,decoder,sink
		ret = video_element_set( handle,  dspModule,  dspPort, display, priority);
		if(ret == 0){
			goto ERROR_EXIT;
		}		
		ret = demux_element_search(handle);
		if(ret == 0){
			goto ERROR_EXIT;
		}		
		ret = video_only_bin_add_link(handle);
		if(ret == 0){
			goto ERROR_EXIT;
		}		
	}			
	else { //audio + video		
		//	add audio queue,decoder,sink,volume
		ret = audio_element_set(handle, volumem, dspPort);
		if(ret == 0){
			goto ERROR_EXIT;
		}		
		//add video queue,decoder,sink
		ret = video_element_set( handle,  dspModule, dspPort, display, priority);
		if(ret == 0){
			goto ERROR_EXIT;
		}		
		ret = demux_element_search(handle);
		if(ret == 0){
			goto ERROR_EXIT;
		}		
		ret = video_audio_bin_add_link(handle);
		if(ret == 0){
			goto ERROR_EXIT;
		}		
	}

	if( GST_STATE_CHANGE_FAILURE == gst_element_set_state (handle->pipeline, GST_STATE_READY) )
	{
		DbgMsg("%s() : GST_STATE_CHANGE_FAILURE\n",__func__);
		return ERROR;
	}

	if( GST_STATE_CHANGE_FAILURE == gst_element_get_state( handle->pipeline, NULL, NULL, -1 ) )
	{
		DbgMsg("%s() : GST_STATE_CHANGE_FAILURE_1\n",__func__);
		return ERROR;
	}

	handle->pipeline_is_linked = TRUE;
	start_loop_thread( handle );

	FUNC_OUT();

	return ERROR_NONE;

ERROR_EXIT:
	if( handle ){
		free( handle );
	}

	return ERROR;
}

void NX_MPClose( MP_HANDLE handle )
{
	FUNC_IN();
	if( !handle )
		return;

	if( handle->pipeline_is_linked )
	{
		stop_my_thread( handle );
		gst_element_set_state (handle->pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (handle->pipeline));
	}

	if(handle->uri)
		free( handle->uri );
	free( handle );
	FUNC_OUT();
}

MP_RESULT NX_MPGetMediaInfo( MP_HANDLE handle, int idx, MP_MEDIA_INFO *pInfo )
{
	FUNC_IN();
//	int ret = 0;
	NX_MPGetCurDuration(handle,  (unsigned int *)&pInfo->VDuration);
	if(handle->TymediaInfo.VideoInfo[idx].Framerate.value_numerator == 0 || handle->TymediaInfo.VideoInfo[idx].Framerate.value_denominator == 0)
		pInfo->Framerate = 0;
	else
		pInfo->Framerate = (unsigned long)handle->TymediaInfo.VideoInfo[idx].Framerate.value_numerator/handle->TymediaInfo.VideoInfo[idx].Framerate.value_denominator;
	pInfo->Width = handle->TymediaInfo.VideoInfo[idx].Width;
	pInfo->Height = handle->TymediaInfo.VideoInfo[idx].Height;
	NX_MPGetCurDuration(handle, (unsigned int *)&pInfo->ADuration);
	pInfo->samplerate = (unsigned long)handle->TymediaInfo.AudioInfo[idx].samplerate;
	pInfo->channels = handle->TymediaInfo.AudioInfo[idx].channels;
	pInfo->bitrate = -1;

#if 0
	g_print("\n");
	g_print("===============================================================================\n");
	if( 0 != handle->TymediaInfo.VideoTrackTotNum)
	{
		g_print("-------------------------------------------------------------------------------\n");
		g_print("                       Video Information \n");
		g_print("-------------------------------------------------------------------------------\n");	
		g_print("    VDuration		: %d ms  \n",  (int)pInfo->VDuration);
		g_print("    Framerate		: %d \n", (int)pInfo->Framerate );
		g_print("    Width		: %d  \n",  (int)pInfo->Width);
		g_print("    Height		: %d  \n",(int)pInfo->Height);
		g_print("\n");
		g_print("\n");
	}
	if( 0 != handle->TymediaInfo.AudioTrackTotNum)
	{
		g_print("-------------------------------------------------------------------------------\n");
		g_print("                       Audio Information \n");
		g_print("-------------------------------------------------------------------------------\n");
		g_print("    ADuration		: %d ms \n",  (int)pInfo->ADuration);
		g_print("    samplerate		: %d \n", (int)pInfo->samplerate );
		g_print("    channels		: %d  \n",  (int)pInfo->channels);
		//		g_print("    bitrate		: %d  \n",(int)pInfo->bitrate);
		g_print("\n");
		g_print("\n");
	}
	g_print("===============================================================================\n\n");
#endif

	FUNC_OUT();
	return	ERROR_NONE;
}

MP_RESULT NX_MPPlay( MP_HANDLE handle, float speed )
{
	GstStateChangeReturn ret;
	FUNC_IN();
	if( !handle || !handle->pipeline_is_linked )
	{
		ErrMsg("%s() : invalid state or invalid operation.(%p,%d)\n", __func__, handle, handle->pipeline_is_linked);
		return ERROR;
	}
	ret = gst_element_set_state (handle->pipeline, GST_STATE_PLAYING);
	if( GST_STATE_CHANGE_FAILURE == ret ){
		ErrMsg("%s() : Sate change failed!!!(ret=%d)\n", __func__, ret);
		return ERROR;
	}

	FUNC_OUT();
	return	ERROR_NONE;
}


MP_RESULT NX_MPPause(MP_HANDLE hande)
{
	GstStateChangeReturn ret;

	FUNC_IN();

	if( !hande || !hande->pipeline_is_linked )
	{
		ErrMsg("%s() : invalid state or invalid operation.(%p,%d)\n", __func__, hande, hande->pipeline_is_linked);
		return ERROR;
	}
	ret = gst_element_set_state (hande->pipeline, GST_STATE_PAUSED);
	if( GST_STATE_CHANGE_FAILURE == ret ){
		ErrMsg("%s() : Sate change failed!!!(ret=%d)\n", __func__, ret);
		return ERROR;
	}

	FUNC_OUT();

	return	ERROR_NONE;
}

MP_RESULT NX_MPStop(MP_HANDLE handle)
{
	GstStateChangeReturn ret;

	FUNC_IN();

	if( !handle || !handle->pipeline_is_linked )
	{
		ErrMsg("%s() : invalid state or invalid operation.(%p,%d)\n", __func__, handle, handle->pipeline_is_linked);
		return ERROR;
	}
//	ret = gst_element_set_state (handle->pipeline, GST_STATE_READY);
	ret = gst_element_set_state (handle->pipeline, GST_STATE_NULL);
	
	if( GST_STATE_CHANGE_FAILURE == ret ){
		ErrMsg("%s() : Sate change failed!!!(ret=%d)\n", __func__, ret);
		return ERROR;
	}

	FUNC_OUT();
	return	ERROR_NONE;
}

MP_RESULT NX_MPSeek(MP_HANDLE handle, unsigned int seekTime)
{
	GstState state, pending;
	GstStateChangeReturn ret;
	unsigned int milliSeconds = 0;
	gint64 seekTimeUs = 0;
	milliSeconds = seekTime;
	seekTimeUs = (gint64)milliSeconds*1000000;
	FUNC_IN();
	if( !handle || !handle->pipeline_is_linked )
	{
		ErrMsg("%s() : invalid state or invalid operation.(%p,%d)\n", __func__, handle, handle->pipeline_is_linked);
		return ERROR;
	}

	if(   (handle->TymediaInfo.AudioInfo[handle->audio_select_track_num].ACodecType == AUDIO_TYPE_RA ) 
		||(handle->TymediaInfo.AudioInfo[handle->audio_select_track_num].ACodecType == AUDIO_TYPE_DTS )
		)
	{
		g_printf(" Not Support Seek RA, DTS !!!\n");
		return	ERROR_NONE;
	}

	ret = gst_element_get_state( handle->pipeline, &state, &pending, 500000000 );		//	wait 500 msec

	if( GST_STATE_CHANGE_FAILURE != ret )
	{
		if( state == GST_STATE_PLAYING || state == GST_STATE_PAUSED || state == GST_STATE_READY )
		{  

			if (!gst_element_seek_simple (handle->pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT , seekTimeUs))
			{
				g_printerr ("Seek failed (%d msec)\n", milliSeconds);
				return ERROR;
			}
		}
		else
		{
			ErrMsg("AUDP ERR STATE!!!\n");
			return ERROR;	//	Invalid state
		}
	}
	else
	{
		ErrMsg("AUDP ERR STATE!!!!\n");	
		return ERROR;
	}

	FUNC_OUT();
	return	ERROR_NONE;
}


MP_RESULT NX_MPGetCurDuration(MP_HANDLE handle, unsigned int *duration)
{
	GstState state, pending;
	GstStateChangeReturn ret;
	unsigned int pMilliSeconds = 0;
	FUNC_IN();
	if( !handle || !handle->pipeline_is_linked )
	{
		ErrMsg("%s() : invalid state or invalid operation.(%p,%d)\n", __func__, handle, handle->pipeline_is_linked);
		return ERROR;
	}

	ret = gst_element_get_state( handle->pipeline, &state, &pending, 500000000 );		//	wait 500 msec
	if( GST_STATE_CHANGE_FAILURE != ret )
	{
		if( state == GST_STATE_PLAYING || state == GST_STATE_PAUSED || state==GST_STATE_READY )
		{
			gint64 len;
			GstFormat format = GST_FORMAT_TIME;
			if (gst_element_query_duration (handle->pipeline, &format, &len))
			{
				pMilliSeconds = (unsigned int)((len / 1000000));	
				*duration = pMilliSeconds;
				VbsMsg("Duration: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS (len));	//
			}
		}
		else
		{
			//DbgMsg("============ Invalid Status\n");
			return ERROR;	//	Invalid state
		}
	}
	else
	{
		//DbgMsg("============ Invalid Status 2\n");
		return ERROR;
	}


//	DbgMsg( "state(%d) , GST_STATE_PLAYING(%d) , GST_STATE_PAUSED(%d) , GST_STATE_READY(%d)\n",
//		state, GST_STATE_PLAYING, GST_STATE_PAUSED, GST_STATE_READY );

	FUNC_OUT();

	return	ERROR_NONE;
}


MP_RESULT NX_MPGetCurPosition(MP_HANDLE handle, unsigned int *position)
{
	GstState state, pending;
	GstStateChangeReturn ret;
	unsigned int pMilliSeconds = 0;
	FUNC_IN();
	if( !handle || !handle->pipeline_is_linked )
	{
		ErrMsg("%s() : invalid state or invalid operation.(%p,%d)\n", __func__, handle, handle->pipeline_is_linked);
		return ERROR;
	}

	ret = gst_element_get_state( handle->pipeline, &state, &pending, 500000000 );		//	wait 500 msec
	if( GST_STATE_CHANGE_FAILURE != ret )
	{
		if( state == GST_STATE_PLAYING || state == GST_STATE_PAUSED )
		{
			gint64 pos;
			GstFormat format = GST_FORMAT_TIME;
			if (gst_element_query_position(handle->pipeline, &format, &pos))
			{
				pMilliSeconds = (unsigned int)((pos / 1000000));
				*position = pMilliSeconds;
				VbsMsg("Position: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS (pos));	//
			}
		}
		else
		{
			return ERROR;	//	Invalid state
		}
	}
	else
	{
		return ERROR;
	}

	FUNC_OUT();

	return ERROR_NONE;

}


MP_RESULT NX_MPSetDspPosition(MP_HANDLE handle,	int dspModule, int dsPport, int x, int y, int width, int height )
{
	FUNC_IN();
	if( handle && handle->video_sink )
	{
		if( ( dspModule == 0) && (dsPport == 0) )
		{
			//g_print("===lcd display===\n");
			g_object_set(G_OBJECT(handle->video_sink), "lcd-display", ON, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-module", dspModule, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-port", dsPport, NULL);			
			g_object_set(G_OBJECT(handle->video_sink), "dsp-x", x, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-y", y, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-width", width, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-height", height, NULL);
		}
		else if( ( dspModule == 1) && (dsPport == 1) )
		{
			//g_print("===hdmi display===\n");
			g_object_set(G_OBJECT(handle->video_sink), "hdmi-display", ON, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-module", dspModule, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-port", dsPport, NULL);			
			g_object_set(G_OBJECT(handle->video_sink), "dsp-x", x, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-y", y, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-width", width, NULL);
			g_object_set(G_OBJECT(handle->video_sink), "dsp-height", height, NULL);
		}

	}
	FUNC_OUT();
	return	ERROR_NONE;
}


MP_RESULT NX_MPSetVolume(MP_HANDLE handle, int volume)
{
	FUNC_IN();
	if( handle && handle->audio_sink && handle->volume )
	{
		double vol = (double)volume/100.;
		printf("NX_MPSetVolume = %f\n", vol);
		g_object_set (G_OBJECT (handle->volume), "volume", vol, NULL);		
	}
	FUNC_OUT();
	return	ERROR_NONE;
}

