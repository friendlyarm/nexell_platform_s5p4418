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

#include <pthread.h>
#include <CNX_WaveOut.h>

#define	WAVEFILE_HEADER_SIZE		12
#define	WAVE_FORMAT_CHUNK_SIZE		24
#define	WAVE_DATA_CHUMK_MIN_SIZE	8
#define	WAVEFILE_MIN_HEADER_SIZE	(WAVEFILE_HEADER_SIZE + WAVE_FORMAT_CHUNK_SIZE + WAVE_DATA_CHUMK_MIN_SIZE)			//	44

typedef struct WaveFileHeader{
	unsigned int	chunkId;
	unsigned int	chunkSize;
	unsigned int	format;
}WaveFileHeader;


typedef struct ChunkInfo{
	unsigned int	chunkId;
	unsigned int	chunkSize;			//	Read PCM Audio Sample Size
}ChunkInfo;


//	" fmt" chunk base data info
typedef struct WaveFormat{
	unsigned short	audioFormat;
	unsigned short	numChannels;
	unsigned int	samplingRate;
	unsigned int	byteRate;
	unsigned short	blockAlign;
	unsigned short	bitsPerSample;
}WaveFormat;


class CNX_WaveFilePlayer {
public:
	CNX_WaveFilePlayer();
	~CNX_WaveFilePlayer();
	int Start( const char *fileName, int useMix );
	int Stop( int bDrop );

	int						m_bRunning;

private :
	static pthread_mutex_t	m_Mutex;
	static int				m_bInitMutex;
	CNX_WaveOut				*m_pWaveOut;
	FILE					*m_hFile;
	pthread_t				m_hThread;
	int						m_ThreadId;
	WaveFormat				m_WaveFormat;
	int						m_WaveDataPos;
	int						m_WaveDataSize;
	int						m_bStopPlay;

	//	Player Thread
	static void *ThreadStub( void *param );
	void	ThreadProc();

	//	Wave file parser
	int		WaveFileParser( FILE *fd );
};