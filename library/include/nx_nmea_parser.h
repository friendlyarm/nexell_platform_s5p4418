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

#ifndef __NX_NMEA_PARSER_H__
#define __NX_NMEA_PARSER_H__

#define NMEA_MAX_CMD_LEN		8		/* maximum command length (NMEA address) */
#define NMEA_MAX_DATA_LEN		256		/* max data length */
#define NMEA_MAX_CHAN			36		/* max channels */
#define NMEA_WP_ID_LEN			32		/* waypoint max string len */
#define NMEA_MAX_FIELD			25		/* max field length */

/* $GPGGA: Global Positioning System Fix Data */
struct	nmea_gpgga {
	unsigned char	hour;
	unsigned char 	minute;
	unsigned char 	second;
	double 			latitude;
	double 			longitude;
	unsigned char 	quality;
	unsigned char 	num_sats;
	double 			hdop;
	double 			altitude;
	unsigned int 	count;
};

/* $GPGSA: GPS DOP and active satellites */
struct	nmea_gpgsa {
	unsigned char	mode1;
	unsigned char	mode2;
	unsigned short	sat_on_ch[NMEA_MAX_CHAN];
	double 			pdop;
	double 			hdop;
	double 			vdop;
	unsigned int 	count;
};

/* $GPGSV: GPS Satellites in view */
struct	gsv_sat {
	unsigned short	prn;
	unsigned short	snr;
	int				used_in_ch;
	unsigned short	azimuth;
	unsigned short	elevation;
};

struct	nmea_gpgsv {
	unsigned char 	num_msgs;
	unsigned short	num_satv;
	struct gsv_sat	sats[NMEA_MAX_CHAN];
	unsigned int 	count;
};

/* $GPRMB: Recommended minimum navigation information */
struct	nmea_gprmb {
	unsigned char 	dat_status;
	double 			crosstrack_err;
	unsigned char 	direction_steer;
	char 			orig_waypoint[NMEA_WP_ID_LEN];
	char 			dest_waypoint[NMEA_WP_ID_LEN];
	double 			dest_latitude;
	double 			dest_longitude;
	double 			range_dest;
	double 			bearing_dest;
	double 			close_velocity;
	unsigned char 	arrival_status;
	unsigned int 	count;
};

/* $GPRMC: Recommended minimum specific GPS/TRANSIT data */
struct	nmea_gprmc {
	unsigned char 	hour;
	unsigned char 	minute;
	unsigned char 	second;
	unsigned char 	dat_valid;
	double 			latitude;
	double 			longitude;
	double 			ground_speed;
	double 			course;
	unsigned char 	day;
	unsigned char 	month;
	unsigned short 	year;
	double 			mag_var;
	unsigned int	count;
};

/* $GPZDA : UTC Date / Time and Local Time Zone Offset */
struct	nmea_gpzda {
	unsigned char 	hour;
	unsigned char 	minute;
	unsigned char 	second;
	unsigned char 	day;
	unsigned char 	month;
	unsigned short 	year;
	unsigned char 	lc_hour;
	unsigned char 	lc_minute;
	unsigned int 	count;
};

#ifdef __cplusplus
extern "C" {
#endif

int		NX_NmeaInit		( void );		/* return : -1 = fail create handle, else handle index */
void	NX_NmeaExit		( int id );		/* return : none */

int 	NX_NmeaParser	( int id, char *buff, int len );
void 	NX_NmeaReset	( int id);

int		NX_NmeaType		( int id, char *name, void *data, int len );

int 	NX_NmeaGpgga	( int id, struct nmea_gpgga *gga );
int 	NX_NmeaGpgsa	( int id, struct nmea_gpgsa *gsa );
int 	NX_NmeaGpgsv	( int id, struct nmea_gpgsv *gsv );
int 	NX_NmeaGprmb	( int id, struct nmea_gprmb *rmb );
int 	NX_NmeaGprmc	( int id, struct nmea_gprmc *rmc );
int 	NX_NmeaGpzda	( int id, struct nmea_gpzda *zda );

#ifdef __cplusplus
}
#endif


#endif /* __NX_NMEA_PARSER_H__ */
