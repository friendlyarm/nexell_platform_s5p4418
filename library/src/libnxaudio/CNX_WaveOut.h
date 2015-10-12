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

#ifndef __CNX_WAVEOUT_H__
#define	__CNX_WAVEOUT_H__

#include <alsa/asoundlib.h>
#include <pthread.h>

//////////////////////////////////////////////////////////////////////////////
//
//					Nexell Alsa WaveOut Module
//
class CNX_WaveOut{
public:
	CNX_WaveOut();
	virtual ~CNX_WaveOut();

public:
	int		WaveOutOpen( unsigned int sampleFreq, int channeles, unsigned int numBuf, unsigned int sizeBuf, int useMixer );
	int		WaveOutPlayBuffer( unsigned char *pBuf, int size, int &error );
	int		WaveOutReset();							//	Stop Play & Reset Buffer
	void	WaveOutClose(int Drop);
	void	WaveOutFlush();
	void	WaveOutPrepare();
	long	WaveOutGetAvailBufSize();
	unsigned int GetPeriodBufferSize();

	void	WriteDummy( unsigned int time );		//	Milli-Seconds
	long long FindDuration( unsigned int Size );	//	duration
	long long GetBufferedTime();
private:
//	unsigned char			m_Dummy[1024*4];
	snd_pcm_t*				m_hPlayback;
	snd_pcm_hw_params_t*	m_pHWParam;
	snd_pcm_status_t*		m_pStatus;
	unsigned int			m_Periods;
	unsigned int			m_PeriodBufSize;
	unsigned int			m_SamplingFreq;
	float					m_TimePerSample;
	int						m_Channels;
	snd_pcm_uframes_t		m_MaxAvailBuffer;

	//
	int						m_bPlaying;
	int						m_bStopPlay;
	FILE					*m_hFile;
};
//
//
//////////////////////////////////////////////////////////////////////////////

#endif	//	__CNX_WAVEOUT_H__
