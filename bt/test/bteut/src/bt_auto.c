/******************************************************************************
 *
 *  Copyright (C) 2018 Spreadtrum Corporation
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
#include <stdlib.h>
#include <time.h>

#include "bt_auto.h"
#include "bt_hal.h"
#include "vendor_suite.h"

#include "sprd_fts_type.h"
#include "sprd_fts_log.h"

#define MAX_AUTO_SUPPORT_RMTDEV_NUM 6

static const bt_test_kit_t *bt_test_kit = NULL;

static bool is_enable = false;
static bool is_inquiry = false;
static bool timer_created = false;

static bdremote_t *bdremote_ptr = NULL;
static bdremote_t bdremote_auto_db[MAX_AUTO_SUPPORT_RMTDEV_NUM];
static timer_t auto_timer_t;
static int device_count = 0;

static void bt_auto_discovery_timeout(void) {
    LOGD("bt_auto: bt_auto_discovery_timeout");
    if (timer_created == true)
    {
        timer_delete(auto_timer_t);
        timer_created = false;
    }
}

static void btStarttimer(void) {
    int status;
    struct itimerspec ts;
    struct sigevent se;
    memset(&ts, 0, sizeof(struct itimerspec));
    memset(&se, 0, sizeof(struct sigevent));
    LOGD("bt_auto: btStarttimer");
    if (timer_created == false)
    {
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &auto_timer_t;
        se.sigev_notify_function = bt_auto_discovery_timeout;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &auto_timer_t);

        if (status == 0)
            timer_created = true;
        else
            LOGD("bt_auto: Failed to creat discovery_timer");
    }

    if (timer_created == true)
    {
        ts.it_value.tv_sec = 4000 / 1000;
        ts.it_value.tv_nsec = 1000000 * (4200 % 1000);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(auto_timer_t, 0, &ts, 0);
        if (status == -1)
            LOGD("bt_auto: Failed to set discovery_timer");
    }
}

static void btStoptimer(void) {
    LOGD("bt_auto: btStoptimer");
    if (timer_created == true)
    {
        timer_delete(auto_timer_t);
        timer_created = false;
    }
}

static int btOpen(void) {
    int ret = -1;
    LOGD("bt_auto: BT OPEN cmd !\n");
    is_enable = true;
    bdremote_ptr = (bdremote_t* )malloc(sizeof(bdremote_t));
    memset(bdremote_ptr, 0 , sizeof(bdremote_t));
    if (!bt_test_kit) {
        bt_test_kit = bt_test_kit_get_interface();
        if (!bt_test_kit) {
            LOGD("bt_auto: get bt_test_kit failed");
            return -1;
        }
    }
    ret = bt_test_kit->enable(NULL);
    if(ret < 0)
        is_enable = false;
    return ret;
}

static int btGetresult(void) {
    int ret = -1, loop_count = 0;
    int bt_add3high_bytestore[MAX_AUTO_SUPPORT_RMTDEV_NUM] = {0};
    int bt_add3low_bytestore[MAX_AUTO_SUPPORT_RMTDEV_NUM] = {0};
    int bt_add3high_byte = 0, bt_add3low_byte = 0;
    device_count = 0;
    timer_created = false;
    memset(bdremote_auto_db, 0 , sizeof(bdremote_t) * MAX_AUTO_SUPPORT_RMTDEV_NUM);
    LOGD("bt_auto: BT GETRESULT cmd !\n");
    btStarttimer();
    if(is_enable && is_inquiry) {
        if (!bt_test_kit) {
            bt_test_kit = bt_test_kit_get_interface();
            if (!bt_test_kit) {
                LOGD("bt_auto: get bt_test_kit failed");
                return -1;
            }
        }
        do {
            memset(bdremote_ptr, 0 , sizeof(bdremote_t));
            if (bt_test_kit->scan_results(bdremote_ptr) < 0)
                continue;
            bt_add3high_byte = ((int)(bdremote_ptr->addr_u8[0]) << 16) +
                             ((int)(bdremote_ptr->addr_u8[1]) << 8) + (bdremote_ptr->addr_u8[2]);
            bt_add3low_byte = ((int)(bdremote_ptr->addr_u8[3]) << 16) +
                            ((int)(bdremote_ptr->addr_u8[4]) << 8) + (bdremote_ptr->addr_u8[5]);
            for (loop_count = 0; loop_count < MAX_AUTO_SUPPORT_RMTDEV_NUM; loop_count++){
                if (bt_add3high_bytestore[loop_count] == bt_add3high_byte) {
                    if (bt_add3low_bytestore[loop_count] == bt_add3low_byte)
                        break;
                }
                if (loop_count == MAX_AUTO_SUPPORT_RMTDEV_NUM - 1) {
                    LOGD("bt_auto: bt_add3high_byte = %06X, device_count = %d\n", bt_add3high_byte, device_count);
                    LOGD("bt_auto: bt_add3low_byte = %06X, device_count = %d\n", bt_add3low_byte, device_count);
                    bt_add3high_bytestore[device_count] = bt_add3high_byte;
                    bt_add3low_bytestore[device_count] = bt_add3low_byte;
                    memcpy(&bdremote_auto_db[device_count], bdremote_ptr, sizeof(bdremote_t));
                    LOGD("bt_auto: bdremote_auto_db[%d] addr: %02x %02x %02x %02x %02x %02x \n", device_count,
                        bdremote_auto_db[device_count].addr_u8[0], bdremote_auto_db[device_count].addr_u8[1],
                        bdremote_auto_db[device_count].addr_u8[2], bdremote_auto_db[device_count].addr_u8[3],
                        bdremote_auto_db[device_count].addr_u8[4], bdremote_auto_db[device_count].addr_u8[5]);
                    LOGD("bt_auto: bdremote_auto_db[%d] rssi: %02x\n", device_count, bdremote_auto_db[device_count].rssi_val);
                    device_count++;
                }
            }
        } while (((device_count < MAX_AUTO_SUPPORT_RMTDEV_NUM) && timer_created));
        LOGD("bt_auto: device_count: %d\n", device_count);
        if (device_count) {
            ret = device_count;
            btStoptimer();
        } else
            ret = -1;
    }
    return ret;
}

static int btInquiry(void) {
    int ret = -1;
    LOGD("bt_auto: BT INQUIRY cmd !\n");
    if(is_enable) {
        if (!is_inquiry) {
            if (!bt_test_kit) {
                bt_test_kit = bt_test_kit_get_interface();
                if (!bt_test_kit) {
                    LOGD("bt_auto: get bt_test_kit failed");
                    return -1;
                }
            }
            ret = bt_test_kit->discovery(true);
            is_inquiry = true;
            ret = btGetresult();
        } else {
            return 0;
        }
    }
    return ret;
}

static int btClose(void) {
    int ret = -1;
    LOGD("bt_auto: BT CLOSE cmd !\n");
    if (is_enable) {
        if (!bt_test_kit) {
            bt_test_kit = bt_test_kit_get_interface();
            if (!bt_test_kit) {
                LOGD("bt_auto: get bt_test_kit failed");
                return -1;
            }
        }
        is_enable = false;
        ret = bt_test_kit->discovery(false);
        ret = bt_test_kit->disable();
        btStoptimer();
        is_inquiry = false;
        if(bdremote_ptr) {
            free(bdremote_ptr);
            bdremote_ptr = NULL;
        }
        return ret;
    } else {
        is_enable = false;
        is_inquiry = false;
        return 0;
    }
}

int bt_autoTset_parse(char *buf, int len, char *rsp, int rsplen) {
    int ret = -1, loop_count = 0;
    char data[1014] = {0 };
    FUN_ENTER ;

    switch( buf[9] ) {
    case 1: // open bt
        ret = btOpen();
        break;
    case 2: // inquiry
        ret = btInquiry();
        if(ret < 0)
            bdremote_ptr = NULL;
        break;
    case 3: // get inquiry
    {
        LOGD("bt_auto: get inquiry device_count: %d\n", device_count);
        ret = 6 * device_count;
        for (loop_count = 0; loop_count < device_count; loop_count++) {
            data[0 + loop_count * 6] = bdremote_auto_db[loop_count].addr_u8[5];
            data[1 + loop_count * 6] = bdremote_auto_db[loop_count].addr_u8[4];
            data[2 + loop_count * 6] = bdremote_auto_db[loop_count].addr_u8[3];
            data[3 + loop_count * 6] = bdremote_auto_db[loop_count].addr_u8[2];
            data[4 + loop_count * 6] = bdremote_auto_db[loop_count].addr_u8[1];
            data[5 + loop_count * 6] = bdremote_auto_db[loop_count].addr_u8[0];
            LOGD("bt_auto: remote device[%d] addr: %02x %02x %02x %02x %02x %02x \n", loop_count,
                data[0 + loop_count * 6], data[1 + loop_count * 6], data[2 + loop_count * 6],
                data[3 + loop_count * 6], data[4 + loop_count * 6], data[5 + loop_count * 6]);
        }
        break;
    }
    case 4:
        ret = btClose();
        break;
    case 5:
    {
        LOGD("bt_auto: get rssi device_count: %d\n", device_count);
        ret = 10 * device_count;
        int sig = 0x00;
        for (loop_count = 0; loop_count < device_count; loop_count++) {
            sig = bdremote_auto_db[loop_count].rssi_val;
            data[0 + loop_count * 10] = bdremote_auto_db[loop_count].addr_u8[5];
            data[1 + loop_count * 10] = bdremote_auto_db[loop_count].addr_u8[4];
            data[2 + loop_count * 10] = bdremote_auto_db[loop_count].addr_u8[3];
            data[3 + loop_count * 10] = bdremote_auto_db[loop_count].addr_u8[2];
            data[4 + loop_count * 10] = bdremote_auto_db[loop_count].addr_u8[1];
            data[5 + loop_count * 10] = bdremote_auto_db[loop_count].addr_u8[0];
            data[6 + loop_count * 10] = (sig >> 0)  & 0xFF;
            data[7 + loop_count * 10] = (sig >> 8)  & 0xFF;
            data[8 + loop_count * 10] = (sig >> 16) & 0xFF;
            data[9 + loop_count * 10] = (sig >> 24) & 0xFF;
            LOGD("bt_auto: remote device[%d] addr: %02x %02x %02x %02x %02x %02x \n", loop_count,
                data[0 + loop_count * 6], data[1 + loop_count * 6], data[2 + loop_count * 6],
                data[3 + loop_count * 6], data[4 + loop_count * 6], data[5 + loop_count * 6]);
            LOGD("bt_auto: remote device[%d] rssi: %02x\n", loop_count, sig);
        }
        break;
    }
    default:
        break;
    }

    MSG_HEAD_T *msg_head_ptr;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T)-1);
    msg_head_ptr = (MSG_HEAD_T *)(rsp + 1);
    msg_head_ptr->len = 8;
    ALOGD("bt_auto: msg_head_ptr,ret=%d",ret);

    if(ret<0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;
    } else if (ret==0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
    } else if(ret >0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), data, ret);
        msg_head_ptr->len+=ret;
    }
    ALOGD("bt_auto: rsp[1 + sizeof(MSG_HEAD_T):%d]:%d",sizeof(MSG_HEAD_T),rsp[sizeof(MSG_HEAD_T)]);

    rsp[msg_head_ptr->len + 2 - 1]=0x7E;
    ALOGD("bt_auto: dylib test :return len:%d",msg_head_ptr->len + 2);
    ALOGD("bt_auto: engpc->pc:%x %x %x %x %x %x %x %x %x %x",rsp[0],rsp[1],rsp[2],rsp[3],rsp[4],rsp[5],rsp[6],rsp[7],rsp[8],rsp[9]);
    return msg_head_ptr->len + 2;
}
