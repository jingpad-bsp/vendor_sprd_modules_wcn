//
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#ifndef _AUDIO_20121110_H__
#define _AUDIO_20121110_H__


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include "autotest.h"
int SendAudioTestCmd(const unsigned char * cmd,int bytes);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _WIFI_20121110_H__