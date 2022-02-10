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
#include <string.h>
#include <log/log.h>

#include "bt_eut.h"
#include "bt_npi.h"
#include "bt_hal.h"
#include "sprd_fts_cb.h"

#define BTD(param, ...) {ALOGD(param, ## __VA_ARGS__);}

static bdremote_t *bdremote_fact_ptr = NULL;
extern BT_Factory_Notify ptrNotify;
static const bt_test_kit_t *bt_test_kit = NULL;

static enum BT_FACTORY_CMD_INDEX {
    BT_FACTORY_TEST_NONE = 0,
    BT_FACTORY_TEST_START,
    BT_FACTORY_TEST_STOP,
};

static int bt_factory_open(void) {
    int ret = -1;
    BTD("bt_factory_open !\n");
    if (!bt_test_kit) {
        bt_test_kit = bt_test_kit_get_interface();
        if (!bt_test_kit) {
            BTD("get bt_test_kit failed");
            return -1;
        }
    }
    ret = bt_test_kit->enable(NULL);
    return ret;
}

static int bt_factory_inquiry(void) {
    int ret = -1;
    BTD("bt_factory_inquiry !\n");
    if (!bt_test_kit) {
        bt_test_kit = bt_test_kit_get_interface();
        if (!bt_test_kit) {
            BTD("get bt_test_kit failed");
            return -1;
        }
    }
    ret = bt_test_kit->discovery(true);
    return ret;
}

static int bt_factory_close( void )
{
    int ret = -1;
    BTD("bt_factory_close !\n");
    if (!bt_test_kit) {
        bt_test_kit = bt_test_kit_get_interface();
        if (!bt_test_kit) {
            BTD("get bt_test_kit failed");
            return -1;
        }
    }
    ret = bt_test_kit->discovery(false);
    ret = bt_test_kit->disable();
    return ret;
}

void bt_factory_start(void) {
    int ret = -1, count = 0, loop_count = 0;
    int bt_add3high_bytestore[MAX_SUPPORT_RMTDEV_NUM] = {0};
    int bt_add3low_bytestore[MAX_SUPPORT_RMTDEV_NUM] = {0};
    int bt_add3high_byte = 0, bt_add3low_byte = 0;
    bdremote_fact_ptr = (bdremote_t* )malloc(sizeof(bdremote_t));
    ret = bt_factory_open();
    if (ret < 0) {
        BTD("bt_factory_start open bt fail !\n");
        goto BT_factory_fail;
    }

    ret = bt_factory_inquiry();
    if (ret < 0) {
        BTD("bt_factory_start bt Inquiry fail !\n");
        goto BT_factory_fail;
    }

   if (!bt_test_kit) {
        bt_test_kit = bt_test_kit_get_interface();
        if (!bt_test_kit) {
            BTD("bt_factory_start get bt_test_kit failed");
            goto BT_factory_fail;
        }
    }

    do {
        memset(bdremote_fact_ptr, 0 , sizeof(bdremote_t));
         if (bt_test_kit->scan_results(bdremote_fact_ptr) < 0)
            continue;
        bt_add3high_byte = ((int)(bdremote_fact_ptr->addr_u8[0]) << 16) +
                        ((int)(bdremote_fact_ptr->addr_u8[1]) << 8) + (bdremote_fact_ptr->addr_u8[2]);
        bt_add3low_byte = ((int)(bdremote_fact_ptr->addr_u8[3]) << 16) +
                        ((int)(bdremote_fact_ptr->addr_u8[4]) << 8) + (bdremote_fact_ptr->addr_u8[5]);
        for (loop_count = 0; loop_count < MAX_SUPPORT_RMTDEV_NUM; loop_count++){
           if (bt_add3high_bytestore[loop_count] == bt_add3high_byte) {
                if (bt_add3low_bytestore[loop_count] == bt_add3low_byte)
                    break;
            }
            if (loop_count == MAX_SUPPORT_RMTDEV_NUM - 1) {
                BTD("bt_factory_start bt_add3high_byte = %06X, device_count = %d\n", bt_add3high_byte, count);
                BTD("bt_factory_start bt_add3low_byte = %06X, device_count = %d\n", bt_add3low_byte, count);
                /*for (loop_tmp = 0; loop_tmp < MAX_REMOTE_DEVICE_NAME_LEN; loop_tmp++) {
                    BTD("bt_factory_start bt_name[%d] = %02X\n", loop_tmp, bdremote_fact_ptr->name[loop_tmp]);
                }*/
                if (NULL != ptrNotify) {
                    ptrNotify(bdremote_fact_ptr);
                    BTD("bt_factory_start send factory data success!\n");
                } else {
                    BTD("bt_factory_start send factory data fail, ptrNotify is NULL!\n");
                }
                bt_add3high_bytestore[count] = bt_add3high_byte;
                bt_add3low_bytestore[count] = bt_add3low_byte;
                count++;
            }
        }
    } while (count < MAX_SUPPORT_RMTDEV_NUM);
    count = 0;
    bt_factory_close();
    if(bdremote_fact_ptr) {
        free(bdremote_fact_ptr);
        bdremote_fact_ptr = NULL;
    }
    return;

BT_factory_fail:
    ret = bt_factory_close();
    if (ret < 0) {
        BTD("bt_factory_start close bt fail !\n");
    }
    if(bdremote_fact_ptr) {
        free(bdremote_fact_ptr);
        bdremote_fact_ptr = NULL;
    }
    return;
}

void bt_factory_stop(void) {
    int ret = -1;
    ret = bt_factory_close();
    if (ret < 0) {
        BTD("bt_factory_stop close bt fail !\n");
    }
    if(bdremote_fact_ptr) {
        free(bdremote_fact_ptr);
        bdremote_fact_ptr = NULL;
    }
    return;
}

void bt_factory_parse(char *buf, char *rsp) {
    char args0[33] = {0}, args1[33] = {0}, args2[33] = {0}, args3[33] = {0};
    char *data[4] = {args0, args1, args2, args3};
    static char cmdline[1024];
    static int cmd_index = BT_FACTORY_TEST_NONE;

    BTD("bt_factory_parse %s", buf);
    if (cmdline_handle(cmdline, buf) <= 0) {
        BTD("bt_factory_parse Got Cmdline Error: %s", buf);
        return;
    }
    get_sprd_sub_str(cmdline, data, '=', ",", 4, 32);
    BTD("bt_factory_parse dump: args0: %s, args1: %s, args2: %s, args3: %s", args0, args1, args2, args3);
    cmd_index = (args1[0] == '1') ? BT_FACTORY_TEST_START : ((args1[0] == '2') ? BT_FACTORY_TEST_STOP : BT_FACTORY_TEST_NONE);

    switch(cmd_index) {
        case BT_FACTORY_TEST_START: {
            bt_factory_start();
        } break;
        case BT_FACTORY_TEST_STOP: {
            bt_factory_stop();
        } break;
        default:
            BTD("can not match the at command: %d", cmd_index);
            return;
    }
}
