#include <unistd.h>     // getopt & optarg
#include <stdio.h>      // printf
#include <string.h>     // strdup
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>   // gettimeofday
#include <nx_deinterlace.h>
#include <nx_fourcc.h>

#define	INPUT_BUFFER_NUM                 4
#define DEINTERLACE_BUFFER_NUM           4


static uint64_t NX_GetTickCount( void )
{
	uint64_t ret;
	struct timeval	tv;
	struct timezone	zv;
	gettimeofday( &tv, &zv );
	ret = ((uint64_t)tv.tv_sec)*1000000 + tv.tv_usec;
	return ret;
}

int32_t main( int32_t argc, char *argv[] )
{
	char *chIn, *chOut = NULL;
	int32_t iMode, iWidth, iHeight, opt;

	while( -1 != (opt=getopt(argc, argv, "i:o:s:m:h")))
	{
		switch( opt ) {
			case 'i' : chIn = strdup( optarg );	break;
			case 'o' : chOut = strdup( optarg );	break;
			case 's' : sscanf( optarg, "%d,%d", &iWidth, &iHeight );	break;
			case 'm' : iMode = atoi( optarg ); break;
			case 'h' : printf("-i [input file name], -o [output_file_name], -s [width],[height] -m[mode] (0:none, 1:discard, 2:mean, 3:blend, 4:bob, 5:linear)\n");		return 0;
			default : break;
		}
	}

	{
		void *hDeInterlace = NULL;
		NX_VID_MEMORY_HANDLE hInMem[INPUT_BUFFER_NUM];
		NX_VID_MEMORY_HANDLE hOutMem[DEINTERLACE_BUFFER_NUM];
		int32_t iFrmCnt = 0;
		uint64_t startTime, endTime, totalTime = 0;

		FILE *fpIn = fopen(chIn, "rb");
		FILE *fpOut = ( chOut )  ? ( fopen(chOut, "wb") ) : ( NULL );

		if ( (hDeInterlace = NX_DeInterlaceOpen( (DEINTERLACE_MODE)iMode )) == NULL )
		{
			printf("NX_DeInterlaceOpen() is failed!!!\n");
			return -1;
		}
		printf("NX_DeInterlaceOpen() is Success!!!\n");

		for( int32_t i=0 ; i<INPUT_BUFFER_NUM ; i++)
		{
			hInMem[i] = NX_VideoAllocateMemory( 4096, iWidth, iHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
			if ( 0 == hInMem[i] ) {
				printf("NX_VideoAllocMemory(64, %d, %d,...) failed. (i=%d)\n", iWidth, iHeight, i);
				return -1;

			}
		}

		for( int32_t i=0 ; i<DEINTERLACE_BUFFER_NUM ; i++ )
		{
			hOutMem[i] = NX_VideoAllocateMemory( 4096, iWidth, iHeight, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );
			if ( 0 == hOutMem[i] ) {
				printf("NX_VideoAllocMemory(64, %d, %d,...) failed. (i=%d)\n", iWidth, iHeight, i);
				return -1;

			}
		}

		while ( 1 )
		{
			int32_t iInputIdx = iFrmCnt%INPUT_BUFFER_NUM;
			if ( fread((uint8_t *)(hInMem[iInputIdx]->luVirAddr), 1, iWidth * iHeight, fpIn) <= 0 )
				break;
			if ( fread((uint8_t *)(hInMem[iInputIdx]->cbVirAddr), 1, iWidth * iHeight / 4, fpIn) <= 0 )
				break;
			if ( fread((uint8_t *)(hInMem[iInputIdx]->crVirAddr), 1, iWidth * iHeight / 4, fpIn) <= 0 )
				break;

			startTime = NX_GetTickCount();
			if ( NX_DeInterlaceFrame( hDeInterlace, hInMem[iInputIdx], 0, hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM] ) < 0 )
			{
				printf("NX_DeInterlaceFrame() failed!!!\n");
				break;
			}
			endTime = NX_GetTickCount();
			totalTime += (endTime - startTime);
			printf("%d Frame : Deinterlace Timer = %lld\n", iFrmCnt, endTime - startTime);

			if ( fpOut )
			{
				int32_t h;
				uint8_t *pbyImg = (uint8_t *)(hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->luVirAddr);
				for(h=0 ; h<hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->imgHeight ; h++)
				{
					fwrite(pbyImg, 1, hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->imgWidth, fpOut);
					pbyImg += hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->luStride;
				}

				pbyImg = (uint8_t *)(hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->cbVirAddr);
				for(h=0 ; h<hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->imgHeight/2 ; h++)
				{
					fwrite(pbyImg, 1, hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->imgWidth/2, fpOut);
					pbyImg += hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->cbStride;
				}

				pbyImg = (uint8_t *)(hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->crVirAddr);
				for(h=0 ; h<hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->imgHeight/2 ; h++)
				{
					fwrite(pbyImg, 1, hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->imgWidth/2, fpOut);
					pbyImg += hOutMem[iFrmCnt%DEINTERLACE_BUFFER_NUM]->crStride;
				}
			}

			iFrmCnt++;
		}

		printf("average timer : %lld ms \n", totalTime/iFrmCnt );

		for (int32_t i=0 ; i<INPUT_BUFFER_NUM ; i++)
			NX_FreeVideoMemory( hInMem[i] );

		for (int32_t i=0 ; i<DEINTERLACE_BUFFER_NUM ; i++)
			NX_FreeVideoMemory( hOutMem[i] );

		if ( hDeInterlace )
			NX_DeInterlaceClose( hDeInterlace );

		fclose(fpIn);
		fclose(fpOut);
	}

	return 0;
}
