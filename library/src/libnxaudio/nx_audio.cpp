//------------------------------------------------------------------------------
//
//  Copyright (C) 2013 Nexell Co. All Rights Reserved
//  Nexell Co. Proprietary & Confidential
//
//  NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//  Module      :
//  File        :
//  Description :
//  Author      : 
//  Export      :
//  History     :
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <alsa/asoundlib.h>

#include <CNX_WaveOutApi.h>
#include <nx_audio.h>

NX_AUDIO_HANDLE		NX_AudioInit( void )
{
	CNX_WaveFilePlayer *handle = new CNX_WaveFilePlayer();
	
	if( handle == NULL ) {
		printf("Audio Handle create filaed! \n");
		return NULL;
	}

	return (NX_AUDIO_HANDLE)handle;
}

void	NX_AudioDeinit( NX_AUDIO_HANDLE hAudio )
{
	CNX_WaveFilePlayer *pHandle = (CNX_WaveFilePlayer*)hAudio;

	if( pHandle )
		delete pHandle;
}

int32_t	NX_AudioPlay( NX_AUDIO_HANDLE hAudio, const char *filename )
{
	CNX_WaveFilePlayer *pHandle = (CNX_WaveFilePlayer*)hAudio;
	int32_t error = 0, isMix = 0;

	assert(pHandle);

	pHandle->Stop( true );
	error = pHandle->Start( filename, isMix );
	if( error != 0 ) {
		printf("Audio play failed!\n");
		return -1;
	}
	
	return 0;
}

int32_t	NX_AudioStop( NX_AUDIO_HANDLE hAudio, bool bImmediate )
{
	CNX_WaveFilePlayer *pHandle = (CNX_WaveFilePlayer*)hAudio;
	int32_t bDrop = 1;

	assert(pHandle);

	if(!bImmediate) {
		while( pHandle->m_bRunning )
		{
			usleep(100000);
		}
	}
	
	pHandle->Stop( bDrop );
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
typedef struct db_info {
	float min;
	float step;
} AUDIO_DB_INFO;

#define AMIXER_CONTROL_SPK_VOLUME	"Speaker Playback Volume"
#define AMIXER_CONTROL_MIC1_VOLUME	"MIC1 Boost"
#define AMIXER_CONTROL_MIC2_VOLUME	"MIC2 Boost"

void DecodeTlv(AUDIO_DB_INFO *dbInfo, uint32_t *tlv, uint32_t tlv_size)
{
	uint32_t type = tlv[0];
	uint32_t size;
	uint32_t idx = 0;

	if (tlv_size < 2 * sizeof(uint32_t)) {
		printf("TLV size error!\n");
		return;
	}

	type = tlv[idx++];
	size = tlv[idx++];
	tlv_size -= 2 * sizeof(uint32_t);

	if (size > tlv_size) {
		printf("TLV size error (%i, %i, %i)!\n", type, size, tlv_size);
		return;
	}
	switch (type) {
	case SND_CTL_TLVT_CONTAINER:
		size += sizeof(uint32_t) -1;
		size /= sizeof(uint32_t);
		while (idx < size) {
			if (tlv[idx+1] > (size - idx) * sizeof(uint32_t)) {
				printf("TLV size error in compound!\n");
				return;
			}
			DecodeTlv(dbInfo, tlv + idx, tlv[idx+1]);
			idx += 2 + (tlv[1] + sizeof(uint32_t) - 1) / sizeof(uint32_t);
		}
		break;
	case SND_CTL_TLVT_DB_SCALE:
		//printf("dBscale-");
		if (size != 2 * sizeof(uint32_t)) {
			while (size > 0) {
				//printf("0x%08x,", tlv[idx]);
				idx++;
				size -= sizeof(uint32_t);
			}
		} else {
			int db_min, db_step;

			//printf("min=");
			//printf("%d\n", (int)tlv[2]);
			//printf(",step=");
			//printf("%d\n", (int)tlv[3] & 0xffff);
			//printf(",mute=%i", (tlv[3] >> 16) & 1);
			
			db_min = (int)tlv[2];
			db_step = (int)tlv[3] & 0xffff;
			
			dbInfo->min = db_min / 100.;
			dbInfo->step = db_step / 100.;
		}
		break;
	case SND_CTL_TLVT_DB_LINEAR:
		//printf("dBlinear-");
		if (size != 2 * sizeof(uint32_t)) {
			while (size > 0) {
				//printf("0x%08x,", tlv[idx]);
				idx++;
				size -= sizeof(uint32_t);
			}
		} else {
			//printf("min=");
			//printf("%d\n", (int)tlv[2]);
			//printf(",max=");
			//printf("%d\n", (int)tlv[3]);
		}
		break;
	case SND_CTL_TLVT_DB_RANGE:
		//printf("dBrange-\n");
		if ((size % (6 * sizeof(uint32_t))) != 0) {
			while (size > 0) {
				//printf("0x%08x,", tlv[idx]);
				idx++;
				size -= sizeof(uint32_t);
			}
			break;
		}
		while (size > 0) {
			//print_spaces(spaces + 2);
			//printf("rangemin=%i,", tlv[idx]);
			idx++;
			//printf(",rangemax=%i\n", tlv[idx]);
			idx++;
			DecodeTlv(dbInfo, tlv + idx, 4 * sizeof(uint32_t));
			idx += 4;
			size -= 6 * sizeof(uint32_t);
		}
		break;
	case SND_CTL_TLVT_DB_MINMAX:
	case SND_CTL_TLVT_DB_MINMAX_MUTE:
		if (type == SND_CTL_TLVT_DB_MINMAX_MUTE) {
			//printf("dBminmaxmute-");
		}
		else {
			//printf("dBminmax-");
		}
		if (size != 2 * sizeof(uint32_t)) {
			while (size > 0) {
				//printf("0x%08x,", tlv[idx]);
				idx++;
				size -= sizeof(uint32_t);
			}
		} else {
			//printf("min=");
			//printf("%d\n", tlv[2]);
			//printf(",max=");
			//printf("%d\n", tlv[3]);
		}
		break;
	default:
		//printf("unk-%i-", type);
		while (size > 0) {
			//printf("0x%08x,", tlv[idx]);
			idx++;
			size -= sizeof(uint32_t);
		}
		break;
	}
}

int32_t GetDbValue(AUDIO_DB_INFO *dbInfo, const char *AudioDevice)
{
	int32_t err, ret = -1;
	uint32_t *tlv;

	static snd_ctl_t *handle = NULL;
	static snd_hctl_t *hctl = NULL;

	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;
	
	snd_hctl_elem_t *elem;

	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&control);

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);		/* default */
	snd_ctl_elem_id_set_name(id, AudioDevice);

	if ((err = snd_ctl_open(&handle, "default", 0)) < 0)  {
		printf("Control open error: %s\n", snd_strerror(err));
		goto ERROR;
	}

	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0) {
		printf("Cannot find the given element from control %s\n", snd_strerror(err));
		goto ERROR;
	}

	if ((err = snd_hctl_open(&hctl, "default", 0)) < 0) {
		printf("Control open error: %s\n", snd_strerror(err));
		goto ERROR;
	}
	if ((err = snd_hctl_load(hctl)) < 0) {
		printf("Control load error: %s\n", snd_strerror(err));
		goto ERROR;
	}
	elem = snd_hctl_find_elem(hctl, id);
	
	if (elem) {
		if (!snd_ctl_elem_info_is_tlv_readable(info)) {
			printf("skip tlv!!!!\n");
		}

		tlv = (unsigned int *)malloc(4096);
	
		if ((err = snd_hctl_elem_tlv_read(elem, tlv, 4096)) < 0) {
			printf("Control element TLV read error: %s\n", snd_strerror(err));
			free(tlv);
			goto ERROR;
		}

		DecodeTlv(dbInfo, tlv, 4096);
		free(tlv);
	}
	else {
		printf("Could not find the specified element\n");
	}

	ret = 0;
	//printf("dbInfo->min = %f\n", dbInfo->min);
	//printf("dbInfo->step = %f\n", dbInfo->step);

ERROR :
	if(handle)	snd_ctl_close(handle);
	if(hctl	)	snd_hctl_close(hctl);
	handle = NULL;
	hctl = NULL;
	
	return ret;
}

int32_t	AudioSetVolume( const char *AudioDevice, int32_t percent )	// volume = 0% - 100%
{
	int err, ret = -1;
	int min = 0, max = 0, stepNum = 0;
	//int value;
	
	double dbLevel, dbLevel_min, dbLevel_max;
	double dbReal, dbReal_min, dbReal_max;
	int32_t dbStep = 0;

	AUDIO_DB_INFO dbInfo;

	static snd_ctl_t *handle = NULL;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;
	
	//snd_ctl_elem_type_t type;    
	//unsigned int count;

	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&control);

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);		/* default */
	snd_ctl_elem_id_set_name(id, AudioDevice);
	
	if ((err = snd_ctl_open(&handle, "default", 0)) < 0)  {
		printf("Control open error: %s\n", snd_strerror(err));
		goto ERROR;
	}
	
	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0) {
		printf("Cannot find the given element from control %s\n", snd_strerror(err));
		goto ERROR;
	}
	
	// hard-coding
	//type = snd_ctl_elem_info_get_type(info);		// boolean(1), integer(2), enumerated(3)
	//count = snd_ctl_elem_info_get_count(info);		// number of control-element
	//printf("type = %d, count= %d\n", type, count);
	
	snd_ctl_elem_value_set_id(control, id);

	if( !snd_ctl_elem_read(handle, control) ) {
		//value = snd_ctl_elem_value_get_integer(control, 0);
        //value = snd_ctl_elem_value_get_integer(control, 1);
		//printf("current value(L) = %d, current value(R) = %d\n", value, value);
		
		min = snd_ctl_elem_info_get_min(info);
		max = snd_ctl_elem_info_get_max(info);
		stepNum = max - min;

		GetDbValue(&dbInfo, AudioDevice);
		
		// -40dB ~ 0dB
		dbLevel_min	= 20. * log10((double)1 / 100.);
		dbLevel_max = 20. * log10((double)100 / 100.);
		dbLevel		= 20. * log10((double)percent / 100.);

		// -46.5dB ~ 12dB
		dbReal_min	= dbInfo.min;
		dbReal_max	= dbInfo.step * stepNum + dbInfo.min;
		dbReal		= (dbLevel * (dbReal_min - dbReal_max)) / (dbLevel_min - dbLevel_max) + dbReal_max;

		//printf("dbLevel_min = %5.02f dB / dbLevel_max = %5.02f dB / dbLevel = %5.02f dB\n", dbLevel_min, dbLevel_max, dbLevel);
		//printf("dbReal_min = %5.02f dB / dbReal_max = %5.02f dB / dbReal = %5.02f dB\n", dbReal_min, dbReal_max, dbReal);

		//
		if( percent != 0) 
			dbStep = -(int)(((dbReal - dbReal_max)+ dbInfo.step / 2) / dbInfo.step);
		else 
			dbStep = max;
		
		//printf("[%s] dbLevel = %5.02f dB / dbReal = %5.02f dB / dbStep = %d\n", __func__, dbLevel, dbReal, dbStep);
	} 

	//printf("Change Value : %d -> %d\n", ctl_value0, (long)((min + max) * ((float)volume / 100)));
	//printf("[%s] max = %d, dbStep = %d, value = %d, stepNum = %d\n", __func__, max, dbStep, max - dbStep, stepNum);

	snd_ctl_elem_value_set_integer(control, 0, (long)( max - dbStep ) );
	snd_ctl_elem_value_set_integer(control, 1, (long)( max - dbStep ) );
		
	if ((err = snd_ctl_elem_write(handle, control)) < 0) {
		printf("Control element write error: %s\n", snd_strerror(err));
		goto ERROR;
	}
	
	ret = 0;

ERROR :
	if(handle) snd_ctl_close(handle);
	handle = NULL;
	
	return ret;
}

int32_t	AudioGetVolume( const char *AudioDevice )	// volume = 0% - 100%
{
	int err, ret = -1;
	int min = 0, max = 0, stepNum = 0;;
	int value, percent;
	
	double dbLevel, dbLevel_min, dbLevel_max;
	double dbReal, dbReal_min, dbReal_max;
	int32_t dbStep = 0;

	AUDIO_DB_INFO dbInfo;

	static snd_ctl_t *handle = NULL;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;
	
	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&control);

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);		/* default */
	snd_ctl_elem_id_set_name(id, AudioDevice);

	if ((err = snd_ctl_open(&handle, "default", 0)) < 0)  {
		printf("Control open error: %s\n", snd_strerror(err));
		goto ERROR;
	}

	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0) {
		printf("Cannot find the given element from control %s\n", snd_strerror(err));
		goto ERROR;
	}

	// hard-coding
	//type = snd_ctl_elem_info_get_type(info);		// boolean(1), integer(2), enumerated(3)
	//count = snd_ctl_elem_info_get_count(info);	// number of control-element
	//printf("type = %d, count= %d\n", type, count);

	snd_ctl_elem_value_set_id(control, id);

	if( !snd_ctl_elem_read(handle, control) ) {
		value = snd_ctl_elem_value_get_integer(control, 0);
		//value = snd_ctl_elem_value_get_integer(control, 1);
		//printf("current value(L) = %d, current value(R) = %d\n", value, value);

		min = snd_ctl_elem_info_get_min(info);
		max = snd_ctl_elem_info_get_max(info);
		stepNum = max - min;

		GetDbValue(&dbInfo, AudioDevice);
		
		// -40dB ~ 0dB
		dbLevel_min	= 20. * log10((double)1 / 100.);
		dbLevel_max = 20. * log10((double)100 / 100.);

		// -46.5dB ~ 12dB
		dbReal_min	= dbInfo.min;
		dbReal_max	= dbInfo.step * stepNum + dbInfo.min;
		
		//printf("[%s] dbReal_min = %f, dbReal_max = %f\n", __func__, dbReal_min, dbReal_max);
		dbStep		= max - value;
		dbReal		=  -(dbStep * dbInfo.step - dbReal_max); 
		dbLevel		= (dbReal - dbReal_max) * (dbLevel_min - dbLevel_max) / (dbReal_min - dbReal_max);

		if(value != 0)
			percent		= 100 * pow(10, dbLevel / 20.);
		else
			percent		= 0;
		
		//printf("[%s] dbLevel = %5.02f dB / dbReal = %5.02f dB / dbStep = %d\n", __func__, dbLevel, dbReal, dbStep);
		//printf("[%s] percent  = %d\n", __func__, percent);
	} else {
		printf("Cannot read element value!\n");
		goto ERROR;
	}

	ret = percent;

ERROR :
	if(handle) snd_ctl_close(handle);
	handle = NULL;
	
	return ret;
}

int32_t	NX_AudioSetVolume( NX_AUDIO_HANDLE hAudio, NX_AUDIO_TYPE nAudioType, int32_t percent )	// volume = 0% - 100%
{
	int32_t ret = 0;

	assert( (CNX_WaveFilePlayer*)hAudio );

	if(nAudioType == AUDIO_TYPE_PLAYBACK) {
		if( 0 > (ret = AudioSetVolume(AMIXER_CONTROL_SPK_VOLUME, percent) ))
			goto ERROR;
	}
	else {
		if( 0 > (ret = AudioSetVolume(AMIXER_CONTROL_MIC1_VOLUME, percent) ))
			goto ERROR;
		if( 0 > (ret = AudioSetVolume(AMIXER_CONTROL_MIC2_VOLUME, percent) ))
			goto ERROR;
	}

	return 0;

ERROR :
	printf("Error!\n");
	return -1;
}

int32_t NX_AudioGetVolume( NX_AUDIO_HANDLE hAudio, NX_AUDIO_TYPE nAudioType )
{
	int32_t ret = 0;

	assert( (CNX_WaveFilePlayer*)hAudio );

	if(nAudioType == AUDIO_TYPE_PLAYBACK) {
		if( 0 > (ret = AudioGetVolume(AMIXER_CONTROL_SPK_VOLUME) ))
			goto ERROR;
	}
	else {
		if( 0 > (ret = AudioGetVolume(AMIXER_CONTROL_MIC1_VOLUME) ))
			goto ERROR;
	}

	return ret;

ERROR :
	printf("Error!\n");
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Test Function..
void* PlayAsyncWaveFile( const char *fileName, int isMix, int *error )
{
	CNX_WaveFilePlayer *handle = new CNX_WaveFilePlayer();
	*error = handle->Start( fileName, isMix );
	if( *error != 0 ){
		delete handle;
		return NULL;
	}
	return (void*)handle;
}

void StopWaveFile( void *handle, int bDrop )
{
	if( handle )
	{
		((CNX_WaveFilePlayer*)handle)->Stop( bDrop );
		delete (CNX_WaveFilePlayer*)handle;
	}
}
