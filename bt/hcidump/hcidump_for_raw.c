/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 */

#ifndef LOG_TAG
    #define LOG_TAG "hcidump"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cutils/sockets.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <cutils/str_parms.h>
#include <sys/eventfd.h>
#include <assert.h>
#include <pthread.h>
#include <utils/Log.h>
#include "ytag.h"

#include<inttypes.h>

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef TRUE
    #define TRUE (!FALSE)
#endif

// #define BT_SNOOP_V
#define BT_SNOOP_D

#ifdef BT_SNOOP_V
    #define HCIV(param, ...) ALOGD("%s " param, __FUNCTION__, ## __VA_ARGS__)
#else
    #define HCIV(param, ...) {}
#endif

#ifdef BT_SNOOP_D
    #define HCID(param, ...) ALOGD("%s " param, __FUNCTION__, ## __VA_ARGS__)
#else
    #define HCID(param, ...) {}
#endif

#define HCII(param, ...) ALOGI("%s " param, __FUNCTION__, ## __VA_ARGS__)
#define HCIE(param, ...) ALOGE("%s " param, __FUNCTION__, ## __VA_ARGS__)


#define HCI_EDR3_DH5_PACKET_SIZE    1021
#define BTSNOOP_HEAD_SIZE  25
#define HCI_PIIL_SVAE_BUF_SIZE 5
#define HCI_POLL_SIZE (HCI_EDR3_DH5_PACKET_SIZE + BTSNOOP_HEAD_SIZE + HCI_PIIL_SVAE_BUF_SIZE)

#define HCI_FILE_SIZE_MAX (10 * 1024 * 1024)
#define MAX_LINE_LEN            2048
#define MAX_NAME_LEN            128

#define UNUSED(x) (void)(x)

#define INVALID_FD (-1)
#define EFD_SEMAPHORE (1 << 0)

#define BTSNOOP_SOCKET_FOR_HCILOG "bt_snoop_slog_socket_for_hcilog"
#define BTSNOOP_SOCKET_FOR_MAINLOG "bt_snoop_slog_socket_for_mainlog"

#define LOG_SOCKET_CTL_FOR_HCILOG "bt_snoop_slog_socket_CTL_for_hcilog"
#define LOG_SOCKET_CTL_FOR_MAINLOG "bt_snoop_slog_socket_CTL_for_mainlog"


#define LOG_TURN_ON "ON"
#define LOG_TURN_OFF "OFF"


typedef enum
{
    BTLOG_ON,
    BTLOG_OFF,
} LOG_CTRL_T;

//static int server_socket = -1;
static int file_fd = -1;
static struct ytag ytag = {0, 0, {}};

static pthread_t thread_id_t;

static pthread_t recv_thread;
static pthread_t pop_thread;

pthread_mutex_t lock;


static int accept_fd[10];
static int accept_fd_index;

typedef struct
{
    uint8_t buf[HCI_POLL_SIZE];
    uint8_t data[HCI_POLL_SIZE * 3];
    uint32_t len;
    uint8_t completion;
} HCI_POOL_T;

typedef struct list_node_t
{
    struct list_node_t *next;
    void *data;
    uint32_t len;
    uint64_t timestamp;
} list_node_t;

typedef struct
{
    list_node_t *head;
    list_node_t *tail;
    uint32_t count;
    uint64_t size;
} list_t;

static list_t hcidata_queue =
{
    .head = NULL,
    .tail = NULL,
    .count = 0,
    .size = 0
};

typedef struct
{
    //int efd;
    int fd;
    int event;
    void *cb;
    int index;
} epoll_data_struct;


static epoll_data_struct s_epolld_pool[5];

static HCI_POOL_T hci_pool;

static int semaphore_fd = INVALID_FD;

static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

uint8_t list_append_node(list_t *list, list_node_t *node);
static list_node_t *list_pop_node(list_t *list);
static inline void semaphore_new(void);
static inline void semaphore_free(void);
static inline void semaphore_wait(void);
static inline void semaphore_post(void);

static uint64_t btsnoop_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Timestamp is in microseconds.
    uint64_t timestamp = tv.tv_sec * 1000 * 1000LL;
    timestamp += tv.tv_usec;
    return timestamp;
}

#define BT_CONF_PATH "/data/misc/bluedroid/bt_stack.conf"
#define BT_LOG_STATUS "BtSnoopLogOutput="
#define BT_LOG_PATH "BtSnoopFileName="


static int skt_connect(const char *path, size_t buffer_sz)
{
    int ret;
    int skt_fd;
    int len;

    HCID("connect to %s (sz %zu)", path, buffer_sz);

    if ((skt_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
        HCID("failed to socket (%s)", strerror(errno));
        return -1;
    }

    if (socket_local_client_connect(skt_fd, path,
                                    ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM) < 0)
    {
        HCID("failed to connect (%s)", strerror(errno));
        close(skt_fd);
        return -1;
    }

    if (buffer_sz != 0)
    {
        len = buffer_sz;
        ret = setsockopt(skt_fd, SOL_SOCKET, SO_SNDBUF, (char *)&len, (int)sizeof(len));

        /* only issue warning if failed */
        if (ret < 0)
        {
            HCID("setsockopt failed (%s)", strerror(errno));
        }

        ret = setsockopt(skt_fd, SOL_SOCKET, SO_RCVBUF, (char *)&len, (int)sizeof(len));

        /* only issue warning if failed */
        if (ret < 0)
        {
            HCID("setsockopt failed (%s)", strerror(errno));
        }

        HCID("connected to stack fd = %d", skt_fd);
    }

    return skt_fd;
}

static void hci_log_enable(uint8_t enable)
{
    int fd = skt_connect(LOG_SOCKET_CTL_FOR_MAINLOG, 0);

    if (fd < 0)
    {
        HCID("skt_connect failed : fd = %d", fd);
        return;
    }

    if (enable)
    {
        HCID("enable");
        int ret = write(fd, LOG_TURN_ON, strlen(LOG_TURN_ON));
        uint8_t buf[20] = {0};
        ret = read(fd, buf, sizeof(buf));
    }
    else
    {
        HCID("disable");
        int ret = write(fd, LOG_TURN_OFF, strlen(LOG_TURN_OFF));
        uint8_t buf[20] = {0};
        ret = read(fd, buf, sizeof(buf));
    }

    close(fd);

    fd = skt_connect(LOG_SOCKET_CTL_FOR_HCILOG, 0);

    if (fd < 0)
    {
        HCID("skt_connect failed : fd = %d", fd);
        return;
    }

    if (enable)
    {
        HCID("enable");
        int ret = write(fd, LOG_TURN_ON, strlen(LOG_TURN_ON));
        uint8_t buf[20] = {0};
        ret = read(fd, buf, sizeof(buf));
    }
    else
    {
        HCID("disable");
        int ret = write(fd, LOG_TURN_OFF, strlen(LOG_TURN_OFF));
        uint8_t buf[20] = {0};
        ret = read(fd, buf, sizeof(buf));
    }

    close(fd);
}

static inline int create_server_socket(const char *name)
{
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);

    if (s < 0)
    {
        return -1;
    }

    HCID("name: %s", name);

    if (socket_local_server_bind(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0)
    {
        if (listen(s, 5) == 0)
        {
            HCID("listen socket: %s, fd: %d", name, s);
            return s;
        }
        else
        {
            HCIE("listen socket: %s, fd: %d failed, %s (%d)", name, s, strerror(errno), errno);
        }
    }
    else
    {
        HCIE("failed: %s fd: %d, %s (%d)", name, s, strerror(errno), errno);
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
        HCIE("sock accept failed (%s)", strerror(errno));
        return -1;
    }

    return fd;
}

static int create_log_file(void)
{
    int fd;
    int ret = 0;
    char file_name[256] = {0};
    struct tm timenow, *ptm;
    struct timeval tv;

    fd = dup(STDOUT_FILENO);

    gettimeofday(&tv, NULL);
    ptm = localtime_r(&tv.tv_sec, &timenow);

    strftime(file_name, sizeof(file_name), "btsnoop.%Y-%m-%d_%H.%M.%S", ptm);
    sprintf(file_name + strlen(file_name), ".%03.3d.log.cfa", tv.tv_usec % 1000000 / 1000);

    ytag_newfile_begin(file_name);
    ytag_rawdata("btsnoop\0\0\0\0\1\0\0\x3\xea", 16);

    if (ret <= 0)
    {
        HCIE("write btsnoop header failed: %d", ret);
    }

    return fd;
}

static void dump_hex(uint8_t *src, int len)
{
#define DUMP_LINE_MAX 20
    int i, j, buf_size;
    char *dump_buf = NULL;
    buf_size = DUMP_LINE_MAX * strlen("FF ");
    dump_buf = (char *)malloc(buf_size);

    if (len < DUMP_LINE_MAX)
    {
        for (i = 0; i < len; i++)
        {
            snprintf(dump_buf + 3 * i, buf_size, "%02X ", src[i]);
        }

        dump_buf[len * 3 - 1] = '\0';
        ALOGD("  %s", dump_buf);
    }
    else
    {
        for (i = 0; i < len / DUMP_LINE_MAX; i++)
        {
            for (j = 0; j < DUMP_LINE_MAX; j++)
            {
                snprintf(dump_buf + 3 * j, buf_size, "%02X ", src[i * DUMP_LINE_MAX + j]);
            }

            dump_buf[DUMP_LINE_MAX * 3 - 1] = '\0';
            ALOGD("  %s", dump_buf);
        }

        if (len % DUMP_LINE_MAX)
        {
            for (j = 0; j < len % DUMP_LINE_MAX; j++)
            {
                snprintf(dump_buf + 3 * j, buf_size, "%02X ", src[(i - 1) * DUMP_LINE_MAX + j]);
            }

            dump_buf[len % DUMP_LINE_MAX * 3 - 1] = '\0';
            ALOGD("  %s", dump_buf);
        }
    }

    free(dump_buf);
    dump_buf = NULL;
}


static int parse_and_save(int fd)
{
    int ret, type;
    uint32_t length_a = 0, length_b = 0;

    ret = recv(fd, hci_pool.buf, sizeof(hci_pool.buf), MSG_NOSIGNAL);

    if (ret <= 0)
    {
        HCIE("recv error: %d", ret);
        semaphore_post();
        return -1;
    }

#ifdef BT_SNOOP_V
    dump_hex(hci_pool.buf, ret);
    HCIV("ret=%d, hci_pool.len=%d", ret, hci_pool.len);
#endif

    if ((uint32_t)ret > (uint32_t)sizeof(hci_pool.data) - hci_pool.len)
    {
        HCIE("out of memory, ret=%d, hci_pool.len=%d, sizeof(hci_pool.data)=%d", ret, hci_pool.len, (int)sizeof(hci_pool.data));
        exit(0);
    }
    else
    {
        memcpy(hci_pool.data + hci_pool.len, hci_pool.buf, ret);
        hci_pool.len += ret;
    }

    do
    {
        if (hci_pool.len < BTSNOOP_HEAD_SIZE)
        {
            HCIV("not reach head size");
            break;
        }

        length_a = ((uint32_t)(hci_pool.data[0] & 0xFF)) << 24
                   | ((uint32_t)(hci_pool.data[1] & 0xFF)) << 16
                   | ((uint32_t)(hci_pool.data[2] & 0xFF)) << 8
                   | ((uint32_t)(hci_pool.data[3] & 0xFF)) << 0;
        length_b = ((uint32_t)(hci_pool.data[4] & 0xFF)) << 24
                   | ((uint32_t)(hci_pool.data[5] & 0xFF)) << 16
                   | ((uint32_t)(hci_pool.data[6] & 0xFF)) << 8
                   | ((uint32_t)(hci_pool.data[7] & 0xFF)) << 0;

        memcpy(&length_a, hci_pool.data, 4);
        memcpy(&length_b, hci_pool.data + 4, 4);
        length_a = htonl(length_a);
        length_b = htonl(length_b);

        type = hci_pool.data[24];

        if (length_a == length_b)
        {
            HCIV("length: %d", length_a);

            if (length_a > (uint32_t)sizeof(hci_pool.data))
            {
                HCIE("unknow head length_a %"PRIu32, length_a);
                return -1;
            }

            if (hci_pool.len < length_a + BTSNOOP_HEAD_SIZE - 1)
            {
                HCIV("incomplete");
                break;
            }

            switch (type)
            {
                case 1:
                    if (length_a == (uint32_t)hci_pool.data[27] + 4)
                    {
                        hci_pool.completion = 1;
                        HCIV("COMMAND");
                    }

                    break;

                case 2:
                    if (length_a == (uint32_t)hci_pool.data[27] + 5 + (((uint32_t)hci_pool.data[28]) << 8))
                    {
                        hci_pool.completion = 1;
                        HCIV("ACL");
                    }

                    break;

                case 3:
                    if (length_a == (uint32_t)hci_pool.data[27] + 4)
                    {
                        hci_pool.completion = 1;
                        HCIV("SCO");
                    }

                    break;

                case 4:
                    if (length_a == (uint32_t)hci_pool.data[26] + 3)
                    {
                        hci_pool.completion = 1;
                        HCIV("EVENT");
                    }

                    break;

                default:
                    HCIE("unknow type: %d", type);
                    break;
            }
        }
        else
        {
            HCIE("length_a=%"PRIu32" length_b=%" PRIu32, length_a, length_b);
            return -1;
        }

        if (hci_pool.completion)
        {
            list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t) + (length_a + BTSNOOP_HEAD_SIZE - 1));
            node->data = (uint8_t *)(node + 1);
            memcpy(node->data, hci_pool.data, (length_a + BTSNOOP_HEAD_SIZE - 1));
            node->next = NULL;
            node->len = (length_a + BTSNOOP_HEAD_SIZE - 1);
            node->timestamp = ((uint64_t)(hci_pool.data[16] & 0xFF)) << 56
                              | ((uint64_t)(hci_pool.data[17] & 0xFF)) << 48
                              | ((uint64_t)(hci_pool.data[18] & 0xFF)) << 40
                              | ((uint64_t)(hci_pool.data[19] & 0xFF)) << 32
                              | ((uint64_t)(hci_pool.data[20] & 0xFF)) << 24
                              | ((uint64_t)(hci_pool.data[21] & 0xFF)) << 16
                              | ((uint64_t)(hci_pool.data[22] & 0xFF)) << 8
                              | ((uint64_t)(hci_pool.data[23] & 0xFF)) << 0;
            pthread_mutex_lock(&lock);
            list_append_node(&hcidata_queue, node);
            pthread_mutex_unlock(&lock);
            semaphore_post();
            hci_pool.len -= (length_a + BTSNOOP_HEAD_SIZE - 1);
            HCIV("len: %d, save: %d", hci_pool.len, (length_a + BTSNOOP_HEAD_SIZE - 1));

            if (hci_pool.len)
            {
                memmove(hci_pool.data, &hci_pool.data[length_a + BTSNOOP_HEAD_SIZE - 1], hci_pool.len);

            }

            HCIV("copy complete");
            hci_pool.completion = 0;
            continue;
        }
        else
        {
            break;
        }
    }
    while (1);

    return 0;
}

static inline void semaphore_new(void)
{
    if (semaphore_fd != INVALID_FD)
    {
        close(semaphore_fd);
    }

    semaphore_fd = eventfd(0, EFD_SEMAPHORE);

    if (semaphore_fd == INVALID_FD)
    {
        HCIE("error");
    }
}

static inline void semaphore_free(void)
{
    if (semaphore_fd != INVALID_FD)
    {
        close(semaphore_fd);
    }
}

static inline void semaphore_wait(void)
{
    uint64_t value;
    assert(semaphore_fd != INVALID_FD);

    if (eventfd_read(semaphore_fd, &value) == -1)
    {
        HCIE("%s unable to wait on semaphore: %s", __func__, strerror(errno));
    }
}

static inline void semaphore_post(void)
{
    assert(semaphore_fd != INVALID_FD);

    if (eventfd_write(semaphore_fd, 1ULL) == -1)
    {
        HCIE("%s unable to post to semaphore: %s", __func__, strerror(errno));
    }
}

uint8_t list_append_node(list_t *list, list_node_t *node)
{
    assert(list != NULL);
    assert(data != NULL);

    if (!node)
    {
        HCIE("malloc failed");
        return FALSE;
    }

    if (list->tail == NULL)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
    }

    list->size += node->len;
    ++list->count;
    return TRUE;
}

static list_node_t *list_pop_node(list_t *list)
{
    assert(list != NULL);

    list_node_t *node = NULL;

    if (list->head != NULL)
    {
        node = list->head;

        if (list->head == list->tail)
        {
            list->tail = list->head->next;
        }

        list->head = list->head->next;
        --list->count;
        list->size -= node->len;
    }
    else
    {
        HCIE("not found list head");
    }

    return node;
}

static void btsnoop_stop()
{
    ytag_newfile_end(NULL);
}

static void btsnoop_recv_thread(void *arg)
{
    //UNUSED(arg);
    memset(&hci_pool, 0, sizeof(hci_pool));

    while (parse_and_save((int)arg) == 0) {};

    close((int)arg);
}

static void btsnoop_pop_thread(void *arg)
{
    int ret = 0;
    uint64_t write_time, done_time;
    UNUSED(arg);

    uint64_t file_size = 0;

    do
    {
        semaphore_wait();

        if (!hcidata_queue.count)
        {
            HCID("hcidata_queue.count: %d", (int)hcidata_queue.count);
        }

        pthread_mutex_lock(&lock);
        list_node_t *node = list_pop_node(&hcidata_queue);
        pthread_mutex_unlock(&lock);

        if (node == NULL)
        {
            btsnoop_stop();
            HCIE("pop failed");

            if (file_fd != -1)
            {
                close(file_fd);
                file_fd = -1;
            }

            //return;
            continue;
        }
        else
        {
            if (file_fd < 0)
            {
                file_fd = create_log_file();
            }

        }

        write_time = btsnoop_timestamp();
        ytag_rawdata(node->data, node->len);
        done_time = btsnoop_timestamp();

        if ((done_time - write_time) > (100 * 1000LL))
        {
            uint64_t before = done_time - write_time;
            uint64_t after = done_time - (node->timestamp - BTSNOOP_EPOCH_DELTA);
            ALOGD("write block: %" PRIu64 ": %" PRIu64 " us", before, after);
        }

        if (ret <= 0)
        {
            HCIE("write failed: %d", ret);
            free(node);
            node = NULL;
            continue;
        }

        file_size += node->len;

        free(node);
        node = NULL;
    }
    while (1);
}

static inline void btsnoop_thread_start(int fd)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&recv_thread, &attr,
                       (void *)btsnoop_recv_thread, (void *)(size_t)fd) != 0)
    {
        ALOGE("create error");
    }

    HCIV("out");
}

static inline void btsnoop_pop_thread_start()
{
    HCID("start");
    pthread_mutex_init(&lock, NULL);
    semaphore_new();
    memset(&hci_pool, 0, sizeof(hci_pool));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&pop_thread, &attr,
                       (void *)btsnoop_pop_thread, NULL) != 0)
    {
        ALOGE("create error");
    }
}


static int epoll_add_fd(int efd, int fd, void *arg)
{
    struct epoll_event ev;
    int i = 0;
    memset(&ev, 0, sizeof(ev));

    if (efd < 0 || fd < 0)
    {
        return -1;
    }

    for ( i = 0; i < sizeof(s_epolld_pool) / sizeof(s_epolld_pool[0]); ++i)
    {
        if (s_epolld_pool[i].fd < 0)
        {
            break;
        }
    }

    if (i >= sizeof(s_epolld_pool) / sizeof(s_epolld_pool[0]) )
    {
        return -1;
    }

    s_epolld_pool[i].event = EPOLLERR | EPOLLHUP | EPOLLIN;
    s_epolld_pool[i].fd = fd;
    s_epolld_pool[i].cb = arg;
    s_epolld_pool[i].index = i;

    ev.events = EPOLLERR | EPOLLHUP | EPOLLIN;
    ev.data.ptr = &s_epolld_pool[i];

    return epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
}
static int epoll_del_fd(int efd, int fd, int event, void *arg)
{
    struct epoll_event ev;
    int i = 0;
    memset(&ev, 0, sizeof(ev));

    if (efd < 0 || fd < 0)
    {
        return -1;
    }

    for ( i = 0; i < sizeof(s_epolld_pool) / sizeof(s_epolld_pool[0]); ++i)
    {
        if (s_epolld_pool[i].fd == fd)
        {
            break;
        }
    }

    if (i >= sizeof(s_epolld_pool) / sizeof(s_epolld_pool[0]) )
    {
        return -1;
    }

    s_epolld_pool[i].fd = -1;
    ev.events = s_epolld_pool[i].event;
    ev.data.ptr = &s_epolld_pool[i];
    return epoll_ctl(efd, EPOLL_CTL_DEL, fd, &ev);
}
static int hcilog_handle(int efd, int fd, int event)
{
    int ret = 0;

    if (event & EPOLLIN)
    {

        if (0 > parse_and_save((int)fd))
        {
            ret = epoll_del_fd(efd, fd, EPOLLERR | EPOLLHUP | EPOLLIN, (void *)hcilog_handle);
            close(fd);

            if (ret < 0)
            {
                HCIE("epoll del err %d", errno);
                return ret;
            }
        }
    }
    else
    {
        HCID("epoll event  err %d", event);
        ret = epoll_del_fd(efd, fd, EPOLLERR | EPOLLHUP | EPOLLIN, (void *)hcilog_handle);
        close(fd);

        if (ret < 0)
        {
            HCIE("epoll del err %d", errno);
            return ret;
        }
    }

    return 0;
}


static int mainlog_handle(int efd, int fd, int event)
{

    int ret = 0, clientfd = -1;

    HCID("epoll event  err %d", event);
    ret = epoll_del_fd(efd, fd, EPOLLERR | EPOLLHUP | EPOLLIN, (void *)mainlog_handle);
    close(fd);

    if (ret < 0)
    {
        HCIE("epoll del err %d", errno);
        return ret;
    }

    return 0;
}


static int hcilog_accept_handle(int efd, int fd, int event)
{
    struct epoll_event ev;
    int ret = 0, clientfd = -1;

    HCIE("hci accept recive event %d", event);

    if (event & EPOLLIN)
    {
        memset(&hci_pool, 0, sizeof(hci_pool));
        clientfd = accept_server_socket(fd);

        if (clientfd < 0)
        {
            HCIE("accept err %d", errno);
            return -1;
        }

        fcntl(clientfd, F_SETFD, fcntl(clientfd, F_GETFD, 0) | O_NONBLOCK);

        if ( 0 > epoll_add_fd(efd, clientfd, (void *)hcilog_handle))
        {
            HCIE("epoll_add_fd err %d", errno);
            close(clientfd);
            return -1;
        }
    }
    else
    {
        HCID("epoll event  err %d", event);
        ret = epoll_del_fd(efd, fd, EPOLLERR | EPOLLHUP | EPOLLIN, (void *)hcilog_accept_handle);
        close(fd);

        if (ret < 0)
        {
            HCIE("epoll del err %d", errno);
            return ret;
        }

    }

    return 0;
}


static int mainlog_accept_handle(int efd, int fd, int event)
{
    struct epoll_event ev;
    int ret = 0, clientfd = -1;

    if (event & EPOLLIN)
    {
        clientfd = accept_server_socket(fd);

        if (clientfd < 0)
        {
            HCIE("accept err %d", errno);
            return -1;
        }

        fcntl(clientfd, F_SETFD, fcntl(clientfd, F_GETFD, 0) | O_NONBLOCK);

        if ( 0 > epoll_add_fd(efd, clientfd, (void *)mainlog_handle))
        {
            HCIE("epoll_add_fd err %d", errno);
            close(clientfd);
            return -1;
        }

    }
    else
    {
        HCID("epoll event  err %d", event);
        ret = epoll_del_fd(efd, fd, EPOLLERR | EPOLLHUP | EPOLLIN, (void *)mainlog_accept_handle);
        close(fd);

        if (ret < 0)
        {
            HCIE("epoll del err %d", errno);
            return ret;
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
    int socket_f, socket_s;
    uint8_t action = TRUE;
    int efd = -1;

    UNUSED(argc);
    UNUSED(argv);

#if 0   //for future use
    if (argc > 1)
    {
        char tmp[128] = {0};
        int i = 0;
        int fd = -1;
        int ret = 0;
        strncat(tmp, "logcmd ", sizeof(tmp) - 1 - strlen(tmp));

        for (i = 1; i < argc; i++)
        {
            strncat(tmp, argv[i], sizeof(tmp) - 1 - strlen(tmp));
            strncat(tmp, " ", sizeof(tmp) - 1 - strlen(tmp));
        }

        printf("=====send logcmd %s\n", tmp);
        fd = skt_connect(LOG_SOCKET_CTL_FOR_MAINLOG, 0);

        if (fd >= 0)
        {
            ret = send(fd, tmp, strlen(tmp), 0);
            bzero(tmp, sizeof(tmp));
            ret = recv(fd, tmp, sizeof(tmp) - 1, 0);

            if (ret > 0)
            {
                printf("========recv : %s\n", tmp);
            }
        }

        if (fd >= 0)
        {
            close(fd);
        }

        exit(0);
    }
#endif

    for (int var = 0; var < sizeof(accept_fd) / sizeof(accept_fd[0]); ++var)
    {
        accept_fd[var] = -1;

    }

    btsnoop_pop_thread_start();
    memset(s_epolld_pool, 0, sizeof(s_epolld_pool) / sizeof(s_epolld_pool[0]));

    for (int i = 0; i < sizeof(s_epolld_pool) / sizeof(s_epolld_pool[0]); ++i)
    {
        s_epolld_pool[i].fd = -1;
    }

    efd = epoll_create(2);

    if (efd < 0)
    {
        HCIE("epoll create err %d", errno);
        return -1;
    }

    socket_f = create_server_socket(BTSNOOP_SOCKET_FOR_HCILOG);
    fcntl(socket_f, F_SETFD, fcntl(socket_f, F_GETFD, 0) | O_NONBLOCK);

    if (socket_f < 0 || 0 > epoll_add_fd(efd, socket_f, (void *)hcilog_accept_handle))
    {
        exit(0);
    }


    socket_f = create_server_socket(BTSNOOP_SOCKET_FOR_MAINLOG);
    fcntl(socket_f, F_SETFD, fcntl(socket_f, F_GETFD, 0) | O_NONBLOCK);

    if (socket_f < 0 || 0 > epoll_add_fd(efd, socket_f, (void *)mainlog_accept_handle))
    {
        exit(0);
    }


    hci_log_enable(action);
    struct epoll_event ev[5];
    int ret;

    do
    {
        memset(ev, 0, sizeof(ev));

        ret = epoll_wait(efd, ev, sizeof(ev) / sizeof(ev[0]), -1);

        if (ret <= 0)
        {
            HCIE("wait err %d", errno);
            return -1;
        }

        for (int i = 0; i < ret; ++i)
        {
            epoll_data_struct *tmp = (epoll_data_struct *)ev[i].data.ptr;

            if (!tmp)
            {
                HCIE("ptr is null");
                break;
            }

            int (*func)(int, int, int ) = (int (*)(int, int, int )) (tmp->cb);

            if (func)
            {
                func(efd, tmp->fd, ev[i].events);
            }
            else
            {
                HCIE("func is null");
            }
        }

    }
    while (1);
}
