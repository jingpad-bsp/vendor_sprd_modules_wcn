#include "audio.h"
#include <unistd.h>
#define AUDIO_EXT_CONTROL_PIPE "/dev/pipe/mmi.audio.ctrl"
#define AUDIO_EXT_DATA_CONTROL_PIPE "/data/local/media/mmi.audio.ctrl"
int SendAudioTestCmd(const unsigned char * cmd,int bytes)
{
    int fd = -1;
    int ret = -1;
    int bytes_to_read = bytes;
    if (cmd == NULL) {
	return -1;
    }

	fd = open(AUDIO_EXT_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
		LOGD("open %s err! fd=%d, errno=%d, %s\n", AUDIO_EXT_CONTROL_PIPE, fd, errno, strerror(errno));
		LOGD("open %s\n", AUDIO_EXT_DATA_CONTROL_PIPE);
		fd = open(AUDIO_EXT_DATA_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
		if (fd < 0) {
			LOGD("open %s err! fd=%d, errno=%d, %s\n", AUDIO_EXT_DATA_CONTROL_PIPE, fd, errno, strerror(errno));
			return 1;
		}
    }

	do {
	    ret = write(fd, cmd, bytes);
	    if (ret > 0) {
		if (ret <= bytes) {
		    bytes -= ret;
		}
	    } else if ((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
		LOGD("pipe write error %d, bytes read is %d", errno, bytes_to_read - bytes);
		break;
	    } else {
		LOGD("pipe_write_warning: %d, ret is %d", errno, ret);
	    }
	} while (bytes);

	close(fd);

    if (bytes == bytes_to_read)
	return ret;
    else
	return (bytes_to_read - bytes);
}