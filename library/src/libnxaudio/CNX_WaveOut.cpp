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

#include "CNX_WaveOut.h"

#ifdef DEFAULT_SAMPLE_FREQ
#undef DEFAULT_SAMPLE_FREQ
#endif

#define	DEFAULT_SAMPLE_FREQ		48000		//	48KHz
#define	TIME_RESOLUTION			90000		//	90KHz

//////////////////////////////////////////////////////////////////////////////
//
//					Nexell Win32 WaveOut Module
//
CNX_WaveOut::CNX_WaveOut()
	: m_hPlayback( NULL )
	, m_pHWParam( NULL )
	, m_MaxAvailBuffer( 0 )
{
	m_SamplingFreq = DEFAULT_SAMPLE_FREQ;
	m_TimePerSample = ((double)TIME_RESOLUTION)/((double)DEFAULT_SAMPLE_FREQ);
}

CNX_WaveOut::~CNX_WaveOut()
{
	WaveOutClose( 0 );
}

int CNX_WaveOut::WaveOutOpen( unsigned int sampleFreq,
							 int channels,
							 unsigned int numBuf,
							 unsigned int sizeBuf,
							 int useMixer )
{
	int Ret = 0;
	m_PeriodBufSize	= sizeBuf;
	m_Periods		= numBuf;
	m_SamplingFreq	= sampleFreq;
	m_Channels		= channels;

	if( 0 != m_SamplingFreq ){
		m_TimePerSample = ((double)TIME_RESOLUTION)/((double)m_SamplingFreq);
	}
#if 1	//to do Mixer 
	if( useMixer ){
		Ret=snd_pcm_open(&m_hPlayback, "plug:dmix", SND_PCM_STREAM_PLAYBACK, 0);
	}else{
		Ret=snd_pcm_open(&m_hPlayback, "default", SND_PCM_STREAM_PLAYBACK, 0);
	}
#else
	Ret=snd_pcm_open(&m_hPlayback, "default", SND_PCM_STREAM_PLAYBACK, 0);
#endif

	if( Ret < 0 ){
                printf( "snd_pcm_open failed!!!(%d)\n", Ret );
		m_hPlayback=  NULL;
		return -1;
	}
	if( 0 != (Ret=snd_pcm_hw_params_malloc( &m_pHWParam )) )
	{
		printf( "snd_pcm_hw_params_malloc failed!!!(%d)\n", Ret );
	}
	snd_pcm_hw_params_any( m_hPlayback, m_pHWParam );

	if( snd_pcm_hw_params_set_access(m_hPlayback, m_pHWParam, SND_PCM_ACCESS_RW_INTERLEAVED) < 0 ){
		printf( "snd_pcm_hw_params_set_access failed!!!\n" );
	}

	//	PCM Signed Little Endian
	if( snd_pcm_hw_params_set_format ( m_hPlayback, m_pHWParam, SND_PCM_FORMAT_S16_LE ) < 0 ){
		printf( "snd_pcm_hw_params_set_format failed!!!\n" );
	}
	//	Set Channels
	if( snd_pcm_hw_params_set_channels( m_hPlayback, m_pHWParam, channels ) < 0 ){
		printf( "snd_pcm_hw_params_set_format failed!!!\n" );
	}

	//	Sampling Rate
	Ret = snd_pcm_hw_params_set_rate_near(m_hPlayback, m_pHWParam, &m_SamplingFreq, 0);				//	Set Sampling Freq
    if( Ret < 0 )
    	printf("snd_pcm_hw_params_set_rate_near failed!!!\n");
    if( sampleFreq != m_SamplingFreq )
    	printf( "snd_pcm_hw_params_set_rate_near(%d,%d)!!!\n", Ret, m_SamplingFreq );

#if 1
	//	Set Periods
	if( snd_pcm_hw_params_set_periods_near( m_hPlayback, m_pHWParam, &m_Periods, 0 ) < 0 )
	{
                printf( "snd_pcm_hw_params_set_periods_near failed!!!\n" );
	}else{
		snd_pcm_hw_params_set_periods( m_hPlayback, m_pHWParam, m_Periods, 0 );
	}
#endif

	//	Set period buffer size
	if( snd_pcm_hw_params_set_period_size_near( m_hPlayback, m_pHWParam, (snd_pcm_uframes_t*)&m_PeriodBufSize, 0 ) < 0 )
	{
                printf( "snd_pcm_hw_params_set_period_size_near failed!!!\n" );
	}else{
		snd_pcm_hw_params_set_period_size( m_hPlayback, m_pHWParam, m_PeriodBufSize, 0 );
	}
#if 0
	//	Set Periods
	if( snd_pcm_hw_params_set_periods_near( m_hPlayback, m_pHWParam, &m_Periods, 0 ) < 0 )
	{
                printf( "snd_pcm_hw_params_set_periods_near failed!!!\n" );
	}else{
		snd_pcm_hw_params_set_periods( m_hPlayback, m_pHWParam, m_Periods, 0 );
	}
#endif

	m_MaxAvailBuffer = m_PeriodBufSize * m_Periods;

	if( snd_pcm_hw_params_set_buffer_size_near(m_hPlayback, m_pHWParam, &m_MaxAvailBuffer) < 0 ){
                printf( "snd_pcm_hw_params_set_buffer_size failed!!!\n" );
	}
	if( snd_pcm_hw_params( m_hPlayback, m_pHWParam ) < 0 ){
                printf( "snd_pcm_hw_params failed!!!\n" );
	}
	if( snd_pcm_hw_params_get_buffer_size(m_pHWParam, &m_MaxAvailBuffer) < 0 ){
                printf( "snd_pcm_hw_params_get_buffer_size failed!!!\n" );
	}

	printf( ">>>>>  PeriodBufSize(%d), Periods(%d), Channel(%d), SamplingRate(%d), Max(%d), Mixer(%d) <<<<<\n",
	m_PeriodBufSize, m_Periods, m_Channels, m_SamplingFreq, (unsigned int)m_MaxAvailBuffer, useMixer );

	if ((Ret = snd_pcm_prepare(m_hPlayback)) < 0) {
                printf( "Alsa : Prepare error: %s\n", snd_strerror(Ret) );
	}

	if( Ret < 0 ){
		//	Close Player Driver
		snd_pcm_close(m_hPlayback);
		m_hPlayback = NULL;
	}

	return Ret;
}

int CNX_WaveOut::WaveOutPlayBuffer( unsigned char *pBuf, int Size, int &Error )
{
	int Written;
	if( NULL == m_hPlayback )		return -1;

	Error = snd_pcm_avail(m_hPlayback );
	if( Error < 0 ){
                printf( "===>>> pcm avail error(%d) <<<===\n", Error );
		snd_pcm_recover( m_hPlayback, Error, 1 );
	}

#if 0	//	old routine
	Written = snd_pcm_writei(m_hPlayback, pBuf, Size>>m_Channels);
	if( Written < 0 )
	{
		snd_pcm_recover( m_hPlayback, Written, 1 );
                printf("ALSA Error(%d)\n", Written );
		return -1;
	}
#else
	//printf("++ Size = %d\n", Size);
	Size >>= m_Channels;
	//printf("-- Size = %d\n", Size);
	while( Size > 0 )
	{
		Written = snd_pcm_writei( m_hPlayback, pBuf, Size );
		if( Written < 0 )
		{
			printf("pcm write error(%d)\n", Written );
			snd_pcm_recover( m_hPlayback, Written, 1 );
		}else if( Written <= Size ){
			Size -= Written;
			pBuf += (Written*m_Channels*2);
		}
	}
#endif
	return 0;
}

void CNX_WaveOut::WriteDummy( unsigned int Time )
{
	//int Err;
	//unsigned int Mad, Remain, DummyBufSize, SampleSize;

	//if( NULL==m_hPlayback || 0==Time )	return;
	//SampleSize = (Time * m_SamplingFreq) / TIME_RESOLUTION;
	//if( SampleSize  == 0 ) return;

	//DummyBufSize = sizeof(m_Dummy)>>m_Channels;
	//Mad = SampleSize/DummyBufSize;
	//Remain = SampleSize%DummyBufSize;

	//for( unsigned int i=0; i<Mad ; i++ )
	//{
	//	if( (Err = snd_pcm_writei( m_hPlayback, m_Dummy, DummyBufSize )) < 0 ){
	//		snd_pcm_recover( m_hPlayback, Err, 1 );
	//		return;
	//	}
	//}
	//if( Remain )
	//{
	//	if( (Err = snd_pcm_writei( m_hPlayback, m_Dummy, Remain )) < 0 ){
	//		snd_pcm_recover( m_hPlayback, Err, 1 );
	//		return;
	//	}
	//}
}

long long CNX_WaveOut::GetBufferedTime()
{
	snd_pcm_sframes_t avail;
	if( NULL==m_hPlayback )		return 0;

	if( (avail = snd_pcm_avail( m_hPlayback )) <= 0 ){
		avail = 0;
	}
	return (long long)( (m_MaxAvailBuffer - avail) * m_TimePerSample );
}

long long CNX_WaveOut::FindDuration( unsigned int Size )
{
	if( NULL==m_hPlayback || 0==m_Channels )		return 0;
	//	TODO : 
	Size = Size / 2 / m_Channels;
	return (long long)( ((float)Size) * m_TimePerSample );
}

int CNX_WaveOut::WaveOutReset()
{
	if( !m_hPlayback )		return -1;
	return 0;
}

void CNX_WaveOut::WaveOutClose( int bDrop )
{
	if( m_hPlayback )
	{
		if( bDrop )
			snd_pcm_drop(m_hPlayback);		//	stop playing & drop pending frames
		else
			snd_pcm_drain(m_hPlayback);		//	stop playing & drop pending frames

		snd_pcm_prepare( m_hPlayback );
		snd_pcm_close(m_hPlayback);
		m_hPlayback = NULL;
	}

	if( m_pHWParam )
	{
		snd_pcm_hw_params_free (m_pHWParam);
		//	Dealloc
		m_pHWParam = NULL;
	}
	return ;
}

void CNX_WaveOut::WaveOutFlush()
{
	if( m_hPlayback ){
//		snd_pcm_nonblock( m_hPlayback, 0 );
//		snd_pcm_drain( m_hPlayback );
//		snd_pcm_nonblock( m_hPlayback, 0 );

		snd_pcm_drop( m_hPlayback );
		snd_pcm_prepare( m_hPlayback );
	}
}

long CNX_WaveOut::WaveOutGetAvailBufSize()
{
	snd_pcm_sframes_t avail;
	if( NULL==m_hPlayback )		return 0;
	if( (avail = snd_pcm_avail( m_hPlayback )) <= 0 ){
		avail = 0;
	}
	return avail;
}

unsigned int CNX_WaveOut::GetPeriodBufferSize()
{
	return m_PeriodBufSize;
}

