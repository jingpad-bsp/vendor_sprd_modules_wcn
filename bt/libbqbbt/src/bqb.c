/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 **/

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "libbt-bqb"
#else
#define LOG_TAG "libbt-bqb"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <log/log.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <cutils/sockets.h>
#include <sys/un.h>
#include <poll.h>
#include <signal.h>
#include <termios.h>
#include <cutils/str_parms.h>
#include <cutils/sockets.h>
#include <semaphore.h>
#include <sys/time.h>

#include "bqb.h"

#define UNUSED(expr) do { (void)(expr); } while (0)
#define ENG_BUFFER_SIZE 2048
#define ENG_CMDLINE_LEN 1024
#define HCI_HEADER_LEN 5
#define PATTERN_LEN 4
#define ACL_CREDITS 8
#define SCO_CREDITS 3


#define HCI_READ_BUFFER_SIZE   0x1005

static int bt_fd = -1;
static pthread_t ntid_bqb = (pthread_t)(-1);
int at_fd = -1;

bt_perf_dev_t perf_dev;

controller2uplayer_t controller2uplayer = NULL;
int check_perf_cmd(int fd, char* buf, int len);
void perf_recv_handler(char* buf);
void close_perf(void);

static void thread_exit_handler(int sig) {
    ALOGD("receive signal %d , thread exit\n", sig);
    pthread_exit(0);
}

/*
** Function: eng_receive_data_thread
**
** Description:
**      Receive the data from controller, and send the data to the tester
**
** Arguments:
**     void
**
** Returns:
**
*/
void  eng_receive_data_thread(void) {
    int nRead = 0;
    char buf[1030] = {0};
    struct sigaction actions;

    ALOGD("eng_receive_data_thread");

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR2, &actions, NULL);
    while (1) {
       memset(buf, 0x0, sizeof(buf));
       nRead = read(bt_fd, buf, 1000);

        ALOGD("receive data from controller: %d", nRead);
        if (nRead > 0 && controller2uplayer != NULL) {
             perf_recv_handler(buf);
             controller2uplayer(buf, nRead);
        }
    }
}

/*
** Function: eng_controller_bqb_start
**
** Description:
**      open the ttys0 and create a thread, download bt controller code
**
** Arguments:
**     void
**
** Returns:
**      0
**    -1 failed
*/
int eng_controller_bqb_start(controller2uplayer_t func, bt_bqb_mode_t mode) {
    pthread_t thread;
    int ret = -1;
    uint8_t local_bdaddr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    ALOGV("test eng_controller_bqb_start");
    controller2uplayer = func;
    ALOGV("bt_on");
    bt_fd = bt_on(mode);
    ALOGV("bt_fd = %d", bt_fd);

    ret = pthread_create(&ntid_bqb, NULL, (void *)eng_receive_data_thread, NULL);   /*create thread*/
    if (ret== -1) {
        ALOGD("create thread failed");
    }

    return ret;
}

int eng_controller_bqb_stop(void) {
    ALOGD("stop BQB test receive data thread");

    if (pthread_kill(ntid_bqb, SIGUSR2) != 0) {
        ALOGD("pthread_kill BQB test receive data thread error\n");
        return -1;
    }
    pthread_join(ntid_bqb, NULL);
    bt_off();
    return 0;
}

/*
** Function: eng_send_data
**
** Description:
**      Recieve the data from tester and send the data to controller
**
** Arguments:
**     data  from the tester
**   the data length
**
** Returns:
**      0  success
**    -1 failed
*/
void eng_send_data(char * data, int data_len) {
    int nWritten = 0;
    int count = data_len;
    char * data_ptr = data;

    ALOGD("eng_send_data, fd=%d, len=%d", bt_fd, count);
    if(check_perf_cmd(at_fd, data, data_len)) {
        ALOGD("eng_send_data,cmd %s is perf_cmd", data);
        return;
    }
    lmp_assert();
    while (count) {
        nWritten = write(bt_fd, data, data_len);
        count -= nWritten;
        data_ptr  += nWritten;
    }
    lmp_deassert();
    ALOGD("eng_send_data nWritten %d ", nWritten);
}



static inline int create_server_socket(const char* name) {
  int s = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (s < 0) {
        ALOGD("socket(AF_LOCAL, SOCK_STREAM, 0) failed \n");
        return -1;
    }
  ALOGD("bqb covert name to android abstract name:%s", name);
  if (socket_local_server_bind(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >=
      0) {
    if (listen(s, 5) == 0) {
      ALOGD("bqb listen to local socket:%s, fd:%d", name, s);
      return s;
    } else {
      ALOGD("bqb listen to local socket:%s, fd:%d failed, errno:%d", name, s,
              errno);
    }
  } else {
    ALOGD("bqb create local socket:%s fd:%d, failed, errno:%d", name, s,
            errno);
  }
  close(s);
  return -1;
}

static int accept_server_socket(int sfd) {
  struct sockaddr_un remote;
  struct pollfd pfd;
  int fd;
  socklen_t len = sizeof(struct sockaddr_un);

  // ENG_LOG("accept fd %d", sfd);

  /* make sure there is data to process */
  pfd.fd = sfd;
  pfd.events = POLLIN;

  if (poll(&pfd, 1, 0) == 0) {
    ALOGD("accept poll timeout");
    return -1;
  }

  // ENG_LOG("poll revents 0x%x", pfd.revents);

  if ((fd = accept(sfd, (struct sockaddr*)&remote, &len)) == -1) {
    ALOGD("sock accept failed (%s)", strerror(errno));
    return -1;
  }

  // ENG_LOG("new fd %d", fd);

  return fd;
}


int eng_controller2tester(char * controller_buf, unsigned int data_len) {
    int len = 0;
    len = write(at_fd,controller_buf, data_len);
    ALOGD("bqb test eng_controller2tester %d", len);
    return len;
}


static void bqb_service_enable(int pc_fd, bt_bqb_mode_t mode) {
  struct termios ser_settings;
  int ret = 0;
  ALOGD("bqb bqb_service_enable");
  tcgetattr(pc_fd, &ser_settings);
  cfmakeraw(&ser_settings);
  ser_settings.c_lflag = 0;
  tcsetattr(pc_fd, TCSANOW, &ser_settings);
  ret = eng_controller_bqb_start(eng_controller2tester, mode);
}

static void bqb_service_disable(int pc_fd) {
  struct termios ser_settings;
  int ret = 0;
  ALOGD("bqb bqb_service_disable");
  tcgetattr(pc_fd, &ser_settings);
  cfmakeraw(&ser_settings);
  ser_settings.c_lflag |= (ECHO | ECHONL);
  ser_settings.c_lflag &= ~ECHOCTL;
  tcsetattr(pc_fd, TCSANOW, &ser_settings);
  close_perf();
  ret = eng_controller_bqb_stop();
}



int check_received_str(int fd, char* engbuf, int len) {
    int is_bqb_cmd = 0;
    ALOGD("pc got: %s: %d", engbuf, len);
      if (strstr(engbuf, ENABLE_BQB_TEST)) {
          is_bqb_cmd = 1;
        if (current_bqb_state == BQB_CLOSED) {
                    bqb_service_enable(at_fd, BQB_NORMAL);
          write(fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
        } else {
          write(fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
        }
      } else if (strstr(engbuf, DISABLE_BQB_TEST)) {
          is_bqb_cmd = 1;
        if (current_bqb_state != BQB_CLOSED) {
          bqb_service_disable(at_fd);
          write(fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));
        } else {
          write(fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));
        }
      } else if (strstr(engbuf, TRIGGER_BQB_TEST)) {
          is_bqb_cmd = 1;
        if (current_bqb_state == BQB_OPENED) {
          write(fd, TRIGGER_BQB_ENABLE, strlen(TRIGGER_BQB_ENABLE));
        } else if (current_bqb_state == BQB_CLOSED) {
          write(fd, TRIGGER_BQB_DISABLE, strlen(TRIGGER_BQB_DISABLE));
        }
      }
      return is_bqb_cmd;
}


static void* eng_read_socket_thread(void* par) {
    UNUSED(par);
    int max_fd, ret, bqb_socket_fd, fd;
    int len;
    char engbuf[ENG_BUFFER_SIZE];

    fd_set read_set;
    bqb_socket_fd = create_server_socket(BQB_CTRL_PATH);
    if (bqb_socket_fd < 0) {
        ALOGD("creat bqb server socket error(%s)", strerror(errno));
        return NULL;
      } else {
        max_fd = bqb_socket_fd;
      }
  for (;;) {
        FD_ZERO(&read_set);
        if (bqb_socket_fd > 0) {
          FD_SET(bqb_socket_fd, &read_set);
        }
        ret = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        if (ret == 0) {
          ALOGD("select timeout");
          continue;
        } else if (ret < 0) {
          ALOGD("select failed %s", strerror(errno));
          continue;
        }

        if (FD_ISSET(bqb_socket_fd, &read_set)) {
          ALOGD("bqb_socket_fd got");
          fd = accept_server_socket(bqb_socket_fd);
            if (fd < 0) {
                ALOGD("bqb get service socket fail");
                sleep(1);
                continue;
        }


      memset(engbuf, 0, ENG_BUFFER_SIZE);
      len = read(fd, engbuf, ENG_BUFFER_SIZE);
      ALOGD("bqb control: %s: len: %d", engbuf, len);
      check_received_str(fd, engbuf, len);
      close(fd);
      continue;
    }
  }

  return NULL;
}

void start_socket_thread() {
    pthread_t t1;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // set thread name
    ALOGD("sensen start eng_read_socket_thread");
    pthread_create(&t1, &attr, eng_read_socket_thread, (void*)NULL);
}

void set_bqb_state(int state) {
    ALOGD("current_bqb_state = %d, next state=%d", current_bqb_state, state);
    current_bqb_state = state;
}


static void bqb_init(void) {
    ALOGD("%s", __func__);

    set_bqb_state(BQB_CLOSED);
    start_socket_thread();
}

static void bqb_exit(void) {
    ALOGD("%s", __func__);
// kill thread
    set_bqb_state(BQB_CLOSED);
}

void set_at_fd(int fd) {
    struct termios ser_settings;

    ALOGD("set_at_fd: %d", fd);
    tcgetattr(fd, &ser_settings);
    cfmakeraw(&ser_settings);
    ser_settings.c_lflag = 0;
    tcsetattr(fd, TCSANOW, &ser_settings);
    at_fd = fd;        // at interface fd
}

int get_bqb_state() {
    ALOGD("current_bqb_state = %d", current_bqb_state);
    return current_bqb_state;
}

bt_perf_entity_t* find_link_entity_handle(uint16_t handle) {
    int xx;
    bt_perf_entity_t* entity = &perf_dev.link[0];

    for (xx = 0; xx < MAX_LINKS; xx++, entity++) {
        if ((entity->in_use) && (entity->handle == handle)) {
          return (entity);
        }
    }

    /* If here, no match found */
    return (NULL);
}

bt_perf_entity_t* allocate_entity(uint16_t handle) {
    int xx;
    bt_perf_entity_t* entity = &perf_dev.link[0];

    for (xx = 0; xx < MAX_LINKS; xx++, entity++) {
        if (!entity->in_use) {
            entity->in_use = true;
            entity->handle = handle;
            return (entity);
        }
    }

    /* If here, no match found */
    return (NULL);
}

void release_perf_dev(void) {
    int xx;
    bt_perf_entity_t* entity = &perf_dev.link[0];

    for (xx = 0; xx < MAX_LINKS; xx++, entity++) {
        if (entity->in_use) {
            entity->in_use = false;
            entity->handle = 0;
        }
    }
    perf_dev.is_buf_checked = false;
    perf_dev.link_num = 0;
}

static int CharToInt(char hex)
{
    if (hex>='0' && hex <='9')
        return hex - '0';
    if (hex>='A' && hex <= 'F')
        return hex-'A'+10;
    if(hex>='a' && hex <= 'f')
        return hex-'a'+10;
    return -1;
}

static uint32_t StringToInt(char *a, int size)
{
    uint32_t value = 0;

    for (int i = 0; i < size*2; i++) {
        value += (CharToInt(a[size*2 - 1 - i]) << 4*i);
    }
    //ALOGD("StringToInt value = %d",value);
    return value;
}

static void parse_cmd(char *line, int len, int size, void *buf, int base) {
    char *p_name, *p_value, *sub_value, *p;
    uint8_t *dest = (uint8_t *)buf;
    uint32_t value;

    p_name = strtok_r(line, "=", &p);
    if (NULL == p_name) {
        ALOGD("cmd missing =");
        return;
    }

    p_value = strtok_r(NULL, "\n\r\t", &p);
    if (NULL == p_value) {
        ALOGD("missing value for name: %s", p_name);
        return;
    }

    sub_value = strtok_r(p_value, " ,", &p);
    do {
        if (sub_value == NULL) {
            break;
        }

        if (base == 16) {
            value = StringToInt(sub_value, size) & 0xFFFFFFFF;
        } else {
            value = strtoul(sub_value, 0, 10) & 0xFFFFFFFF;
        }

        switch (size) {
        case sizeof(uint8_t):
            *dest = value & 0xFF;
            dest += size;
            break;

        case sizeof(uint16_t):
            *((uint16_t *)dest) = value & 0xFFFF;
            dest += size;
            break;

        case sizeof(uint32_t):
            *((uint32_t *)dest) = value & 0xFFFFFFFF;
            dest += size;
            break;

        default:
            break;
        }
        sub_value = strtok_r(NULL, " ,", &p);
    } while (--len);
}

static int allocate_link_quota(int total_quota, int link_num, int link_index) {
    int remainder, quota;

    quota = total_quota / link_num;
    remainder = total_quota % link_num;
    quota = quota + (link_index < remainder ? 1 : 0);
    return quota;
}

void  perf_send_data_thread(void *arg) {
    int nWritten = 0;
    int count = 0;
    int offset = 0;
    int has_sent =0;
    struct sigaction actions;

    struct timeval startTime, endTime;
    float Timeuse;

    uint16_t link_handle = *(uint16_t *)arg;
    ALOGD("perf_send_data_thread, link_handle = %d",link_handle);

    bt_perf_entity_t* perf_entity= NULL;
    perf_entity = find_link_entity_handle(link_handle);
    if (!perf_entity) {
        ALOGD("perf_send_data_thread, Invalid handle: %d",link_handle);
        return;
    }

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR2, &actions, NULL);

    while (1) {
        gettimeofday(&startTime, NULL);
        sem_wait(&perf_entity->pkt_num);
        ALOGD("handle: %d, pkt_num>0",link_handle);
        sem_wait(&perf_entity->credit_sem);
        ALOGD("handle: %d, send data to cp",link_handle);
        count = perf_entity->pkt_len;
        offset = 0;
        lmp_assert();
        while (count) {
            nWritten = write(bt_fd, &perf_entity->data[offset], perf_entity->pkt_len);
            if(nWritten < 0) {
                ALOGD("send data to cp write failed!");
                break;
            }
            count -= nWritten;
            offset += nWritten;
        }
        lmp_deassert();
        ALOGD("handle: %d, nWritten %d ",link_handle, nWritten);
        gettimeofday(&endTime, NULL);
        Timeuse = (endTime.tv_sec - startTime.tv_sec) * 1000 + (endTime.tv_usec - startTime.tv_usec) / 1000;
        ALOGD("handle: %d, Timeuse = %3.1f",link_handle, Timeuse);
        ALOGD("handle: %d, data sent = %d",link_handle, ++has_sent);
    }
}

static void read_buffer_size(void)
{
    unsigned char buf[] = {0x01, 0x05, 0x10, 0x00};
    if (!perf_dev.is_buf_checked) {
        ALOGD("read_buffer_size");
        write(bt_fd, buf, sizeof(buf));
        perf_dev.is_buf_checked = true;
    }
}

static int perf_setdata(char* buf) {
    ALOGD("perf_setdata");
    int len = 0;
    char set_data[HCI_HEADER_LEN+PATTERN_LEN] = {0};
    uint16_t handle;
    bt_perf_entity_t* perf_entity= NULL;

    perf_dev.acl_num = ACL_CREDITS;
    perf_dev.sco_num = SCO_CREDITS;

    parse_cmd(buf, HCI_HEADER_LEN+PATTERN_LEN, 1, set_data, 16);
    handle = (((uint16_t)set_data[2] << 8) | set_data[1]) & 0x0FFF;
    ALOGD("perf_setdata, handle= %d",handle);

    perf_entity = allocate_entity(handle);
    if (!perf_entity) {
        ALOGD("perf_setdata, allocate_entity fail");
        return -1;
    }
    memset(perf_entity->data, 0, sizeof(perf_entity->data));

    perf_entity->timer_created = false;
    len = (set_data[3] & 0x00ff) | ((set_data[4] << 8) & 0xff00);
    perf_entity->pkt_len = len + HCI_HEADER_LEN;

    memcpy(perf_entity->data, set_data, HCI_HEADER_LEN);
    for (int i = 0; i < len; i++) {
        perf_entity->data[HCI_HEADER_LEN+i] = set_data[HCI_HEADER_LEN+i%PATTERN_LEN];
    }
    perf_dev.link_num++;
    read_buffer_size();
    return 0;
}

static void timeout_handler(union sigval v) {
    uint16_t link_handle = v.sival_int;
    ALOGD("timeout_handler, link_handle = %d",link_handle);

    bt_perf_entity_t* perf_entity= NULL;
    perf_entity = find_link_entity_handle(link_handle);
    if (!perf_entity) {
        ALOGD("timeout_handler, Invalid handle: %d",link_handle);
        return;
    }

    sem_post(&perf_entity->pkt_num);

    if((--perf_entity->pkt_cnt == 0)&&(perf_entity->timer_created)){
        ALOGD("timeout_handler,delete timer");
        timer_delete(perf_entity->timer_id);
        perf_entity->timer_created = false;
    }
}

static void start_timer(uint64_t delay_millis, alarm_cb cb, timer_t *timer_id, bool *is_created, uint16_t handle) {
    int stat = 0;

    if (*is_created==false)
    {
        struct sigevent sigevent;
        memset(&sigevent, 0, sizeof(sigevent));
        sigevent.sigev_notify = SIGEV_THREAD;
        sigevent.sigev_notify_function = (void (*)(union sigval))cb;
        sigevent.sigev_value.sival_int  = handle;
        stat = timer_create(CLOCK_MONOTONIC, &sigevent, timer_id);
        if (stat == -1)
            ALOGD("timer_create: fail create timer");
        *is_created = true;
    }

    struct itimerspec new_value;
    new_value.it_value.tv_sec = delay_millis / 1000;
    new_value.it_value.tv_nsec = (delay_millis % 1000) * 1000 * 1000;
    new_value.it_interval.tv_sec = delay_millis / 1000;
    new_value.it_interval.tv_nsec = (delay_millis % 1000) * 1000 * 1000;

    ALOGD("set_wake_alarm: -------timer_settime");
    timer_settime(*timer_id, 0, &new_value, NULL);
}

static int perf_start(char* buf) {
    ALOGD(" perf_start");
    uint16_t cmd_data[3*MAX_LINKS] = {0};
    int ret =-1;
    bt_perf_entity_t* perf_entity= NULL;
    uint16_t handle, pkt_cnt, interval;
    int xx, link_quota;

    parse_cmd(buf, 3*perf_dev.link_num, 2, cmd_data, 10);

    for (xx = 0; xx < perf_dev.link_num; xx++) {
        handle = cmd_data[xx*3];
        pkt_cnt = cmd_data[1+xx*3];
        interval = cmd_data[2+xx*3];
        ALOGD("perf_start, handle = %d, pkt_cnt = %d, interval = %d", handle, pkt_cnt, interval);

        perf_entity = find_link_entity_handle(handle);
        if (!perf_entity) {
            ALOGD("perf_start, Invalid handle: %d",handle);
            return -1;
        }
        perf_entity->pkt_cnt = pkt_cnt;

        if (0 == interval) {
            sem_init(&perf_entity->pkt_num, 0, perf_entity->pkt_cnt);
        } else {
            sem_init(&perf_entity->pkt_num, 0, 0);
            start_timer(interval, timeout_handler, &perf_entity->timer_id, &perf_entity->timer_created, handle);
        }

        link_quota = allocate_link_quota(perf_dev.acl_num, perf_dev.link_num, xx);
        sem_init(&perf_entity->credit_sem, 0, link_quota);

        ret = pthread_create(&perf_entity->ntid, NULL, (void *)perf_send_data_thread, &perf_entity->handle);
        if (ret== -1) {
            ALOGD("create thread failed");
            return -1;
        }
    }
    return 0;
}

static void perf_stop(void) {
    ALOGD("perf_stop enter");

    int xx;
    bt_perf_entity_t* perf_entity = &perf_dev.link[0];

    for (xx = 0; xx < MAX_LINKS; xx++, perf_entity++) {
        if (perf_entity->in_use) {
            if(perf_entity->timer_created){
                ALOGD("perf_stop, delete handle %d timer",perf_entity->handle);
                timer_delete(perf_entity->timer_id);
                perf_entity->timer_created = false;
            }

            sem_destroy(&perf_entity->credit_sem);
            sem_destroy(&perf_entity->pkt_num);

            if (pthread_kill(perf_entity->ntid, SIGUSR2) != 0) {
                ALOGD("pthread_kill handle %d perf send data thread error\n",perf_entity->handle);
                return;
            }
            pthread_join(perf_entity->ntid, NULL);
            perf_entity->handle = 0;
            perf_entity->in_use = false;
        }
    }
    perf_dev.is_buf_checked = false;
    perf_dev.link_num = 0;
    ALOGD("perf_stop leave");
}

void close_perf(void) {
    ALOGD("close_perf");
    if (current_perf_state == PERF_STARTED) {
        perf_stop();
        current_perf_state = PERF_STOPPED;
    }
    release_perf_dev();
}

int check_perf_cmd(int fd, char* buf, int len) {
    int is_perf_cmd = 0;
    ALOGD("got: %s: %d", buf, len);
    if (strstr(buf, SETDATA_PERF_TEST)) {
        is_perf_cmd = 1;
        if (current_perf_state == PERF_STOPPED) {
            if (0 == perf_setdata(buf)) {
                write(fd, NOTIFY_SETDATA_PERF, strlen(NOTIFY_SETDATA_PERF));
            } else {
                write(fd, NOTIFY_SETDATA_ERROR, strlen(NOTIFY_SETDATA_ERROR));
            }
        } else {
            write(fd, NOTIFY_SETDATA_ERROR, strlen(NOTIFY_SETDATA_ERROR));
        }
    } else if (strstr(buf, START_PERF_TEST)) {
        is_perf_cmd = 1;
        if (current_perf_state != PERF_STARTED) {
            if (0 == perf_start(buf)) {
                write(fd, NOTIFY_START_PERF, strlen(NOTIFY_START_PERF));
                current_perf_state = PERF_STARTED;
            } else {
                write(fd, NOTIFY_START_ERROR, strlen(NOTIFY_START_ERROR));
            }
        } else {
            write(fd, NOTIFY_START_ERROR, strlen(NOTIFY_START_ERROR));
        }
    } else if (strstr(buf, STOP_PERF_TEST)) {
        is_perf_cmd = 1;
        if (current_perf_state == PERF_STARTED) {
            perf_stop();
            write(fd, NOTIFY_STOP_PERF, strlen(NOTIFY_STOP_PERF));
            current_perf_state = PERF_STOPPED;
        } else {
            write(fd, NOTIFY_STOP_ERROR, strlen(NOTIFY_STOP_ERROR));
        }
    }
    return is_perf_cmd;
}

void process_num_completed_pkts(char* buf) {
    uint8_t num_handles, xx;
    uint16_t handle,num_sent;
    bt_perf_entity_t* perf_entity= NULL;

    num_handles = buf[3];
    ALOGD("process_num_completed_pkts, num_handles = %d",num_handles);

    for (xx = 0; xx < num_handles; xx++) {
        handle = ((uint16_t)buf[5+4*xx] << 8) | buf[4+4*xx];
        num_sent = ((uint16_t)buf[7+4*xx] << 8) | buf[6+4*xx];
        ALOGD("process_num_completed_pkts, handle = %d,num_sent = %d",handle, num_sent);

        perf_entity = find_link_entity_handle(handle);
        if (!perf_entity) {
            ALOGD("perf_start, Invalid handle: %d",handle);
            return;
        }

        while (num_sent--) {
            sem_post(&perf_entity->credit_sem);
        }
    }
}

void perf_recv_handler(char* buf) {
    switch (buf[0]) {
    case 0x04:  //HCI_EVT
        switch (buf[1]) {
        case 0x13:  //NUM_COMPLETED_PACKETS
            {
                if (current_perf_state == PERF_STARTED) {
                    process_num_completed_pkts(buf);
                }
                break;
            }
        case 0x0e:  //CMD_COMPLETE
            {
                uint16_t opcode;
                opcode = ((uint16_t)buf[5] << 8) | buf[4];
                if (opcode == HCI_READ_BUFFER_SIZE) {
                    perf_dev.acl_num = ((uint16_t)buf[11] << 8) | buf[10];
                    perf_dev.sco_num = ((uint16_t)buf[13] << 8) | buf[12];
                    ALOGD("acl_num = %d,sco_num = %d",perf_dev.acl_num, perf_dev.sco_num);
                }
                break;
            }
        break;
        }
    }
}

const bt_bqb_interface_t BLUETOOTH_BQB_INTERFACE = {
    sizeof(bt_bqb_interface_t),
    bqb_init,
    bqb_exit,
    set_at_fd,
    get_bqb_state,
    check_received_str,
    eng_send_data
};
