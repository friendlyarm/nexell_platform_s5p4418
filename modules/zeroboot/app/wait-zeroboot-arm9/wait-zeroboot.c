/**    
    @file     wait-zeroboot.c
    @date     2012/2/3
    @author   Jaekyong Oh, freefrug@falinux.com  FALinux.Co.,Ltd.
    @brief    제로부트 드라이버에 연결하여 상태를 얻어오거나 기다린다.
              Ver 0.1.0
                           
    @modify   
    @todo     
    @bug     
    @remark   
    @warning 
*/
//----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/kdev_t.h>

#include <zb_app_inf.h>

#define	VERSION_STR		"Ver 0.1.0"


//------------------------------------------------------------------------------
/** @brief   디바이스 드라이버 노드파일을 연다.
    @param   fname  디바이스 노드파일 이름
    @param   major  디바이스 드라이버 메이져번호
    @param   minor  디바이스 드라이버 마이너번호
    @return  노드파일 핸들, 0보다 작으면 에러
    @remakr  노드파일이 없으면 노드파일을 생성한 후 연다.
*///----------------------------------------------------------------------------
int dev_open( char *fname, unsigned char major, unsigned char minor )
{
	int	dev;
	
	dev = open( fname, O_RDWR|O_NDELAY );
	if (dev < 0 )
	{
		if( access( fname, F_OK ) == 0 ) 
		{
			unlink( fname );	
		}
		
		mknod( fname, (S_IRWXU|S_IRWXG|S_IFCHR), MKDEV(major,minor) );
		
		dev = open( fname, O_RDWR|O_NDELAY );
		if (dev < 0 )
		{
			printf( " Device OPEN FAIL %s\n", fname );
			return -1;
		}
	}
        
	return dev;
}

//------------------------------------------------------------------------------
/** @brief   help
    
*///----------------------------------------------------------------------------
void print_zb_status( int status )
{
	switch( status )
	{
	case ZB_STATUS_NORMALBOOT            : printf( "\t%2d:%s\n", status, "ZB_STATUS_NORMALBOOT            " ); break;
	case ZB_STATUS_SNAPSHOT_READY        : printf( "\t%2d:%s\n", status, "ZB_STATUS_SNAPSHOT_READY        " ); break;
	case ZB_STATUS_SNAPSHOT_DONE         : printf( "\t%2d:%s\n", status, "ZB_STATUS_SNAPSHOT_DONE         " ); break;
	case ZB_STATUS_ZEROBOOT              : printf( "\t%2d:%s\n", status, "ZB_STATUS_ZEROBOOT              " ); break;
	case ZB_STATUS_ZEROBOOT_RECOVERY_DONE: printf( "\t%2d:%s\n", status, "ZB_STATUS_ZEROBOOT_RECOVERY_DONE" ); break;
	default                              : printf( "\t%2d:%s\n", status, "unknown" ); break;
	}
}

//------------------------------------------------------------------------------
/** @brief   help
    
*///----------------------------------------------------------------------------
void print_help( char *exec_name )
{
	printf( " %s\n", exec_name );
	printf( " %s [wait-status]\n", exec_name );

	printf( "\twait-status\n" );
	printf( "\t  %2d:%s\n", ZB_STATUS_NORMALBOOT            , "ZB_STATUS_NORMALBOOT            " );
	printf( "\t  %2d:%s\n", ZB_STATUS_SNAPSHOT_READY        , "ZB_STATUS_SNAPSHOT_READY        " );
	printf( "\t  %2d:%s\n", ZB_STATUS_SNAPSHOT_DONE         , "ZB_STATUS_SNAPSHOT_DONE         " );
	printf( "\t  %2d:%s\n", ZB_STATUS_ZEROBOOT              , "ZB_STATUS_ZEROBOOT              " );
	printf( "\t  %2d:%s\n", ZB_STATUS_ZEROBOOT_RECOVERY_DONE, "ZB_STATUS_ZEROBOOT_RECOVERY_DONE" );
	
	printf( "\n" );
}

//------------------------------------------------------------------------------
/** @brief   main
    @remark  
*///----------------------------------------------------------------------------
int 	main( int argc, char **argv )
{       
	int  zbfd;
	int  status, wait_status;

	printf( "\n\t%s\n\n", VERSION_STR );

	zbfd = dev_open( "zeroboot-node", ZB_APP_INF_DRV_MAJOR, 0 );
	if ( 0 > zbfd ) return -1;
	
	wait_status = -1;
	if ( argc >= 2 )
	{
		if ( 0 == strncmp( "--help", argv[1], 6 ) )
		{
			print_help( argv[0] );
			return 0;	
		}
		
		wait_status	= strtoul( argv[1], NULL, 0 );
	}
/*	
	if ( argc >= 3 )
	{
		if ( 0 == strcmp( "start", argv[2] ) )
		{
			system( "echo 'start' > /proc/zeroboot_trigger" );
		}
	}		
*/
	
	if ( 0 > wait_status )
	{
		while( 1 )
		{
			// 제로부트 상태를 얻어온다.
			ioctl( zbfd, IOCTL_ZB_GET_STATUS, &status );
            print_zb_status( status );
			
			usleep( 500*1000 );
		}
	}
	else
	{
		printf( " wait.. %2d", wait_status );  fflush( stdout );
		
		//---[wait status]--------------------------------------------
		ioctl( zbfd, IOCTL_ZB_WAIT_STATUS, &wait_status );
		//------------------------------------------------------------

		return 0;
		
		// display current status
		//ioctl( zbfd, IOCTL_ZB_GET_STATUS, &status );
        //print_zb_status( status );
	}

	// 파일을 닫는다.
	close( zbfd ); 
	
	printf( "done\n" );
	return 0;
}



