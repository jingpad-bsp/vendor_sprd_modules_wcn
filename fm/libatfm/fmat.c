#ifndef _FM_AT
#define _FM_AT

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>

#include <utils/Log.h>
#include "fmat.h"

#include "sprd_fts_type.h"
#include "sprd_fts_log.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG 	"FMAT"

#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#define FM_DEV_NAME "/dev/fm"
#define AUDCTL "/dev/pipe/mmi.audio.ctrl"
#define AUDCTL_NEW "/data/local/media/mmi.audio.ctrl"

/*  seek direction */
#define FM_SEEK_UP 1
#define FM_SEEK_DOWN 0
#define FM_RSSI_MIN 105

#define FM_CMD_STATE 0x00  //open/close fm
#define FM_CMD_VOLUME 0x01  //set FM volume
#define FM_CMD_MUTE   0x02   // set FM mute
#define FM_CMD_TUNE  0x03   //get a single frequency information
#define FM_CMD_SEEK   0x04  //
#define FM_CMD_READ_REG  0x05
#define FM_CMD_WRITE_REG  0x06
#define FM_SUCCESS 0
#define FM_FAILURE -1
#define FM_OPEN 1
#define FM_CLOSE 0

#define FM_SEEK_SPACE 1
// auto HiLo
#define FM_AUTO_HILO_OFF    0
#define FM_AUTO_HILO_ON     1

enum fmStatus {
	FM_STATE_DISABLED,
	FM_STATE_ENABLED,
	FM_STATE_PLAYING,
	FM_STATE_STOPED,
	FM_STATE_PANIC,
	FM_STATE_ERR,
};

int fd = -1;
struct fm_seek_parm rx_parm;

struct fm_tune_parm {
	uchar err;
	uchar band;
	uchar space;
	uchar hilo;
	uint freq;
};

struct fm_seek_parm {
	uchar err;
	uchar band;
	uchar space;
	uchar hilo;
	uchar seekdir;
	uchar seekth;
	ushort freq;
};

struct fm_check_status {
	uchar status;
	int rssi;
	uint freq;
};

typedef struct _FM_SIGNAL_PARAM_T
{
	unsigned char  nOperInd;	// 	Tune: 	0: tune successful;
								//			1: tune failure
								//	Seek: 	0: seek out valid channel successful
								//			1: seek out valid channel failure
	unsigned char  nStereoInd;			// 	0: Stereo; Other: Mono;
	unsigned short 	nRssi;  			// 	RSSI Value
	unsigned int	nFreqValue; 		// 	Frequency, Unit:KHz
	unsigned int	nPwrIndicator; 	    // 	Power indicator
	unsigned int	nFreqOffset; 		// 	Frequency offset
	unsigned int	nPilotDet; 			// 	pilot_det
	unsigned int	nNoDacLpf; 			// 	no_dac_lpf
} __attribute__((PACKED))FM_SIGNAL_PARAM_T;

typedef struct
{
	unsigned char 	nErrorCode;  //DIAG_FM_REG_ERR_E
	unsigned int 	nStartAddr;
	unsigned int 	nUintCount;
} FM_RW_REG_T;

static int mFmTune = 87500;
static FM_SIGNAL_PARAM_T mFmSignalParm;

static int fm_on(void) {
	int ret = 0;
	struct fm_tune_parm parm = {0};

	fd = open("/dev/fm", O_RDWR);
	if (fd < 0) {
		ALOGD("fm on fail,fd = %d, errno = %d, %s !!!\n", fd, errno, strerror(errno));
		return FM_FAILURE;
	}

	ALOGD("fm on successful, ret = %d, fd = %d \n",ret,fd);
	return ret;
}

static int fm_off(void) {
	int ret = -1;
	int type = 0;

	ret = close(fd);
	if (ret) {
		ALOGD("Radio_Open fail, errno = %d, %s !!!\n", errno, strerror(errno));
		return FM_FAILURE;
	}

	return ret;
}

static int fm_getrssi(void) {
	int rssi,rx_rssi;
	int ret = -1;
	ret = ioctl(fd, FM_IOCTL_GETRSSI, &rssi);
	if (ret) {
		ALOGE("get fm rssi error!ret = %d\n", ret);
		return FM_FAILURE;
	} else {
		rx_rssi = rssi;
		ALOGD("get rssi successful, ret: %d, rssi: %d \n", ret,rx_rssi);
	}
	return rx_rssi;
}

static int fm_tune(uint freq) {
	int ret = -1;
	struct fm_tune_parm parm = {0};

	// use default parameters.
	parm.band = 1;
	parm.hilo = 0;
	parm.space = 1;
	parm.freq = freq;
	ALOGD("fm tune freq: %d", parm.freq);
	ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
	if (ret) {
		ALOGE("tune error! freq = %d, ret = %d\n", parm.freq, ret);
		return FM_FAILURE;
	}
	return ret;
}

static int fm_seek() {
	int ret = -1;
	struct fm_seek_parm parm_seek;

	parm_seek.band = 1;
	parm_seek.freq = 10000;
	parm_seek.hilo = FM_AUTO_HILO_OFF;
	parm_seek.space = FM_SEEK_SPACE;
	parm_seek.seekdir = FM_SEEK_UP;
	parm_seek.seekth = 0;

	ret = ioctl(fd, FM_IOCTL_SEEK, &parm_seek);
	if (ret == 0) {
			rx_parm.freq = parm_seek.freq;
			ALOGD("seek successful, ret: %d, seek station:%d \n", ret, rx_parm.freq);
	} else {
		ALOGE("seek error! ret = %d\n", ret);
		return FM_FAILURE;
	}
	return rx_parm.freq;
}

static int fm_mute() {
	int ret = -1;
	int temp_mute = 1;

	ret = ioctl(fd, FM_IOCTL_MUTE, &temp_mute);
	if (ret == 0) {
			printf("mute successful\n");
	} else {
		ALOGE("mute error! ret = %d\n", ret);
		return FM_FAILURE;
	}
	return ret;
}

static int fm_unmute() {
	int ret = -1;
	int temp_mute = 0;

	ret = ioctl(fd, FM_IOCTL_MUTE, &temp_mute);
	if (ret == 0) {
			printf("unmute successful\n");
	} else {
		ALOGE("unmute error! ret = %d\n", ret);
		return FM_FAILURE;
	}
	return ret;
}

static int fm_atenable_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
	unsigned int length = strlen(p);
	static char buf[1024];
	int ret;

	if (length > sizeof(buf)) {
		ALOGD("out of memory");
		return -1;
	}

	memcpy(buf, p, length);
	buf[length - 1] = 0;
	ALOGD("%s, len: %d", buf, strlen(buf));
	ret = fm_on();
	strncpy(at_rsp, ret == 0 ? NOTIFY_FM_ENABLE : NOTIFY_FM_ERROR,FM_AT_COMMAND_RSP_MAX_LEN);
	return 0;
}

static int fm_atdisable_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
	unsigned int length = strlen(p);
	static char buf[1024];
	int ret;

	if (length > sizeof(buf)) {
		ALOGD("out of memory");
		return -1;
	}

	memcpy(buf, p, length);
	buf[length - 1] = 0;
	ALOGD("%s, len: %d", buf, strlen(buf));
	ret = fm_off();
	strncpy(at_rsp, ret == 0 ? NOTIFY_FM_DISABLE : NOTIFY_FM_ERROR,FM_AT_COMMAND_RSP_MAX_LEN);
	return 0;
}

static int fm_attune_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
	unsigned int length = strlen(p);
	static char buf[1024];
	int ret;

	if (length > sizeof(buf)) {
		ALOGD("out of memory");
		return -1;
	}

	memcpy(buf, p, length);
	buf[length - 1] = 0;
	ALOGD("%s, len: %d", buf, strlen(buf));
	ret = fm_tune(9140);
	strncpy(at_rsp, ret == 0 ? NOTIFY_FM_TUNE : NOTIFY_FM_ERROR,FM_AT_COMMAND_RSP_MAX_LEN);
	return 0;
}

static int fm_atseek_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
	unsigned int length = strlen(p);
	static char buf[1024];
	int ret;
	int rx_freq;

	if (length > sizeof(buf)) {
		ALOGD("out of memory");
		return -1;
	}

	memcpy(buf, p, length);
	buf[length - 1] = 0;
	ALOGD("%s, len: %d", buf, strlen(buf));
	rx_freq = fm_seek();
	ALOGD(" seek freq = %d", rx_freq);
	if (rx_freq == FM_FAILURE) {
		snprintf(at_rsp, FM_AT_COMMAND_RSP_MAX_LEN, "%s",NOTIFY_FM_ERROR);
	} else {
		snprintf(at_rsp, FM_AT_COMMAND_RSP_MAX_LEN, "%s freq=%d",NOTIFY_FM_SEEK,rx_freq);
	}

	return 0;
}

static int fm_atmute_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
	unsigned int length = strlen(p);
	static char buf[1024];
	int ret;

	if (length > sizeof(buf)) {
		ALOGD("out of memory");
		return -1;
	}

	memcpy(buf, p, length);
	buf[length - 1] = 0;
	ALOGD("%s, len: %d", buf, strlen(buf));
	ret = fm_mute();
	strncpy(at_rsp, ret == 0 ? NOTIFY_FM_MUTE : NOTIFY_FM_ERROR,FM_AT_COMMAND_RSP_MAX_LEN);
	return 0;
}

static int fm_atunmute_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
	unsigned int length = strlen(p);
	static char buf[1024];
	int ret;

	if (length > sizeof(buf)) {
		ALOGD("out of memory");
		return -1;
	}

	memcpy(buf, p, length);
	buf[length - 1] = 0;
	ALOGD("%s, len: %d", buf, strlen(buf));
	ret = fm_unmute();
	strncpy(at_rsp, ret == 0 ? NOTIFY_FM_UNMUTE : NOTIFY_FM_ERROR,FM_AT_COMMAND_RSP_MAX_LEN);
	return 0;
}

static int fm_atgetrssi_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
	unsigned int length = strlen(p);
	static char buf[1024];
	int rssi;

	if (length > sizeof(buf)) {
		ALOGD("out of memory");
		return -1;
	}

	memcpy(buf, p, length);
	buf[length - 1] = 0;
	ALOGD("%s, len: %d", buf, strlen(buf));
	rssi = fm_getrssi();
	ALOGD(" get rssi = %d", rssi);
	if (rssi == FM_FAILURE) {
		snprintf(at_rsp, FM_AT_COMMAND_RSP_MAX_LEN, "%s",NOTIFY_FM_ERROR);
	} else {
		snprintf(at_rsp, FM_AT_COMMAND_RSP_MAX_LEN, "%s rssi=%d",NOTIFY_FM_GETRSSI,rssi);
	}

	return 0;
}

void register_this_module_ext(struct eng_callback * reg,int *num)
{
	ALOGD("%s : for FM AT ", __func__);
	int moudles_num = 0;

	sprintf((reg + moudles_num)->at_cmd, "%s", ENABLE_FM_TEST);
	(reg + moudles_num)->eng_linuxcmd_func = fm_atenable_cmd_callback;
	moudles_num++;

	sprintf((reg + moudles_num)->at_cmd, "%s", DISABLE_FM_TEST);
	(reg + moudles_num)->eng_linuxcmd_func = fm_atdisable_cmd_callback;
	moudles_num++;

	sprintf((reg + moudles_num)->at_cmd, "%s", ENABLE_FMTUNE_TEST);
	(reg + moudles_num)->eng_linuxcmd_func = fm_attune_cmd_callback;
	moudles_num++;

	sprintf((reg + moudles_num)->at_cmd, "%s", ENABLE_FMSEEK_TEST);
	(reg + moudles_num)->eng_linuxcmd_func = fm_atseek_cmd_callback;
	moudles_num++;

	sprintf((reg + moudles_num)->at_cmd, "%s", ENABLE_FMMUTE_TEST);
	(reg + moudles_num)->eng_linuxcmd_func = fm_atmute_cmd_callback;
	moudles_num++;

	sprintf((reg + moudles_num)->at_cmd, "%s", ENABLE_FMUNMUTE_TEST);
	(reg + moudles_num)->eng_linuxcmd_func = fm_atunmute_cmd_callback;
	moudles_num++;

	sprintf((reg + moudles_num)->at_cmd, "%s", ENABLE_FMGETRSSI_TEST);
	(reg + moudles_num)->eng_linuxcmd_func = fm_atgetrssi_cmd_callback;
	moudles_num++;

	*num = moudles_num;
	ENG_LOG("register_this_module_ext: %d , register completed",*num);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//_FM_EUT

