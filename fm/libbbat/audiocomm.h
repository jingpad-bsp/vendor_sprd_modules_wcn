#ifndef _AUDIO_COMMON_H
#define _AUDIO_COMMON_H

#define AUDCTL "/dev/pipe/mmi.audio.ctrl"
#define AUDIO_EXT_DATA_CONTROL_PIPE "/data/local/media/mmi.audio.ctrl"
#define SPRD_AUDIO_FILE "/system/media/engtest_sample.pcm"

int SendAudioTestCmd(const uchar * cmd,int bytes);

#endif