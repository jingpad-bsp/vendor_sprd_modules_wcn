#ifndef _WIFI_MAC_GENERATOR_H
#define _WIFI_MAC_GENERATOR_H

#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "WMGENER"
#include <log/log.h>

#define WMGENER_TRACING
#ifdef WMGENER_TRACING
#define WMGENER_LOG(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#else
#define WMGENER_LOG(format, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif
