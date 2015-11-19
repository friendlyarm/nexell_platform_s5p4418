//-------------------------------------------------------------------
// Copyright SAMSUNG Electronics Co., Ltd
// All right reserved.
//
// This software is the confidential and proprietary information
// of Samsung Electronics, Inc. ("Confidential Information").  You
// shall not disclose such Confidential Information and shall use
// it only in accordance with the terms of the license agreement
// you entered into with Samsung Electronics.
//-------------------------------------------------------------------
/**
 * @file  libcec.c
 * @brief This file contains an implementation of CEC library.
 *
 * @author  Digital IP Development Team (js13.park@samsung.com) \n
 *          SystemLSI, Samsung Electronics
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

#include "libcec.h"

#ifdef ANDROID
#define LOG_TAG "libcec"
#include <utils/Log.h>
#endif

#ifndef _IO
#define	TYPE_SHIFT		8
#define _IO(type,nr)	((type<<TYPE_SHIFT) | nr)
#define _IOC_TYPE(nr)	((nr>>TYPE_SHIFT) & 0xFF)
#define _IOC_NR(nr)		(nr & 0xFF)
#endif

#define	IOC_NX_MAGIC	0x6e78	/* "nx" */

enum {
    IOCTL_HDMI_CEC_SETLADDR    =  _IOW(IOC_NX_MAGIC, 1, unsigned int),
    IOCTL_HDMI_CEC_GETPADDR    =  _IOR(IOC_NX_MAGIC, 2, unsigned int),
};

#define CEC_DEBUG 1

#ifndef ANDROID
#define ALOGD(x...)     printf(x); printf("\n")
#define ALOGE(x...)     printf(x); printf("\n")
#endif

/**
 * @def CEC_DEVICE_NAME
 * Defines simbolic name of the CEC device.
 */
#define CEC_DEVICE_NAME     "/dev/hdmi-cec"

static struct {
    enum CECDeviceType devtype;
    unsigned char laddr;
} laddresses[] = {
    { CEC_DEVICE_RECODER, 1  },
    { CEC_DEVICE_RECODER, 2  },
    { CEC_DEVICE_TUNER,   3  },
    { CEC_DEVICE_PLAYER,  4  },
    { CEC_DEVICE_AUDIO,   5  },
    { CEC_DEVICE_TUNER,   6  },
    { CEC_DEVICE_TUNER,   7  },
    { CEC_DEVICE_PLAYER,  8  },
    { CEC_DEVICE_RECODER, 9  },
    { CEC_DEVICE_TUNER,   10 },
    { CEC_DEVICE_PLAYER,  11 },
};

static int CECSetLogicalAddr(unsigned int laddr);

#if CEC_DEBUG
inline static void CECPrintFrame(unsigned char *buffer, unsigned int size);
#endif

static int fd = -1;


/**
 * Open device driver and assign CEC file descriptor.
 *
 * @return  If success to assign CEC file descriptor, return 1; otherwise, return 0.
 */
int CECOpen()
{
    int res = 1;

    if (fd != -1)
        CECClose();

    ALOGD("opening %s...\n", CEC_DEVICE_NAME);
    if ((fd = open(CEC_DEVICE_NAME, O_RDWR)) < 0) {
        ALOGE("Can't open %s!\n", CEC_DEVICE_NAME);
        res = 0;
    }

    return res;
}

/**
 * Close CEC file descriptor.
 *
 * @return  If success to close CEC file descriptor, return 1; otherwise, return 0.
 */
int CECClose()
{
    int res = 1;

    if (fd != -1) {
        ALOGD("closing...\n");
        if (close(fd) != 0) {
            ALOGE("close() failed!\n");
            res = 0;
        }
        fd = -1;
    }

    return res;
}

/**
 * Allocate logical address.
 *
 * @param paddr   [in] CEC device physical address.
 * @param devtype [in] CEC device type.
 *
 * @return new logical address, or 0 if an arror occured.
 */
int CECAllocLogicalAddress(int paddr, enum CECDeviceType devtype)
{
    unsigned char laddr = CEC_LADDR_UNREGISTERED;
    unsigned int i = 0;

    ALOGD("%s 1\n", __func__);

    if (fd == -1) {
        ALOGE("open device first!");
        return 0;
    }

    ALOGD("%s 2\n", __func__);
    if (CECSetLogicalAddr(laddr) < 0) {
        ALOGE("CECSetLogicalAddr() failed!\n");
        return 0;
    }

    ALOGD("%s 3\n", __func__);
    if (paddr == CEC_NOT_VALID_PHYSICAL_ADDRESS) {
        return CEC_LADDR_UNREGISTERED;
    }

    ALOGD("%s 4\n", __func__);
    /* send "Polling Message" */
    while (i < sizeof(laddresses)/sizeof(laddresses[0])) {
        ALOGD("%s: i %d\n", __func__, i);
        if (laddresses[i].devtype == devtype) {
            unsigned char _laddr = laddresses[i].laddr;
            unsigned char message = ((_laddr << 4) | _laddr);
            if (CECSendMessage(&message, 1) != 1) {
                laddr = _laddr;
                break;
            }
        }
        i++;
    }
    ALOGD("%s 5\n", __func__);

    if (laddr == CEC_LADDR_UNREGISTERED) {
        ALOGD("All LA addresses in use!!!\n");
        return CEC_LADDR_UNREGISTERED;
    }
    ALOGD("%s 6\n", __func__);

    if (CECSetLogicalAddr(laddr) < 0) {
        ALOGD("CECSetLogicalAddr() failed!\n");
        return 0;
    }

    ALOGD("Logical address = %d\n", laddr);

    /* broadcast "Report Physical Address" */
    unsigned char buffer[5];
    buffer[0] = (laddr << 4) | CEC_MSG_BROADCAST;
    buffer[1] = CEC_OPCODE_REPORT_PHYSICAL_ADDRESS;
    buffer[2] = (paddr >> 8) & 0xFF;
    buffer[3] = paddr & 0xFF;
    buffer[4] = devtype;

    if (CECSendMessage(buffer, 5) != 5) {
        ALOGD("CECSendMessage() failed!\n");
        return 0;
    }
    ALOGD("%s 7\n", __func__);

    return laddr;
}

/**
 * Send CEC message.
 *
 * @param *buffer   [in] pointer to buffer address where message located.
 * @param size      [in] message size.
 *
 * @return number of bytes written, or 0 if an arror occured.
 */
int CECSendMessage(unsigned char *buffer, int size)
{
    if (fd == -1) {
        ALOGD("open device first!\n");
        return 0;
    }

    if (size > CEC_MAX_FRAME_SIZE) {
        ALOGD("size should not exceed %d\n", CEC_MAX_FRAME_SIZE);
        return 0;
    }

#if CEC_DEBUG
    ALOGD("CECSendMessage() : ");
    CECPrintFrame(buffer, size);
#endif

    return write(fd, buffer, size);
}

/**
 * Receive CEC message.
 *
 * @param *buffer   [in] pointer to buffer address where message will be stored.
 * @param size      [in] buffer size.
 * @param timeout   [in] timeout in microseconds.
 *
 * @return number of bytes received, or 0 if an arror occured.
 */
int CECReceiveMessage(unsigned char *buffer, int size, long timeout)
{
    int bytes = 0;
    fd_set rfds;
    struct timeval tv;
    int retval;

    if (fd == -1) {
        ALOGD("open device first!\n");
        return 0;
    }

    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    retval = select(fd + 1, &rfds, NULL, NULL, &tv);

    if (retval == -1) {
        return 0;
    } else if (retval) {
        bytes = read(fd, buffer, size);
#if CEC_DEBUG
        ALOGD("CECReceiveMessage() : ");
        CECPrintFrame(buffer, bytes);
#endif
    }

    return bytes;
}

/**
 * Set CEC logical address.
 *
 * @return 1 if success, otherwise, return 0.
 */
int CECSetLogicalAddr(unsigned int laddr)
{
    if (ioctl(fd, IOCTL_HDMI_CEC_SETLADDR, &laddr)) {
        ALOGE("ioctl(CEC_SETLADDR) failed!\n");
        return 0;
    }

    return 1;
}

int CECGetTargetCECPhysicalAddress(unsigned int *paddr)
{
    if (ioctl(fd, IOCTL_HDMI_CEC_GETPADDR, paddr)) {
        ALOGD("ioctl(CEC_GETPADDR) failed!\n");
        return 0;
    }

    return 1;
}


#if CEC_DEBUG
/**
 * Print CEC frame.
 */
void CECPrintFrame(unsigned char *buffer, unsigned int size)
{
    if (size > 0) {
        unsigned int i;
        ALOGD("fsize: %d ", size);
        ALOGD("frame: ");
        for (i = 0; i < size; i++) {
            ALOGD("0x%02x ", buffer[i]);
        }
        ALOGD("\n");
    }
}
#endif

/**
 * Check CEC message.
 *
 * @param opcode   [in] command opcode.
 * @param lsrc     [in] device logical address (source).
 *
 * @return 1 if message should be ignored, otherwise, return 0.
 */
//TODO: not finished
int CECIgnoreMessage(unsigned char opcode, unsigned char lsrc)
{
    int retval = 0;

    /* if a message coming from address 15 (unregistered) */
    if (lsrc == CEC_LADDR_UNREGISTERED) {
        switch (opcode) {
            case CEC_OPCODE_DECK_CONTROL:
            case CEC_OPCODE_PLAY:
                retval = 1;
                break;
            default:
                break;
        }
    }

    return retval;
}

/**
 * Check CEC message.
 *
 * @param opcode   [in] command opcode.
 * @param size     [in] message size.
 *
 * @return 0 if message should be ignored, otherwise, return 1.
 */
//TODO: not finished
int CECCheckMessageSize(unsigned char opcode, int size)
{
    int retval = 1;

    switch (opcode) {
        case CEC_OPCODE_PLAY:
        case CEC_OPCODE_DECK_CONTROL:
            if (size != 3) retval = 0;
            break;
        case CEC_OPCODE_SET_MENU_LANGUAGE:
            if (size != 5) retval = 0;
            break;
        case CEC_OPCODE_FEATURE_ABORT:
            if (size != 4) retval = 0;
            break;
        case CEC_OPCODE_REPORT_POWER_STATUS:
            if (size != 3) retval = 0;
            break;
        default:
            break;
    }

    return retval;
}

/**
 * Check CEC message.
 *
 * @param opcode    [in] command opcode.
 * @param broadcast [in] broadcast/direct message.
 *
 * @return 0 if message should be ignored, otherwise, return 1.
 */
//TODO: not finished
int CECCheckMessageMode(unsigned char opcode, int broadcast)
{
    int retval = 1;

    switch (opcode) {
//TODO: check
//        case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
        case CEC_OPCODE_SET_MENU_LANGUAGE:
            if (!broadcast) retval = 0;
            break;
        case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
        case CEC_OPCODE_DECK_CONTROL:
        case CEC_OPCODE_PLAY:
        case CEC_OPCODE_FEATURE_ABORT:
        case CEC_OPCODE_ABORT:
        case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
        case CEC_OPCODE_ACTIVE_SOURCE:
        case CEC_OPCODE_REPORT_POWER_STATUS:
        case CEC_OPCODE_INITIATE_ARC:
        case CEC_OPCODE_REQUEST_ARC_INITIATION:
        case CEC_OPCODE_REQUEST_ARC_TERMINATION:
        case CEC_OPCODE_TERMINATE_ARC:
        case CEC_OPCODE_CDC_MESSAGE:
            if (broadcast) retval = 0;
            break;
        default:
            break;
    }

    return retval;
}
