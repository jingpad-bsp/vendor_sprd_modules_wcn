/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "btsnoop_sprd_for_raw"

//#include <utils/Log.h>
#include <log/log.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include "bt_vendor_lib.h"
#include "btsnoop_sprd_for_raw.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <cutils/sockets.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/eventfd.h>

#include <cutils/properties.h>

#include "string.h"



#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>


#ifndef SNOOP_DBG
    #define SNOOP_DBG TRUE
#endif

#if (SNOOP_DBG == TRUE)
    #define SNOOPD(param, ...) ALOGD("%s " param, __FUNCTION__, ## __VA_ARGS__)
#else
    #define SNOOPD(param, ...) {}
#endif

#ifdef SNOOPD_V
    #define SNOOPV(param, ...) ALOGD("%s " param, __FUNCTION__, ## __VA_ARGS__)
#else
    #define SNOOPV(param, ...) {}
#endif

#define SNOOPE(param, ...) ALOGE("%s " param, __FUNCTION__, ## __VA_ARGS__)


/* macro for parsing config file */
#define BTE_STACK_CONF_FILE "/data/misc/bluedroid/bt_stack.conf"
#define CONF_MAX_LINE_LEN 255
#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "=\n\r\t#"

#define EFD_SEMAPHORE (1 << 0)
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

/* macro for socket */
#define LOG_SOCKET_FOR_HCILOG "bt_snoop_slog_socket_for_hcilog"
#define LOG_SOCKET_CTL_FOR_HCILOG "bt_snoop_slog_socket_CTL_for_hcilog"


#define LOG_TURN_ON "ON"
#define LOG_TURN_OFF "OFF"
#define CONTROL_ACK "ACK"
#define CONTROL_NACK "NACK"

#define HCI_EDR3_DH5_PACKET_SIZE    1021
#define BTSNOOP_HEAD_SIZE  25
#define HCI_PIIL_SVAE_BUF_SIZE 5
#define SLOG_STREAM_OUTPUT_BUFFER_SZ      ((HCI_EDR3_DH5_PACKET_SIZE + BTSNOOP_HEAD_SIZE) * 5)
#define HCI_POLL_SIZE (HCI_EDR3_DH5_PACKET_SIZE + BTSNOOP_HEAD_SIZE + HCI_PIIL_SVAE_BUF_SIZE)
#define CTRL_CHAN_RETRY_COUNT 3
#define MAX_EPOLL_EVENTS 3
#define INVALID_FD -1
#define CASE_RETURN_STR(const) case const: return #const;




typedef enum
{
    CMD = 1,
    ACL,
    SCO,
    EVE
} packet_type_t;

// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

static int btsnoop_fd = INVALID_FD;
static int controller_fd = INVALID_FD;
static pthread_t btsnoop_tid = (pthread_t)(-1);
static int config_fd = INVALID_FD;
static int epoll_fd = INVALID_FD;
static int  hci_logging_enabled = 0;
static int hcidump_state = 0;
static struct timeval tv;


static int s_socketpair[2] = {-1, -1};


static uint64_t btsnoop_timestamp(void);
static void btsnoop_write_packet(packet_type_t type, const uint8_t *packet, bool is_received);
static void remove_btsnoop_fd(void);
static void touch_btsnoop_fd(void);
static int is_btsnoop_enable(void);
//extern void bte_trace_level_switch(int debug);

static uint64_t snoop_timestamp = 0;
static int semaphore_fd = INVALID_FD;
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;

static inline int create_server_socket(const char *name)
{
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);

    if (s < 0)
    {
        SNOOPE("socket err, %s (%d)", name, strerror(errno), errno);
        return -1;
    }

    SNOOPD("name: %s", name);

    if (socket_local_server_bind(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0)
    {
        if (listen(s, 5) == 0)
        {
            SNOOPD("listen socket: %s, fd: %d", name, s);
            return s;
        }
        else
        {
            SNOOPD("listen socket: %s, fd: %d failed, %s (%d)", name, s, strerror(errno), errno);
        }
    }
    else
    {
        SNOOPD("failed: %s fd: %d, %s (%d)", name, s, strerror(errno), errno);
    }

    close(s);
    return -1;
}

static inline int accept_server_socket(int sfd)
{
    struct sockaddr_un remote;
    int fd;
    socklen_t len = sizeof(struct sockaddr_un);

    if ((fd = accept(sfd, (struct sockaddr *)&remote, &len)) == -1)
    {
        SNOOPD("sock accept failed (%s)", strerror(errno));
        return -1;
    }

    return fd;
}

static int skt_connect(const char *path, size_t buffer_sz)
{
    int ret;
    int skt_fd;
    int len;

    SNOOPV("connect to %s (sz %zu)", path, buffer_sz);

    if ((skt_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
        SNOOPE("failed to socket (%s)", strerror(errno));
        return -1;
    }

    if (socket_local_client_connect(skt_fd, path,
                                    ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM) < 0)
    {
        SNOOPE("failed to connect (%s)", strerror(errno));
        close(skt_fd);
        return -1;
    }

    len = buffer_sz;
    ret = setsockopt(skt_fd, SOL_SOCKET, SO_SNDBUF, (char *)&len, (int)sizeof(len));

    /* only issue warning if failed */
    if (ret < 0)
    {
        SNOOPE("setsockopt failed (%s)", strerror(errno));
    }

    ret = setsockopt(skt_fd, SOL_SOCKET, SO_RCVBUF, (char *)&len, (int)sizeof(len));

    /* only issue warning if failed */
    if (ret < 0)
    {
        SNOOPE("setsockopt failed (%s)", strerror(errno));
    }

    SNOOPV("connected to stack fd = %d", skt_fd);

    return skt_fd;
}

static void btsnoop_open(void)
{
    int i;

    SNOOPD();

    if (btsnoop_fd != INVALID_FD)
    {
        SNOOPD("btsnoop log file is already open.");
        return;
    }

    for (i = 0; i < CTRL_CHAN_RETRY_COUNT; i++)
    {
        btsnoop_fd = skt_connect(LOG_SOCKET_FOR_HCILOG, SLOG_STREAM_OUTPUT_BUFFER_SZ);

        if (btsnoop_fd >= 0)
        {
            SNOOPD("%s connected", LOG_SOCKET_FOR_HCILOG);
            break;
        }
        else
        {
            usleep(50 * 1000);
            SNOOPD("retry to: %s", LOG_SOCKET_FOR_HCILOG);
        }
    }

    if (btsnoop_fd < 0)
    {
        SNOOPE("%s connect failed: %d", LOG_SOCKET_FOR_HCILOG, btsnoop_fd);
        hci_logging_enabled = 0;
        //        bte_trace_level_switch(0);
        return;
    }
    else
    {
        //        bte_trace_level_switch(1);
    }
}

static void btsnoop_close(void)
{
    SNOOPD();

    if (btsnoop_fd != INVALID_FD)
    {
        remove_btsnoop_fd();
        close(btsnoop_fd);
        btsnoop_fd = INVALID_FD;
        //        bte_trace_level_switch(0);
    }
}
static void btsnoop_raw_waiter_exit(uint32_t sig)
{
    SNOOPD("");

    if (semaphore_fd != INVALID_FD)
    {
        if (eventfd_write(semaphore_fd, 1ULL))
        {
            SNOOPE("%s unable to post to semaphore: %s (%d)", __func__, strerror(errno), errno);
        }
    }

    // zeecy: if we dont set DETACHED, the HIDL will crash when thread_exit()
    pthread_exit(0);
}
static void controller_handler(uint32_t events)
{
    SNOOPE("events: %u", events);

    if (events & (EPOLLERR | EPOLLHUP))
    {
        SNOOPE("controller_fd: %d miss, %s (%d)", controller_fd, strerror(errno), errno);
    }

    if (events & EPOLLIN)
    {
        SNOOPD("EPOLLIN");
        int fd = accept_server_socket(controller_fd);

        if (fd < 0)
        {
            SNOOPD("accept_server_socket failed: fd = %d", fd);
            return;
        }

        char buf[20] = {0};
        int ret = read(fd, buf, sizeof(buf));

        if (ret > 0)
        {
            if (0 == strncmp(buf, LOG_TURN_ON, sizeof(LOG_TURN_ON)))
            {
                SNOOPD("enable hci slog");
                ret = write(fd, CONTROL_ACK, strlen(CONTROL_ACK));
                btsnoop_open();
                touch_btsnoop_fd();
            }
            else if (0 == strncmp(buf, LOG_TURN_ON, sizeof(LOG_TURN_OFF)))
            {
                SNOOPD("disable hci slog");
                ret = write(fd, CONTROL_ACK, strlen(CONTROL_ACK));
                btsnoop_close();
            }
            else
            {
                ret = write(fd, CONTROL_NACK, strlen(CONTROL_NACK));
            }
        }

        close(fd);
    }

}


static void btsnoop_handler(uint32_t events)
{
    SNOOPE();

    if (events & (EPOLLERR | EPOLLHUP))
    {
        SNOOPE("btsnoop_fd: %d miss, %s (%d)", btsnoop_fd, strerror(errno), errno);
        btsnoop_close();
    }
}


static void touch_controller_fd(void)
{
    SNOOPE();

    if (controller_fd < 0)
    {
        controller_fd = create_server_socket(LOG_SOCKET_CTL_FOR_HCILOG);

        if (controller_fd < 0)
        {
            SNOOPE("create_server_socket fail (%d)", controller_fd);
            return;
        }

        struct epoll_event epev;

        epev.events = EPOLLERR | EPOLLHUP | EPOLLIN;

        epev.data.ptr = (void *)controller_handler;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, controller_fd, &epev) == -1)
        {
            SNOOPE("epoll_ctl for btsnoop_fd control socket failed: %s (%d)", strerror(errno), errno);
            return;
        }
    }
}


static void touch_soketpair_fd(int fd)
{
    SNOOPE();

    if (fd < 0)
    {
        return;
    }

    struct epoll_event epev;

    memset(&epev, 0, sizeof(epev));

    epev.events = EPOLLERR | EPOLLHUP | EPOLLIN;

    epev.data.ptr = (void *)btsnoop_raw_waiter_exit;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &epev) == -1)
    {
        SNOOPE("epoll_ctl failed: %s (%d)", strerror(errno), errno);
        return;
    }

}

static void touch_btsnoop_fd(void)
{
    SNOOPE();

    if (btsnoop_fd == INVALID_FD)
    {
        SNOOPE("btsnoop_fd already closed");
        return;
    }

    struct epoll_event epev;

    epev.events = EPOLLERR | EPOLLHUP;

    epev.data.ptr = (void *)btsnoop_handler;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, btsnoop_fd, &epev) == -1)
    {
        SNOOPE("epoll_ctl for btsnoop_fd control socket failed: %s (%d)", strerror(errno), errno);
        return;
    }
}



static void remove_btsnoop_fd(void)
{

    if (btsnoop_fd == INVALID_FD)
    {
        SNOOPE("invalid fd return");
        return;
    }

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, btsnoop_fd, NULL) != 0)
    {
        SNOOPE("epoll_ctl failed");
    }
}



static void *btsnoop_waiter(void *arg)
{

    SNOOPE();

    epoll_fd = epoll_create(MAX_EPOLL_EVENTS);

    if (btsnoop_fd != INVALID_FD)
    {
        touch_btsnoop_fd();
    }

    touch_soketpair_fd((int)arg);
    touch_controller_fd();
    write((int)arg, "1", 1);

    do
    {
        struct epoll_event events[2];
        int nevents, i;

        SNOOPD("epoll_wait");



        nevents = epoll_wait(epoll_fd, events, 2, -1);
        SNOOPD("epoll_wait got: %d", nevents);

        if (nevents == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            SNOOPE("epoll_wait failed: %s (%d)", strerror(errno), errno);
            continue;
        }

        for (i = 0; i < nevents; ++i)
        {
            if (events[i].events & EPOLLERR)
            {
                SNOOPE("EPOLLERR on event #%d", i);
            }

            if (events[i].data.ptr)
            {
                (*(void (*)(uint32_t))events[i].data.ptr)(events[i].events);
            }
        }
    }
    while (1);

    SNOOPE("thread exit");
    return (0);
}

void *btsnoop_raw_start_up(void)
{
    return 0;
    pthread_mutex_lock(&s_lock);
    SNOOPD();

    if (semaphore_fd >= 0)
    {
        close(semaphore_fd);
        semaphore_fd = -1;
    }

    semaphore_fd = eventfd(0, EFD_SEMAPHORE);

    if (semaphore_fd < 0)
    {
        SNOOPE("semaphore_fd: %d err %d %s", semaphore_fd, errno, strerror(errno));
    }

    assert(semaphore_fd >= 0);
    SNOOPE("semaphore_fd %d", semaphore_fd);


    if (s_socketpair[0] >= 0)
    {
        close(s_socketpair[0]);
        s_socketpair[0] = -1;
    }

    if (s_socketpair[1] >= 0)
    {
        close(s_socketpair[1]);
        s_socketpair[1] = -1;
    }

    if (0 > socketpair(AF_UNIX, SOCK_STREAM, 0, s_socketpair))
    {
        SNOOPE("socketpair error %d", errno);
    }

    SNOOPE("socketpair %d %d", s_socketpair[0], s_socketpair[1]);
    fcntl(s_socketpair[0], F_SETFD, fcntl(s_socketpair[0], F_GETFD, 0) & ~O_NONBLOCK);
    fcntl(s_socketpair[1], F_SETFD, fcntl(s_socketpair[1], F_GETFD, 0) & ~O_NONBLOCK);


    if (is_btsnoop_enable())
    {
        btsnoop_open();
    }

    /*
        if (pthread_create(&btsnoop_tid, NULL, epoll_thread, (void *)&btsnoop_fd) < 0)
        {
            SNOOPE("start failed");
        }
    */

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // zeecy: if we dont set DETACHED, the HIDL will crash when thread_exit()
    if (pthread_create(&btsnoop_tid, &attr, btsnoop_waiter, (void *)(size_t)s_socketpair[0]) < 0)
        //if (pthread_create(&btsnoop_tid, 0, btsnoop_waiter, (void *)s_socketpair[0]) < 0)
    {
        SNOOPE("start failed");
    }

    char buff[16] = {0};
    read(s_socketpair[1], buff, sizeof(buff));
    SNOOPD("out");
    pthread_mutex_unlock(&s_lock);
    return NULL;
}

void *btsnoop_raw_shut_down(void)
{
    return 0;
    uint64_t value = 0;
    pthread_mutex_lock(&s_lock);
    SNOOPD("enter");


    /* stop btsnoop waiter thread */
    //ret = pthread_kill(btsnoop_tid, SIGUSR2);
    if (s_socketpair[1] >= 0)
    {
        close(s_socketpair[1]);
        s_socketpair[1] = -1;
    }

    // zeecy: if we dont set DETACHED, the HIDL will crash when thread_exit()
    assert(semaphore_fd != INVALID_FD);

    if (eventfd_read(semaphore_fd, &value) < 0)
    {
        SNOOPE("%s unable to wait on semaphore %d: %s", __func__, semaphore_fd, strerror(errno));
    }

    //pthread_join(btsnoop_tid, 0);

    /* cleanup */
    if (semaphore_fd != INVALID_FD)
    {
        close(semaphore_fd);
        semaphore_fd = INVALID_FD;
    }

    if (config_fd != INVALID_FD)
    {
        close(config_fd);
        config_fd = INVALID_FD;
    }

    if (epoll_fd != INVALID_FD)
    {
        close(epoll_fd);
        epoll_fd = INVALID_FD;
    }

    if (btsnoop_fd >= 0)
    {
        close(btsnoop_fd);
        btsnoop_fd = -1;
    }

    if (controller_fd >= 0)
    {
        close(controller_fd);
        controller_fd = -1;
    }

    if (s_socketpair[0] >= 0)
    {
        close(s_socketpair[0]);
        s_socketpair[0] = -1;
    }


    SNOOPD("out");
    pthread_mutex_unlock(&s_lock);
    return NULL;
}

static int is_btsnoop_enable(void)
{
    int ret = 0;
    char hcidump_property[128] = {0};

    //ret = property_get("ylog.svc.hcidump", hcidump_property, "unknow");
    /*
    ret = 1;
    strcpy(hcidump_property,"running");
    */
    SNOOPD("hcidump state: %s", hcidump_property);

    //if (ret >= 0 && !strcmp(hcidump_property, "running"))
    if (1)
    {
        hcidump_state = 1;
    }
    else
    {
        hcidump_state = 0;
    }

    SNOOPD("is_btsnoop_enable: %d", hci_logging_enabled);
    return hcidump_state;
}


#ifdef SNOOPD_V
static void dump_hex(unsigned char *src, int len)
{
    int buf_size = len * 5 + 1;
    char *mem = malloc(buf_size);
    memset(mem, 0, buf_size);
    char temp[6] = {0};
    int i;

    for (i = 25; i < len; i++)
    {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "0x%02X ", src[i]);
        strcat(mem, temp);
    }

    ALOGD("%s", mem);
    free(mem);
    mem = NULL;
}
#endif

static void capture_from_raw(int type, const unsigned char *buffer, int is_received)
{
    const uint8_t *p = buffer;


    if (btsnoop_fd == INVALID_FD)
    {
        return;
    }

    switch (type)
    {
        case 4:
            btsnoop_write_packet(EVE, p, false);
            break;

        case 2:
            btsnoop_write_packet(ACL, p, (bool)is_received);
            break;

        case 3:
            btsnoop_write_packet(SCO, p, (bool)is_received);
            break;

        case 1:
            btsnoop_write_packet(CMD, p, true);
            break;
    }


    ALOGI("diff: %" PRIu64, (btsnoop_timestamp() - snoop_timestamp));
}


static const btsnoop_sprd_t interface =
{
    .set_api_wants_to_log = NULL,
    .capture_for_raw = capture_from_raw,
    .is_btsnoop_enable = is_btsnoop_enable

};

const btsnoop_sprd_t *btsnoop_sprd_for_raw_get_interface(void)
{
    return &interface;
}

static const char *dump_packet_type(packet_type_t type)
{
    switch (type)
    {
            CASE_RETURN_STR(CMD)
            CASE_RETURN_STR(ACL)
            CASE_RETURN_STR(SCO)
            CASE_RETURN_STR(EVE)

        default:
            return "UNK";
    }
}

static inline void dump_packet(const char *type, int length)
{
    struct tm *packet_time = gmtime(&tv.tv_sec);
    ALOGI("%02d:%02d:%02d:%06ld %s",
          packet_time->tm_hour, packet_time->tm_min, packet_time->tm_sec, tv.tv_usec, type);
}

static uint64_t btsnoop_timestamp(void)
{
    gettimeofday(&tv, NULL);

    // Timestamp is in microseconds.
    uint64_t timestamp = ((uint64_t)tv.tv_sec) * 1000 * 1000LL;
    timestamp += tv.tv_usec;
    timestamp += BTSNOOP_EPOCH_DELTA;
    return timestamp;
}

static void btsnoop_write_packet(packet_type_t type, const uint8_t *packet, bool is_received)
{
    int length_he = 0;
    int length;
    int flags;
    int drops = 0;
    unsigned char hci_pool[HCI_POLL_SIZE] = {0};

    switch (type)
    {
        case CMD:
            length_he = packet[2] + 4;
            flags = 2;
            break;

        case ACL:
            length_he = (packet[3] << 8) + packet[2] + 5;
            flags = is_received;
            break;

        case SCO:
            length_he = packet[2] + 4;
            flags = is_received;
            break;

        case EVE:
            length_he = packet[1] + 3;
            flags = 3;
            break;
    }

    snoop_timestamp = btsnoop_timestamp();
    uint32_t time_hi = snoop_timestamp >> 32;
    uint32_t time_lo = snoop_timestamp & 0xFFFFFFFF;

    length = htonl(length_he);
    flags = htonl(flags);
    drops = htonl(drops);
    time_hi = htonl(time_hi);
    time_lo = htonl(time_lo);

    dump_packet(dump_packet_type(type), length_he);

    if (BTSNOOP_HEAD_SIZE + length_he - 1 > HCI_POLL_SIZE)
    {
        SNOOPE("warning out of buffer");
    }

    memcpy(hci_pool, &length, 4);
    memcpy(hci_pool + 4, &length, 4);
    memcpy(hci_pool + 8, &flags, 4);
    memcpy(hci_pool + 12, &drops, 4);
    memcpy(hci_pool + 16, &time_hi, 4);
    memcpy(hci_pool + 20, &time_lo, 4);
    memcpy(hci_pool + 24, &type, 1);
    memcpy(hci_pool + 25, packet, length_he - 1);
#ifdef SNOOPD_V
    dump_hex(hci_pool, (BTSNOOP_HEAD_SIZE + length_he - 1));
#endif

    if (btsnoop_fd < 0)
    {
        SNOOPE("btsnoop_fd error return");
        return;
    }

    int index = 0;
    int ret = 0;
    pthread_mutex_lock(&s_lock);

    while (1)
    {
        ret = send(btsnoop_fd, hci_pool + index, (BTSNOOP_HEAD_SIZE + length_he - 1 - index), 0);

        if (ret <= 0)
        {
            SNOOPE("send to : %d failed: ret %d %s (%d)", btsnoop_fd, ret, strerror(errno), errno);
            break;
        }

        index += ret;

        if (index >= BTSNOOP_HEAD_SIZE + length_he - 1)
        {
            break;
        }

    }

    pthread_mutex_unlock(&s_lock);

}

