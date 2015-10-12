#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gst/gst.h>
#include <glib.h>
#include "NX_Util.h"

long long NX_GetTime()
{
	struct timeval tv;
	long long t;
	gettimeofday( &tv, NULL );
	t = tv.tv_sec * 1000;
	if( tv.tv_usec != 0 ){
		t += tv.tv_usec/1000;
	}
	return t;
}

void SWVolume(int request_volume,int sample_size, short *pcm_sample )
{
	int audio_volume = 0;
	short *p_samples = NULL;
	int L_tmp = 0;
	int i = 0;

	audio_volume = request_volume * (float)2.56;
	if (audio_volume <= 256) 
	{			
		p_samples = (short *)pcm_sample;
		for(i=0;i<(int)(sample_size / sizeof(short));i++) 
		{
			L_tmp = ((*p_samples) * audio_volume + 128) >> 8;
			if (L_tmp < -32768) L_tmp = -32768;
			if (L_tmp >  32767) L_tmp = 32767;
			*p_samples++ = (short)L_tmp;
		}
	}

}



