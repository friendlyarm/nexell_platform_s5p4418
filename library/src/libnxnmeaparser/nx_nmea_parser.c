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
#include <string.h>
#include <stdlib.h>

#include "nx_nmea_parser.h"

#if (0)
#define DBGOUT(msg...)		{ printf(msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#define ERROUT(msg...)		{ 						\
		printf("ERROR: %s, %s line %d: \n",			\
			__FILE__, __FUNCTION__, __LINE__),		\
		printf(msg); }

/*------------------------------------------------------------------------------
 * NMEA define
 *----------------------------------------------------------------------------*/
enum NMEA_STATE {
	NMEA_STATE_START 		= 0,
	NMEA_STATE_CMD			= 1,
	NMEA_STATE_DATA			= 2,
	NMEA_STATE_CHECKSUM_1   = 3,
	NMEA_STATE_CHECKSUM_2   = 4,
};

struct nmea_data {
	char			    cmd [NMEA_MAX_CMD_LEN];
	char			    data[NMEA_MAX_DATA_LEN];
	unsigned char		calu_csum;				/* calculate check sum */
	unsigned char		recv_csum;				/* received  check sum */
	int					index;
	int					count;
	int					status;

	/* nmea data: GPGGA, GPGSA, GPGSV, ... */
	void			*	nmea_data[8];
	int					nmea_num;

	struct nmea_gpgga	gga;		/* $GPGGA */
	struct nmea_gpgsa	gsa;		/* $GPGSA */
	struct nmea_gpgsv	gsv;		/* $GPGSV */
	struct nmea_gprmb	rmb;		/* $GPRMB */
	struct nmea_gprmc	rmc;		/* $GPRMC */
	struct nmea_gpzda	zda;		/* $GPZDA */
};

struct nmea_desc {
	int					index;
	struct nmea_data  * data;
};

#define	NMEA_MAX_HND	32
struct nmea_desc 	  * g_np_desc[NMEA_MAX_HND] = { NULL, };

/*------------------------------------------------------------------------------
 * NMEA local
 *----------------------------------------------------------------------------*/
static void parse_gpgga(int id, char *data);
static void parse_gpgsa(int id, char *data);
static void parse_gpgsv(int id, char *data);
static void parse_gprmb(int id, char *data);
static void parse_gprmc(int id, char *data);
static void parse_gpzda(int id, char *data);

static int parse_cmd(int id, char *cmd, char *data)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;

	DBGOUT("%s, id=%d, cmd=%s, data=%s\n", __func__, id, cmd, data);

	if (0 == strcmp(cmd, "GPGGA"))
		parse_gpgga(id, data);

	else if (0 == strcmp(cmd, "GPGSA"))
		parse_gpgsa(id, data);

	else if (0 == strcmp(cmd, "GPGSV"))
		parse_gpgsv(id, data);

	else if (0 == strcmp(cmd, "GPRMB"))
		parse_gprmb(id, data);

	else if (0 == strcmp(cmd, "GPRMC"))
		parse_gprmc(id, data);

	else if (0 == strcmp(cmd, "GPZDA"))
		parse_gpzda(id, data);

	npd->count++;
	return 0;
}

static int parse_field(char *data, char *field, int num, int max)
{
	int i = 0, n = 0, end = 0;

	if (! data || ! field || 0 >= max)
		return 0;

	/* Go to the beginning of the selected field */
	while (end != num && data[i]) {
		if (data[i] == ',')
			end++;

		i++; /* add */

		if (! data[i]) {
			field[0] = '\0';
			return 0;
		}
	}

	if (data[i] == ',' || data[i] == '*') {
		field[0] = '\0';
		return 0;
	}

	/* copy field from data to Field */
	while (data[i] != ',' && data[i] != '*' && data[i]) {
		field[n] = data[i];
		n++; i++;

		/* check if field is too big to fit on passed parameter. If it is,
		   crop returned field to its max length. */
		if (n >= max) {
			n = max-1;
			break;
		}
	}

	field[n] = '\0';
	return 1;
}

static int parse_sat_stat(int id, unsigned short sat)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	int i;

	if (! npd || ! sat)
		return 0;

	for (i = 0; NMEA_MAX_CHAN > i ; i++) {
		if (sat == npd->gsa.sat_on_ch[i])
			return 1;
	}
	return 1;
}

static void parse_nmea(int id, char data)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	if (! npd) {
		ERROUT("not initialized ...\n");
		return;
	}

	DBGOUT("%s, status=%d, data=%c\n", __func__, npd->status, data);
	switch (npd->status) {
		/* Search for start of message '$' */
		case NMEA_STATE_START :
			if (data == '$') {
				npd->calu_csum = 0;
				npd->index     = 0;
				npd->status    = NMEA_STATE_CMD;	/* Next */
			}
			break;

		/* Retrieve command (NMEA Address) */
		case NMEA_STATE_CMD :
			if (data != ',' && data != '*') {
				npd->cmd[npd->index++] = data;	/* command */
				npd->calu_csum ^= data;

				/* Check overflow */
				if (npd->index >= NMEA_MAX_CMD_LEN)
					npd->status = NMEA_STATE_START;
			} else {
				npd->cmd[npd->index] = '\0';		/* end command */
				npd->index      = 0;
				npd->status     = NMEA_STATE_DATA;	/* next */
				npd->calu_csum ^= data;
			}
			break;

		/* Store data and check for end of sentence or checksum flag */
		case NMEA_STATE_DATA :
			/* checksum flag? */
			if (data == '*') {
				npd->data[npd->index] = '\0';
				npd->status = NMEA_STATE_CHECKSUM_1;

			/* no checksum flag, store data */
			} else {
				/* Check for end of sentence with no checksum */
				if (data == '\r') {
					npd->data[npd->index] = '\0';
					parse_cmd(id, npd->cmd, npd->data);
					npd->status = NMEA_STATE_START;
					return;
				}

				/* Store data and calculate checksum */
				npd->calu_csum 	  	 ^= data;
				npd->data[npd->index] = data;

				/* Check overflow */
				if (++npd->index >= NMEA_MAX_DATA_LEN)
					npd->status = NMEA_STATE_START;
			}
			break;

		case NMEA_STATE_CHECKSUM_1 :
			if (9 >= (data - '0'))
				npd->recv_csum = (data - '0') << 4;
			else
				npd->recv_csum = (data - 'A' + 10) << 4;

			npd->status = NMEA_STATE_CHECKSUM_2;
			break;

		case NMEA_STATE_CHECKSUM_2 :
			if (9 >= (data - '0'))
				npd->recv_csum |= (data - '0');
			else
				npd->recv_csum |= (data - 'A' + 10);

			if (npd->calu_csum == npd->recv_csum)
				parse_cmd(id, npd->cmd, npd->data);

			npd->status = NMEA_STATE_START;
			break;

		default :
			npd->status = NMEA_STATE_START;
			break;
	}
}

static void parse_reset(int id)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	int i;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return;
	}

	/* GPGGA Data */
	npd->gga.hour 		= 0;
	npd->gga.minute 	= 0;
	npd->gga.second 	= 0;
	npd->gga.latitude 	= 0.0;
	npd->gga.longitude 	= 0.0;
	npd->gga.quality 	= 0;
	npd->gga.num_sats 	= 0;
	npd->gga.hdop 		= 0.0;
	npd->gga.altitude 	= 0.0;
	npd->gga.count 		= 0;

	/* GPGSA */
	npd->gsa.mode1 = 'M';
	npd->gsa.mode2 = 1;
	for (i = 0; i < NMEA_MAX_CHAN; i++)
		npd->gsa.sat_on_ch[i] = 0;

	npd->gsa.pdop  = 0.0;
	npd->gsa.hdop  = 0.0;
	npd->gsa.vdop  = 0.0;
	npd->gsa.count = 0;

	/* GPGSV */
	npd->gsv.num_msgs = 0;
	npd->gsv.num_satv = 0;
	for (i = 0; i < NMEA_MAX_CHAN; i++) {
		npd->gsv.sats[i].azimuth 	= 0;
		npd->gsv.sats[i].elevation 	= 0;
		npd->gsv.sats[i].prn 		= 0;
		npd->gsv.sats[i].snr 		= 0;
		npd->gsv.sats[i].used_in_ch = 0;
	}
	npd->gsv.count = 0;

	/* GPRMB */
	npd->rmb.dat_status 		= 'V';
	npd->rmb.crosstrack_err  	= 0.0;
	npd->rmb.direction_steer  	= '?';
	npd->rmb.orig_waypoint[0] 	= '\0';
	npd->rmb.dest_waypoint[0] 	= '\0';
	npd->rmb.dest_latitude 	 	= 0.0;
	npd->rmb.dest_longitude 	= 0.0;
	npd->rmb.range_dest	 	 	= 0.0;
	npd->rmb.bearing_dest 	 	= 0.0;
	npd->rmb.close_velocity 	= 0.0;
	npd->rmb.arrival_status   	= 'V';
	npd->rmb.count 		     	= 0;

	/* GPRMC */
	npd->rmc.hour 			= 0;
	npd->rmc.minute 		= 0;
	npd->rmc.second 		= 0;
	npd->rmc.dat_valid 		= 'V';
	npd->rmc.latitude 		= 0.0;
	npd->rmc.longitude 		= 0.0;
	npd->rmc.ground_speed	= 0.0;
	npd->rmc.course 		= 0.0;
	npd->rmc.day 			= 1;
	npd->rmc.month 			= 1;
	npd->rmc.year 			= 2000;
	npd->rmc.mag_var 		= 0.0;
	npd->rmc.count			= 0;

	/* GPZDA */
	npd->zda.hour 		= 0;
	npd->zda.minute 	= 0;
	npd->zda.second 	= 0;
	npd->zda.day 		= 1;
	npd->zda.month 		= 1;
	npd->zda.year 		= 2000;
	npd->zda.lc_hour 	= 0;
	npd->zda.lc_minute 	= 0;
	npd->zda.count 		= 0;
}

/*------------------------------------------------------------------------------
 * $GPGGA: Global Positioning System Fix Data
 -----------------------------------------------------------------------------*/
static void parse_gpgga(int id, char *data)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	char   field[NMEA_MAX_FIELD];
	char   buff [10];

	if (! npd) {
		ERROUT("not initialized ...\n");
		return;
	}
	DBGOUT("%s, id=%d, data=%s\n", __func__, id, data);

	/* Time */
	if (parse_field(data, field, 0, NMEA_MAX_FIELD)) {
		// Hour
		buff[0] = field[0];
		buff[1] = field[1];
		buff[2] = '\0';
		npd->gga.hour = atoi(buff);

		// minute
		buff[0] = field[2];
		buff[1] = field[3];
		buff[2] = '\0';
		npd->gga.minute = atoi(buff);

		// Second
		buff[0] = field[4];
		buff[1] = field[5];
		buff[2] = '\0';
		npd->gga.second = atoi(buff);
	}

	/* Latitude */
	if (parse_field(data, field, 1, NMEA_MAX_FIELD)) {
		npd->gga.latitude = atof(field+2) / 60.0;
		field[2] = '\0';
		npd->gga.latitude += atof(field);

	}

	if (parse_field(data, field, 2, NMEA_MAX_FIELD)) {
		if (field[0] == 'S')
			npd->gga.latitude = -npd->gga.latitude;
	}

	/* Longitude */
	if (parse_field(data, field, 3, NMEA_MAX_FIELD)) {
		npd->gga.longitude = atof(field+3) / 60.0;
		field[3] = '\0';
		npd->gga.longitude += atof(field);
	}

	if (parse_field(data, field, 4, NMEA_MAX_FIELD)) {
		if (field[0] == 'W')
			npd->gga.longitude = -npd->gga.longitude;
	}

	/* GPS quality */
	if (parse_field(data, field, 5, NMEA_MAX_FIELD))
		npd->gga.quality = field[0] - '0';

	/* Satellites in use */
	if (parse_field(data, field, 6, NMEA_MAX_FIELD)) {
		buff[0] = field[0];
		buff[1] = field[1];
		buff[2] = '\0';
		npd->gga.num_sats = atoi(buff);
	}

	/* HDOP */
	if (parse_field(data, field, 7, NMEA_MAX_FIELD))
		npd->gga.hdop = atof(field);

	/* Altitude */
	if (parse_field(data, field, 8, NMEA_MAX_FIELD))
		npd->gga.altitude = atof(field);

	npd->gga.count++;
}

static void parse_gpgsa(int id, char *data)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	char   field[NMEA_MAX_FIELD];
	char   buff [10];
	int    i;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return;
	}
	DBGOUT("%s, id=%d, data=%s\n", __func__, id, data);

	/* Mode 1 */
	if (parse_field(data, field, 0, NMEA_MAX_FIELD))
		npd->gsa.mode1 = field[0];

	/* Mode 2 */
	if (parse_field(data, field, 1, NMEA_MAX_FIELD))
		npd->gsa.mode2 = field[0] - '0';

	/* Active satellites */
	for (i = 0; i < 12; i++) {
		if (parse_field(data, field, 2 + i, NMEA_MAX_FIELD)) {
			buff[0] = field[0];
			buff[1] = field[1];
			buff[2] = '\0';
			npd->gsa.sat_on_ch[i] = atoi(buff);
		} else {
			npd->gsa.sat_on_ch[i] = 0;
		}
	}

	/* PDOP */
	if (parse_field(data, field, 14, NMEA_MAX_FIELD))
		npd->gsa.pdop = atof(field);
	else
		npd->gsa.pdop = 0.0;

	/* HDOP */
	if (parse_field(data, field, 15, NMEA_MAX_FIELD))
		npd->gsa.hdop = atof(field);
	else
		npd->gsa.hdop = 0.0;

	/* VDOP */
	if (parse_field(data, field, 16, NMEA_MAX_FIELD))
		npd->gsa.vdop = atof(field);
	else
		npd->gsa.vdop = 0.0;

	npd->gsa.count++;
}

static void parse_gpgsv(int id, char *data)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	char   field[NMEA_MAX_FIELD];
	int    total_msg = 0, msgnum = 0, i;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return;
	}
	DBGOUT("%s, id=%d, data=%s\n", __func__, id, data);

	/* Total number of messages */
	if (parse_field(data, field, 0, NMEA_MAX_FIELD)) {
		total_msg = atoi(field);

		// Make sure that the total_msg is valid. This is used to
		// calculate indexes into an array. I've seen corrept NMEA strings
		// with no checksum set this to large values.
		if (total_msg > 9 || total_msg < 0)
			return;
	}

	if (total_msg < 1 || total_msg*4 >= NMEA_MAX_CHAN)
		return;

	/* message number */
	if (parse_field(data, field, 1, NMEA_MAX_FIELD)) {
		msgnum = atoi(field);

		// Make sure that the message number is valid. This is used to
		// calculate indexes into an array
		if (msgnum > 9 || msgnum < 0) return;
	}

	/* Total satellites in view */
	if (parse_field(data, field, 2, NMEA_MAX_FIELD))
		npd->gsv.num_satv = atoi(field);

	/* Satelite data */
	for (i = 0; 4 > i; i++) {
		/* Satellite ID */
		if (parse_field(data, field, 3 + 4*i, NMEA_MAX_FIELD))
			npd->gsv.sats[i+(msgnum-1)*4].prn = atoi(field);
		else
			npd->gsv.sats[i+(msgnum-1)*4].prn = 0;

		/* Elevarion */
		if (parse_field(data, field, 4 + 4*i, NMEA_MAX_FIELD))
			npd->gsv.sats[i+(msgnum-1)*4].elevation = atoi(field);
		else
			npd->gsv.sats[i+(msgnum-1)*4].elevation = 0;

		/* Azimuth */
		if (parse_field(data, field, 5 + 4*i, NMEA_MAX_FIELD))
			npd->gsv.sats[i+(msgnum-1)*4].azimuth = atoi(field);
		else
			npd->gsv.sats[i+(msgnum-1)*4].azimuth = 0;

		/* SNR */
		if (parse_field(data, field, 6 + 4*i, NMEA_MAX_FIELD))
			npd->gsv.sats[i+(msgnum-1)*4].snr = atoi(field);
		else
			npd->gsv.sats[i+(msgnum-1)*4].snr = 0;

		// Update "used in solution" (used_in_ch) flag. This is base
		// on the GSA message and is an added convenience for post processing
		npd->gsv.sats[i+(msgnum-1)*4].used_in_ch =
				parse_sat_stat(id, npd->gsv.sats[i+(msgnum-1)*4].prn);
	}

	npd->gsv.count++;
}

static void parse_gprmb(int id, char *data)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	char   field[NMEA_MAX_FIELD];

	if (! npd) {
		ERROUT("not initialized ...\n");
		return;
	}
	DBGOUT("%s, id=%d, data=%s\n", __func__, id, data);

	/* Data status */
	if (parse_field(data, field, 0, NMEA_MAX_FIELD))
		npd->rmb.dat_status = field[0];
	else
		npd->rmb.dat_status = 'V';

	/* Cross track error */
	if (parse_field(data, field, 1, NMEA_MAX_FIELD))
		npd->rmb.crosstrack_err = atof(field);
	else
		npd->rmb.crosstrack_err = 0.0;

	/* Direction to steer */
	if (parse_field(data, field, 2, NMEA_MAX_FIELD))
		npd->rmb.direction_steer = field[0];
	else
		npd->rmb.direction_steer = '?';

	/* Orgin waypoint ID */
	if (parse_field(data, field, 3, NMEA_MAX_FIELD))
		strcpy(npd->rmb.orig_waypoint, field);
	else
		npd->rmb.orig_waypoint[0] = '\0';

	/* Destination waypoint ID */
	if (parse_field(data, field, 4, NMEA_MAX_FIELD))
		strcpy(npd->rmb.dest_waypoint, field);
	else
		npd->rmb.dest_waypoint[0] = '\0';

	/* Destination latitude */
	if (parse_field(data, field, 5, NMEA_MAX_FIELD)) {
		npd->rmb.dest_latitude = atof(field+2) / 60.0;
		field[2] = '\0';
		npd->rmb.dest_latitude += atof(field);
	}

	if (parse_field(data, field, 6, NMEA_MAX_FIELD)) {
		if (field[0] == 'S')
			npd->rmb.dest_latitude = -npd->rmb.dest_latitude;
	}

	/* Destination Longitude */
	if (parse_field(data, field, 7, NMEA_MAX_FIELD)) {
		npd->rmb.dest_longitude = atof(field+3) / 60.0;
		field[3] = '\0';
		npd->rmb.dest_longitude += atof(field);
	}

	if (parse_field(data, field, 8, NMEA_MAX_FIELD)) {
		if (field[0] == 'W')
			npd->rmb.dest_longitude = -npd->rmb.dest_longitude;
	}

	/* Range to destination nautical mi */
	if (parse_field(data, field, 9, NMEA_MAX_FIELD))
		npd->rmb.range_dest = atof(field);
	else
		npd->rmb.crosstrack_err = 0.0;

	/* Bearing to destination degrees true */
	if (parse_field(data, field, 10, NMEA_MAX_FIELD))
		npd->rmb.bearing_dest = atof(field);
	else
		npd->rmb.bearing_dest = 0.0;

	/* Closing velocity */
	if (parse_field(data, field, 11, NMEA_MAX_FIELD))
		npd->rmb.close_velocity = atof(field);
	else
		npd->rmb.close_velocity = 0.0;

	/* Arrival status */
	if (parse_field(data, field, 12, NMEA_MAX_FIELD))
		npd->rmb.arrival_status = field[0];
	else
		npd->rmb.close_velocity = 'V';

	npd->rmb.count++;

}

static void parse_gprmc(int id, char *data)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	char   field[NMEA_MAX_FIELD];
	char   buff [10];

	if (! npd) {
		ERROUT("not initialized ...\n");
		return;
	}
	DBGOUT("%s, id=%d, data=%s\n", __func__, id, data);

	/* Time */
	if (parse_field(data, field, 0, NMEA_MAX_FIELD)) {
		/* Hour */
		buff[0] = field[0];
		buff[1] = field[1];
		buff[2] = '\0';
		npd->rmc.hour = atoi(buff);

		/* minute */
		buff[0] = field[2];
		buff[1] = field[3];
		buff[2] = '\0';
		npd->rmc.minute = atoi(buff);

		/* Second */
		buff[0] = field[4];
		buff[1] = field[5];
		buff[2] = '\0';
		npd->rmc.second = atoi(buff);
	}

	/* Data valid */
	if (parse_field(data, field, 1, NMEA_MAX_FIELD))
		npd->rmc.dat_valid = field[0];
	else
		npd->rmc.dat_valid = 'V';

	/* latitude */
	if (parse_field(data, field, 2, NMEA_MAX_FIELD)) {
		npd->rmc.latitude = atof(field+2) / 60.0;
		field[2] = '\0';
		npd->rmc.latitude += atof(field);
	}

	if (parse_field(data, field, 3, NMEA_MAX_FIELD)) {
		if (field[0] == 'S')
			npd->rmc.latitude = -npd->rmc.latitude;
	}

	/* Longitude */
	if (parse_field(data, field, 4, NMEA_MAX_FIELD)) {
		npd->rmc.longitude = atof(field+3) / 60.0;
		field[3] = '\0';
		npd->rmc.longitude += atof(field);
	}

	if (parse_field(data, field, 5, NMEA_MAX_FIELD)) {
		if (field[0] == 'W')
			npd->rmc.longitude = -npd->rmc.longitude;
	}

	/* Ground speed */
	if (parse_field(data, field, 6, NMEA_MAX_FIELD))
		npd->rmc.ground_speed = atof(field);
	else
		npd->rmc.ground_speed = 0.0;

	/* course over ground, degrees true */
	if (parse_field(data, field, 7, NMEA_MAX_FIELD))
		npd->rmc.course = atof(field);
	else
		npd->rmc.course = 0.0;

	/* date */
	if (parse_field(data, field, 8, NMEA_MAX_FIELD)) {
		/* Day */
		buff[0] = field[0];
		buff[1] = field[1];
		buff[2] = '\0';
		npd->rmc.day = atoi(buff);

		/* Month */
		buff[0] = field[2];
		buff[1] = field[3];
		buff[2] = '\0';
		npd->rmc.month = atoi(buff);

		/* Year (Only two digits. I wonder why?) */
		buff[0] = field[4];
		buff[1] = field[5];
		buff[2] = '\0';
		npd->rmc.year = atoi(buff);
		npd->rmc.year += 2000;	/* make 4 digit date -- What assumptions should be made here? */
	}

	/* course over ground, degrees true */
	if (parse_field(data, field, 9, NMEA_MAX_FIELD))
		npd->rmc.mag_var = atof(field);
	else
		npd->rmc.mag_var = 0.0;

	if (parse_field(data, field, 10, NMEA_MAX_FIELD)) {
		if (field[0] == 'W')
			npd->rmc.mag_var = -npd->rmc.mag_var;
	}

	npd->rmc.count++;
}

static void parse_gpzda(int id, char *data)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;
	char   field[NMEA_MAX_FIELD];
	char   buff [10];

	if (! npd) {
		ERROUT("not initialized ...\n");
		return;
	}
	DBGOUT("%s, id=%d, data=%s\n", __func__, id, data);

	/* Time */
	if (parse_field(data, field, 0, NMEA_MAX_FIELD)) {
		/* Hour */
		buff[0] = field[0];
		buff[1] = field[1];
		buff[2] = '\0';
		npd->zda.hour = atoi(buff);

		/* minute */
		buff[0] = field[2];
		buff[1] = field[3];
		buff[2] = '\0';
		npd->zda.minute = atoi(buff);

		/* Second */
		buff[0] = field[4];
		buff[1] = field[5];
		buff[2] = '\0';
		npd->zda.second = atoi(buff);
	}

	/* Day */
	if (parse_field(data, field, 1, NMEA_MAX_FIELD))
		npd->zda.day = atoi(field);
	else
		npd->zda.day = 1;

	/* Month */
	if (parse_field(data, field, 2, NMEA_MAX_FIELD))
		npd->zda.month = atoi(field);
	else
		npd->zda.month = 1;

	/* Year */
	if (parse_field(data, field, 3, NMEA_MAX_FIELD))
		npd->zda.year = atoi(field);
	else
		npd->zda.year = 1;

	/* Local zone hour */
	if (parse_field(data, field, 4, NMEA_MAX_FIELD))
		npd->zda.lc_hour = atoi(field);
	else
		npd->zda.lc_hour = 0;

	/* Local zone hour */
	if (parse_field(data, field, 5, NMEA_MAX_FIELD))
		npd->zda.lc_minute = atoi(field);
	else
		npd->zda.lc_minute = 0;

	npd->zda.count++;
}


/*------------------------------------------------------------------------------
 * NMEA interface
 *----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 * 	Description	: create nmea handle index
 *	Return		: -1 = fail create handle, else handle index
 */
int		NX_NmeaInit(void)
{
	struct nmea_desc *desc = NULL;
	struct nmea_data *data = NULL;
	int i, id = -1;
	DBGOUT("%s\n", __func__);

	/* get empty handle */
	for (i = 0; NMEA_MAX_HND > i; i++) {
		if (NULL == g_np_desc[i])
			break;
	}

	if (NMEA_MAX_HND == i)
		return -1;

	desc = (struct nmea_desc *)malloc(sizeof(struct nmea_desc));
	if (! desc)
		return -1;

	data = (struct nmea_data *)malloc(sizeof(struct nmea_data));
	if (! data) {
		free(desc);
		return -1;
	}
	desc->data = data;

	/* clear */
	memset(data, 0 , sizeof(struct nmea_data));

	/* set head */
	g_np_desc[i] = desc;
	g_np_desc[i]->index = (i+1);
	DBGOUT("done %s, id=%d\n", __func__, g_np_desc[i]->index);

	/* reset */
	id = g_np_desc[i]->index;
	parse_reset(id);

	return id;
}

/*------------------------------------------------------------------------------
 * 	Description	: delete nmea handle index
 *	In[id]		: nmea handle index
 *	Return		: none
 */
void	NX_NmeaExit(int id)
{
	struct nmea_desc *desc = NULL;
	struct nmea_data *data = NULL;

	DBGOUT("%s, id=%d\n", __func__, id);
	if (0 >= id || id >= NMEA_MAX_HND)
		return;

	desc = g_np_desc[id-1];
	if (desc) {
		data = desc->data;
		if (data)
			free(data);
		free(desc);
	}

	g_np_desc[id-1]->index = 0;
	g_np_desc[id-1]->data  = NULL;
}

/*------------------------------------------------------------------------------
 * 	Description	: parse data to inner nmea data struct
 *
 *	In[id]		: nmea handle index
 *	In[buff]	: input nmea data
 *	In[len]		: nmea data length
 *
 *	Return		: -1 = fail parsing, 0 = OK
 */
int 	NX_NmeaParser(int id, char *buff, int len)
{
	int i;
	if (0 >= id || id >= NMEA_MAX_HND)
		return -1;

	for (i = 0; len > i; i++)
		parse_nmea(id, buff[i]);

	return 0;
}

/*------------------------------------------------------------------------------
 * 	Description	: crear nmea data struct
 *	In[id]		: nmea handle index
 *	Return		: -1 = fail parsing, 0 = OK
 */
void 	NX_NmeaReset(int id)
{
	parse_reset(id);
}

/*------------------------------------------------------------------------------
 * 	Description	: get nmea data
 *
 *	In[id]		: nmea handle index
 *	In[name]	: nmea name, ex> GPGGA, GPGSA, GPGSV, GPRMB, GPRMC, GPZDA
 *	In[buff]	: user buffer to copy nmea data
 *	In[len]		: user buffer length
 *
 *	Return		: -1 = fail parsing, 0 = OK
 */
int		NX_NmeaType (int id, char *name, void *data, int len)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return -1;
	}

	if (0 == strcmp(name, "GPGGA"))
		memcpy(data, &npd->gga, len);

	else if (0 == strcmp(name, "GPGSA"))
		memcpy(data, &npd->gsa, len);

	else if (0 == strcmp(name, "GPGSV"))
		memcpy(data, &npd->gsv, len);

	else if (0 == strcmp(name, "GPRMB"))
		memcpy(data, &npd->rmb, len);

	else if (0 == strcmp(name, "GPRMC"))
		memcpy(data, &npd->rmc, len);

	else if (0 == strcmp(name, "GPZDA"))
		memcpy(data, &npd->zda, len);

	return 0;
}

/*------------------------------------------------------------------------------
 * 	Description	: get GPGGA
 *	In[id]		: nmea handle index
 *	In[gga]		:  *
 *	Return		: -1 = fail parsing, 0 = OK
 */
int 	NX_NmeaGpgga(int id, struct nmea_gpgga *gga)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return -1;
	}

	memcpy(gga, &npd->gga, sizeof(struct nmea_gpgga));
	return 0;
}

/*------------------------------------------------------------------------------
 * 	Description	: get GPGSA
 *	In[id]		: nmea handle index
 *	In[gga]		:  *
 *	Return		: -1 = fail parsing, 0 = OK
 */
int 	NX_NmeaGpgsa(int id, struct nmea_gpgsa *gsa)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return -1;
	}

	memcpy(gsa, &npd->gsa, sizeof(struct nmea_gpgsa));
	return 0;
}

/*------------------------------------------------------------------------------
 * 	Description	: get GPGSV
 *	In[id]		: nmea handle index
 *	In[gga]		:  *
 *	Return		: -1 = fail parsing, 0 = OK
 */
int 	NX_NmeaGpgsv(int id, struct nmea_gpgsv *gsv)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return -1;
	}

	memcpy(gsv, &npd->gsv, sizeof(struct nmea_gpgsv));
	return 0;
}

/*------------------------------------------------------------------------------
 * 	Description	: get GPRMB
 *	In[id]		: nmea handle index
 *	In[gga]		:  *
 *	Return		: -1 = fail parsing, 0 = OK
 */
int 	NX_NmeaGprmb(int id, struct nmea_gprmb *rmb)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return -1;
	}

	memcpy(rmb, &npd->rmb, sizeof(struct nmea_gprmb));
	return 0;
}

/*------------------------------------------------------------------------------
 * 	Description	: get GPRMC
 *	In[id]		: nmea handle index
 *	In[gga]		:  *
 *	Return		: -1 = fail parsing, 0 = OK
 */
int 	NX_NmeaGprmc(int id, struct nmea_gprmc *rmc)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return -1;
	}

	memcpy(rmc, &npd->rmc, sizeof(struct nmea_gprmc));
	return 0;
}

/*------------------------------------------------------------------------------
 * 	Description	: get GPZDA
 *	In[id]		: nmea handle index
 *	In[gga]		:  *
 *	Return		: -1 = fail parsing, 0 = OK
 */
int 	NX_NmeaGpzda(int id, struct nmea_gpzda *zda)
{
	struct nmea_desc *desc = g_np_desc[id-1];
	struct nmea_data *npd  = desc->data;

	if (! npd) {
		ERROUT("not initialized ...\n");
		return -1;
	}

	memcpy(zda, &npd->zda, sizeof(struct nmea_gpzda));
	return 0;
}
