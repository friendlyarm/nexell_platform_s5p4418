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

/*////////////////////////////////////////////////////////////////////////////
//
//						Wave File Fomat
//
The canonical WAVE format starts with the RIFF header:

0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                               (0x52494646 big-endian form).
4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                               4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                               This is the size of the rest of the chunk 
                               following this number.  This is the size of the 
                               entire file in bytes minus 8 bytes for the
                               two fields not included in this count:
                               ChunkID and ChunkSize.
8         4   Format           Contains the letters "WAVE"
                               (0x57415645 big-endian form).

The "WAVE" format consists of two subchunks: "fmt " and "data":
The "fmt " subchunk describes the sound data's format:

12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some 
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
          2   ExtraParamSize   if PCM, then doesn't exist
          X   ExtraParams      space for extra parameters

The "data" subchunk contains the size of the data and the actual sound:

36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this 
                               number.
44        *   Data             The actual sound data.
//
//
*/////////////////////////////////////////////////////////////////////////////

#include "CNX_WaveOutApi.h"

int CNX_WaveFilePlayer::m_bInitMutex = 0;
pthread_mutex_t CNX_WaveFilePlayer::m_Mutex = PTHREAD_MUTEX_INITIALIZER;

CNX_WaveFilePlayer::CNX_WaveFilePlayer()
	: m_pWaveOut( NULL )
	, m_hFile ( NULL )
	, m_ThreadId( -1 )
	, m_WaveDataPos( 0 )
	, m_WaveDataSize( 0 )
{
	if( m_bInitMutex == 0 )
	{
		//	Initialize Mutex
		m_bInitMutex = 1;
		pthread_mutex_init( &m_Mutex, NULL );
	}
}

CNX_WaveFilePlayer::~CNX_WaveFilePlayer()
{
	if( m_pWaveOut ){
		delete m_pWaveOut;
	}
	if( m_hFile ){
		fclose( m_hFile );
	}
}

void *CNX_WaveFilePlayer::ThreadStub( void *param )
{
	CNX_WaveFilePlayer *pObj = (CNX_WaveFilePlayer *)param;
	if( pObj ){
		pObj->ThreadProc();
	}
	return (void*)0xDEADDEAD;
}

void CNX_WaveFilePlayer::ThreadProc()
{
	unsigned char * buf;
	int readSize, error;
	int readUnit = m_pWaveOut->GetPeriodBufferSize() * 2 * m_WaveFormat.numChannels * 4;
	int totalSize = m_WaveDataSize;
	buf = new unsigned char[readUnit];
	fseek( m_hFile, m_WaveDataPos, SEEK_SET );
	m_bStopPlay = 0;
	m_bRunning = 1;

	do{
		memset( buf, 0, readUnit );

		if( readUnit > totalSize )
			readSize = totalSize;
		else
			readSize = readUnit;

		readSize = fread(buf, 1, readSize, m_hFile );

		if( 0 >= readSize )
			break;
		m_pWaveOut->WaveOutPlayBuffer( buf, readSize, error );

		totalSize -= readSize;
	} while(!m_bStopPlay);

	m_bRunning = 0;

	delete []buf;
}

//	Wave Parser
int CNX_WaveFilePlayer::WaveFileParser( FILE *fd )
{
	int fileSize, readSize, i, bfound;
	//int addtionalInfo;
	WaveFileHeader header;
	WaveFormat format;
	ChunkInfo chunk;
	//int dataPos=0, dataSize=0;

	fseek( fd, 0, SEEK_END );
	fileSize = ftell(fd);
	fseek( fd, 0, SEEK_SET );

	if( fileSize < WAVEFILE_MIN_HEADER_SIZE )
	{
		return -1;
	}

	//	Check RIFF Chunk
	readSize = fread( &header, 1, WAVEFILE_HEADER_SIZE, fd );
	if( readSize != WAVEFILE_HEADER_SIZE )
		return -1;
	if( (header.chunkId != 0x46464952) || (header.format != 0x45564157) )	//	Check 'RIFF' && 'WAVE'
		return -1;

	bfound = 0;
	for( i=0 ; i<10 ; i++ )
	{
		readSize = fread( &chunk, 1, 8, fd );

		if( readSize != 8 )
		{
			break;
		}

		switch (chunk.chunkId)
		{
		case 0x20746d66:	//	" fmt", wave file fomrat chunk
			bfound++;
			readSize = fread( &m_WaveFormat, 1, sizeof(format), fd );
			if( chunk.chunkSize > (unsigned int)readSize )
			{
				fseek( fd, chunk.chunkSize-readSize, SEEK_CUR );
			}
			//	Check Audio Format
			if( m_WaveFormat.audioFormat != 1 )
			{
				//	Bad format
				return -1;
			}
			break;
		case 0x61746164:	//	"data", wave file data chunk
			bfound ++;
			m_WaveDataPos = ftell( fd );
			m_WaveDataSize = chunk.chunkSize;
			fseek( fd, chunk.chunkSize, SEEK_CUR );
			break;
		default:
			fseek( fd, chunk.chunkSize, SEEK_CUR );
			break;
		}
		if( bfound > 1 )
			break;
	}

	if( bfound < 2 || m_WaveDataSize == 0 )
		return -1;

	fseek( fd, m_WaveDataSize, SEEK_SET );

	return m_WaveDataSize;
}

int CNX_WaveFilePlayer::Start( const char *fileName, int useMix )
{
	pthread_mutex_lock( &m_Mutex );

	//	Open File
	m_hFile = fopen( fileName, "rb" );
	if( m_hFile == NULL ){
		pthread_mutex_unlock( &m_Mutex );
		return -2;	//	cannot open file
	}

	if( WaveFileParser( m_hFile ) < 0 ){
		pthread_mutex_unlock( &m_Mutex );
		return -3;	//	invalid wave file
	}

	m_pWaveOut =  new CNX_WaveOut();
	if( m_pWaveOut->WaveOutOpen( m_WaveFormat.samplingRate, m_WaveFormat.numChannels, 32, 1024*2, useMix ) < 0 ){
		delete m_pWaveOut;
		m_pWaveOut = NULL;
		pthread_mutex_unlock( &m_Mutex );
		return -4;	//	wave device open failed
	}

	m_ThreadId = pthread_create( &m_hThread, NULL, ThreadStub, this );

	if( m_ThreadId != 0 ){
		pthread_mutex_unlock( &m_Mutex );
		return -5;	//	thread create failed
	}

	pthread_mutex_unlock( &m_Mutex );
	return 0;
}

int CNX_WaveFilePlayer::Stop( int bDrop )
{
	m_bStopPlay = 1;
	if( m_ThreadId == 0 ){
		pthread_join( m_hThread, NULL );
		m_ThreadId = -1;
		if( m_pWaveOut ) 
			m_pWaveOut->WaveOutClose(bDrop);
	}
	return 0;
}

