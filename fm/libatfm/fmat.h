// Spreadtrum fm at test
// xiaopeng.gao   2020-1-06

#ifndef _FM_2020106_H__
#define _FM_2020106_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_fm {
//-----------------------------------------------------------------------------

typedef unsigned char  byte;
typedef unsigned char  uchar;
#ifndef uint
typedef unsigned int   uint;
#endif // uint
typedef unsigned short ushort;

struct fm_reg_ctl_parm {
	unsigned char err;
	unsigned int addr;
	unsigned int val;
	/*0:write, 1:read*/
	unsigned char rw_flag;
} __packed;

/* ********** ***********FM IOCTL define start ****************/
#define FM_IOC_MAGIC        0xf5

#define FM_IOCTL_POWERUP       _IOWR(FM_IOC_MAGIC, 0, struct fm_tune_parm*)
#define FM_IOCTL_POWERDOWN     _IOWR(FM_IOC_MAGIC, 1, int32_t*)
#define FM_IOCTL_TUNE          _IOWR(FM_IOC_MAGIC, 2, struct fm_tune_parm*)
#define FM_IOCTL_SEEK          _IOWR(FM_IOC_MAGIC, 3, struct fm_seek_parm*)
#define FM_IOCTL_SETVOL        _IOWR(FM_IOC_MAGIC, 4, uint32_t*)
#define FM_IOCTL_GETVOL        _IOWR(FM_IOC_MAGIC, 5, uint32_t*)
#define FM_IOCTL_MUTE          _IOWR(FM_IOC_MAGIC, 6, uint32_t*)
#define FM_IOCTL_GETRSSI       _IOWR(FM_IOC_MAGIC, 7, int32_t*)
#define FM_IOCTL_SCAN          _IOWR(FM_IOC_MAGIC, 8, struct fm_scan_parm*)
#define FM_IOCTL_STOP_SCAN     _IO(FM_IOC_MAGIC,   9)

/* IOCTL and struct for test */
#define FM_IOCTL_GETCHIPID     _IOWR(FM_IOC_MAGIC, 10, uint16_t*)
#define FM_IOCTL_EM_TEST       _IOWR(FM_IOC_MAGIC, 11, struct fm_em_parm*)
#define FM_IOCTL_RW_REG        _IOWR(FM_IOC_MAGIC, 12, struct fm_ctl_parm*)
#define FM_IOCTL_GETMONOSTERO  _IOWR(FM_IOC_MAGIC, 13, uint16_t*)
#define FM_IOCTL_GETCURPAMD    _IOWR(FM_IOC_MAGIC, 14, uint16_t*)
#define FM_IOCTL_GETGOODBCNT   _IOWR(FM_IOC_MAGIC, 15, uint16_t*)
#define FM_IOCTL_GETBADBNT     _IOWR(FM_IOC_MAGIC, 16, uint16_t*)
#define FM_IOCTL_GETBLERRATIO  _IOWR(FM_IOC_MAGIC, 17, uint16_t*)

/* IOCTL for RDS */
#define FM_IOCTL_RDS_ONOFF     _IOWR(FM_IOC_MAGIC, 18, uint16_t*)
#define FM_IOCTL_RDS_SUPPORT   _IOWR(FM_IOC_MAGIC, 19, int32_t*)

#define FM_IOCTL_RDS_SIM_DATA  _IOWR(FM_IOC_MAGIC, 23, uint32_t*)
#define FM_IOCTL_IS_FM_POWERED_UP  _IOWR(FM_IOC_MAGIC, 24, uint32_t*)

/* IOCTL for FM over BT */
#define FM_IOCTL_OVER_BT_ENABLE  _IOWR(FM_IOC_MAGIC, 29, int32_t*)

/* IOCTL for FM ANTENNA SWITCH */
#define FM_IOCTL_ANA_SWITCH     _IOWR(FM_IOC_MAGIC, 30, int32_t*)
#define FM_IOCTL_GETCAPARRAY      _IOWR(FM_IOC_MAGIC, 31, int32_t*)

/* IOCTL for FM I2S Setting  */
#define FM_IOCTL_I2S_SETTING  _IOWR(FM_IOC_MAGIC, 33, struct fm_i2s_setting*)

#define FM_IOCTL_RDS_GROUPCNT   _IOWR(FM_IOC_MAGIC, 34, \
				struct rds_group_cnt_req*)
#define FM_IOCTL_RDS_GET_LOG    _IOWR(FM_IOC_MAGIC, 35, struct rds_raw_data*)

#define FM_IOCTL_SCAN_GETRSSI   _IOWR(FM_IOC_MAGIC, 36, struct fm_rssi_req*)
#define FM_IOCTL_SETMONOSTERO   _IOWR(FM_IOC_MAGIC, 37, int32_t)
#define FM_IOCTL_RDS_BC_RST     _IOWR(FM_IOC_MAGIC, 38, int32_t*)
#define FM_IOCTL_CQI_GET     _IOWR(FM_IOC_MAGIC, 39, struct fm_cqi_req*)
#define FM_IOCTL_GET_HW_INFO    _IOWR(FM_IOC_MAGIC, 40, struct fm_hw_info*)
#define FM_IOCTL_GET_I2S_INFO   _IOWR(FM_IOC_MAGIC, 41, struct fm_i2s_info_t*)
#define FM_IOCTL_IS_DESE_CHAN   _IOWR(FM_IOC_MAGIC, 42, int32_t*)
#define FM_IOCTL_TOP_RDWR _IOWR(FM_IOC_MAGIC, 43, struct fm_top_rw_parm*)
#define FM_IOCTL_HOST_RDWR  _IOWR(FM_IOC_MAGIC, 44, struct fm_host_rw_parm*)

#define FM_IOCTL_PRE_SEARCH _IOWR(FM_IOC_MAGIC, 45, int32_t)
#define FM_IOCTL_RESTORE_SEARCH _IOWR(FM_IOC_MAGIC, 46, int32_t)

#define FM_IOCTL_SET_SEARCH_THRESHOLD   _IOWR(FM_IOC_MAGIC, 47, \
		fm_search_threshold_t*)

#define FM_IOCTL_GET_AUDIO_INFO _IOWR(FM_IOC_MAGIC, 48, struct fm_audio_info_t*)

#define FM_IOCTL_SCAN_NEW       _IOWR(FM_IOC_MAGIC, 60, struct fm_scan_t*)
#define FM_IOCTL_SEEK_NEW       _IOWR(FM_IOC_MAGIC, 61, struct fm_seek_t*)
#define FM_IOCTL_TUNE_NEW       _IOWR(FM_IOC_MAGIC, 62, struct fm_tune_t*)

#define FM_IOCTL_SOFT_MUTE_TUNE _IOWR(FM_IOC_MAGIC, 63, \
	struct fm_softmute_tune_t*)
#define FM_IOCTL_DESENSE_CHECK   _IOWR(FM_IOC_MAGIC, 64, \
	struct fm_desense_check_t*)

/* IOCTL for EM */
#define FM_IOCTL_FULL_CQI_LOG _IOWR(FM_IOC_MAGIC, 70, \
	struct fm_full_cqi_log_t *)
#define FM_IOCTL_CHECK_STATUS  _IOWR(FM_IOC_MAGIC, 71, struct fm_check_status*)

#define FM_IOCTL_DUMP_REG   _IO(FM_IOC_MAGIC, 0xFF)

/* AT Command for test */
#define ENABLE_FM_TEST "AT+SPFMTEST=1"
#define DISABLE_FM_TEST "AT+SPFMTEST=0"
#define ENABLE_FMTUNE_TEST "AT+SPFMTEST=TUNE"
#define ENABLE_FMSEEK_TEST "AT+SPFMTEST=SEEK"
#define ENABLE_FMMUTE_TEST "AT+SPFMTEST=MUTE"
#define ENABLE_FMUNMUTE_TEST "AT+SPFMTEST=UNMUTE"
#define ENABLE_FMGETRSSI_TEST "AT+SPFMTEST=GETRSSI"
#define NOTIFY_FM_ENABLE "\r\n+SPFMTEST OK: ENABLED\r\n"
#define NOTIFY_FM_DISABLE "\r\n+SPFMTEST OK: DISABLED\r\n"
#define NOTIFY_FM_TUNE "\r\n+SPFMTEST OK: TUNED\r\n"
#define NOTIFY_FM_SEEK "\r\n+SPFMTEST OK: SEEKD\r\n"
#define NOTIFY_FM_MUTE "\r\n+SPFMTEST OK: MUTED\r\n"
#define NOTIFY_FM_UNMUTE "\r\n+SPFMTEST OK: UNMUTED\r\n"
#define NOTIFY_FM_GETRSSI "\r\n+SPFMTEST OK: GETRSSI\r\n"
#define NOTIFY_FM_ERROR "\r\n+SPRDFMTEST FAIL\r\n"
#define FM_AT_COMMAND_RSP_MAX_LEN 128


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _FM_2020106_H__
