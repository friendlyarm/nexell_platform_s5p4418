#include <stdio.h>
#include <unistd.h>	//	usleep

#include <nx_iso7816.h>

void test_pwm(void)
{
	uint32_t freq, duty;
	NX_PWM_HANDLE hPwm;
	hPwm = NX_PwmInit( 1 );
	if( hPwm )
	{
		NX_PwmSetFreqDuty( hPwm, 372*9600, 50 );
		NX_PwmGetInfo( hPwm, &freq, &duty );
		printf("Read Info : freq = %d, duty = %d\n", freq, duty);
	}
}


typedef struct BCAS_ATR_TYPE{
	uint8_t		TS;		//	Initial Character
	uint8_t		T_0;	//	Format Character
	//	Interface Characters : TA_1 ~ TA_4 ( 10 bytes )
	uint8_t		TA_1;
	uint8_t		TB_1;
	uint8_t		TC_1;
	uint8_t		TD_1;
	uint8_t		TA_2;
	uint8_t		TD_2;
	uint8_t		TA_3;
	uint8_t		TB_3;
	uint8_t		TD_3;
	uint8_t		TA_4;
	uint8_t		TCK;	//	Exclusive CheckSum T0 to TA4
} BCAS_ATR_TYPE;


void DumpData( void *pBuf, int32_t byteSize )
{
	int32_t i = 0;
	uint8_t *buf = (uint8_t*)pBuf;
	printf("Dump Data(%d) :", byteSize);
	for( ; i<byteSize ; i++ )
	{
		if( i%16 == 0 )
		{
			printf("\n");
		}
		if( i%4==0 )
		{
			printf(" 0x");
		}
		printf( "%02x", buf[i] );
	}
	printf("\n");
}

//
//	F Index Table
//
const int32_t FITable[16] = { 0, 372, 558,  744, 1116, 1488, 1860, -1, -1, 512, 768, 1024, 1536, 2048,   -1, -1};
//
//	Max Frequency Table 
//
const float FSMaxTable[16] = { 0., 5.,  6.,  8., 12., 16., 20., -1., -1., 5., 7.5, 10., 15., 20., -1., -1.};
//
//	D Index Table
//
const float DITable[16] = {0, 1, 2, 4, 8, 16, 0, 0, 0, 0, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64};

void DumpBCASATR( BCAS_ATR_TYPE *pAtr )
{
	uint8_t checkSum = pAtr->T_0 ^ pAtr->TA_1 ^ pAtr->TB_1 ^ pAtr->TC_1 ^ 
					  pAtr->TD_1 ^ pAtr->TA_2 ^ pAtr->TD_2 ^ pAtr->TA_3 ^ 
					  pAtr->TB_3 ^ pAtr->TD_3 ^ pAtr->TA_4;

	printf("========= ATR Dump Information ============\n");
	printf(" TS   = 0x%02x\n", pAtr->TS  );
	printf(" T_0  = 0x%02x\n", pAtr->T_0 );
	printf(" TA_1 = 0x%02x(F=%d, FsMax=%.1fMHz, D=%.2f)\n", pAtr->TA_1, FITable[pAtr->TA_1>>4], FSMaxTable[pAtr->TA_1>>4], DITable[pAtr->TA_1&0xf]);
	printf(" TB_1 = 0x%02x\n", pAtr->TB_1);
	printf(" TC_1 = 0x%02x(Guard Time=N=%d)\n", pAtr->TC_1, pAtr->TC_1);
	printf(" TD_1 = 0x%02x\n", pAtr->TD_1);
	printf(" TA_2 = 0x%02x\n", pAtr->TA_2);
	printf(" TD_2 = 0x%02x\n", pAtr->TD_2);
	printf(" TA_3 = 0x%02x(IFSI=%d, IFSC=%d)\n", pAtr->TA_3, pAtr->TA_3, pAtr->TA_3);
	printf(" TB_3 = 0x%02x(BWI=%d, CWI=%d)\n", pAtr->TB_3, pAtr->TB_3>>4, pAtr->TB_3&0xF);
	printf(" TD_3 = 0x%02x\n", pAtr->TD_3);
	printf(" TA_4 = 0x%02x\n", pAtr->TA_4);
	printf(" TCK  = 0x%02x(Exclusive OR T_0 ~ TA_4)\n", pAtr->TCK );
	printf(" Calculated checksum(T_0 ~ TA_4) = 0x%02x\n", checkSum );
	printf(" etu = (1/D) * (F/f) = %f usec\n", (1./DITable[pAtr->TA_1&0xf]) * ((double)FITable[pAtr->TA_1>>4]/(372.*9600.)) );
	printf("===========================================\n");
}


//
//		B-CAS Card Protocol
//	 ----------------- -------------------- -------------
//	|  Initail Field  | Information Field  |  End Field  |
//	|    (3 Bytes)    |     (n Bytes)      |  (1 Byte)   |
//	 ----------------- -------------------- -------------
//
//
//	Initial Field
//	NAD(1Byte), PCB(1Byte), LEN(1Byte)
//
//	Intial Field
//	NAD (1Byte) : Node Address
//	PCB (1Byte) : Protocol Control Byte ( n )
//	LEN (1Byte) : Length
//
//	Information Field
//	0 <= Information Field Length(n) <= IFCS or IFSD
//
//	End Field
//	EDC 1(Byte) : Error Detection Code (Longitudinal Redundancy Check)
//


//
//	Check B-CAS Card Command
//
uint8_t CheckComand( uint8_t *buf, int32_t size )
{
	uint8_t lrc = 0;
	while( --size > 0 )
	{
		lrc = (lrc ^ *buf++) & 0xff;
	}
	return *buf == lrc;
}

//
//	Supported Data Frequency :
//		9600, 19200, 38400, 57600,
//		115200 ,230400 ,460800 ,500000 ,
//		576000 ,921600 ,1000000, 1152000,
//		1500000, 2000000, 2500000, 3000000,
//		3500000, 4000000,
//
void Test_CheckBCASCard()
{
	uint8_t sciBuf[256];
	int32_t readByte, written;
	NX_ISO7816_HANDLE hSCI = NX_InitISO7816();
	if( hSCI == NULL )
	{
		printf( "Error : NX_InitISO7816() is Failed!!!\n");
		return;
	}
	usleep(100000);

	NX_SetResetISO7816( hSCI, 1 );

	usleep(100000);

	readByte = NX_ReadISO7816( hSCI, sciBuf, sizeof(sciBuf), 500 );
	DumpData( sciBuf, readByte );

	DumpBCASATR( (BCAS_ATR_TYPE*)sciBuf );

	//	Set Data Frequency to 19200
	NX_SetDataFreqIOS7816( hSCI, 19200 );

	usleep(10000);

	sciBuf[0] = 0;
	sciBuf[1] = 0x00;
	sciBuf[2] = 0x05;
	sciBuf[3] = 0x90;
	sciBuf[4] = 0x30;
	sciBuf[5] = 0;
	sciBuf[6] = 0;
	sciBuf[7] = 0;
	sciBuf[8] = 0xa5;

	written = NX_WriteISO7816( hSCI, sciBuf, 9 );
	readByte = NX_ReadISO7816( hSCI, sciBuf, 9, 100 );
	DumpData( sciBuf, readByte );
	readByte = NX_ReadISO7816( hSCI, sciBuf, sizeof(sciBuf), 1000 );
	DumpData( sciBuf, readByte );

	if( readByte > 3 )
	{
		printf("Command Check = %d\n", CheckComand(sciBuf, readByte ));
	}

	printf("written=%d, readByte = %d\n", written, readByte);

	NX_DeinitISO7816( hSCI );
}


int main(int argc, char *argv[])
{
	Test_CheckBCASCard();
	return 0;
}
