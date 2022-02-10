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

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <log/log.h>

#include "sprd_fts_type.h"
#include "sprd_fts_cb.h"

#include "bt_npi.h"
#include "bt_cus.h"
#include "bt_eut.h"
#include "bt_auto.h"
#include "bt_factory.h"

#undef LOG_TAG
#define LOG_TAG "bt_eut"

//char *bt_factory_at = "AT+BTTEST=BTFACTORY,1";
QUERYINTERFACE BT_ptfQueryInterface = NULL;
BT_Factory_Notify ptrNotify = NULL;

#define BTD(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#define BTE(param, ...) {ALOGE(param, ## __VA_ARGS__);}

static int bt_npi_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
    unsigned int length = strlen(p);
    static char buf[1024];

    if (length > sizeof(buf)) {
        BTD("out of memory");
        return -1;
    }

    memcpy(buf, p, length);
    buf[length - 1] = 0;
    BTD("[%s], len: %d", buf, strlen(buf));
    bt_npi_parse(EUT_BT_MODULE_INDEX, buf, at_rsp);
	return 0;
}


static int ble_npi_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
    unsigned int length = strlen(p);
    static char buf[1024];

    if (length > sizeof(buf)) {
        BTD("out of memory");
        return -1;
    }

    memcpy(buf, p, length);
    buf[length - 1] = 0;
    BTD("[%s], len: %d", buf, strlen(buf));
    bt_npi_parse(EUT_BLE_MODULE_INDEX, buf, at_rsp);
	return 0;
}

static int bt_cus_getbtstatus_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
    unsigned int length = strlen(p);
    static char buf[1024];

    if (length > sizeof(buf)) {
        BTD("out of memory");
        return -1;
    }

    memcpy(buf, p, length);
    buf[length - 1] = 0;
    BTD("[%s], len: %d", buf, strlen(buf));
    get_bt_status_parse(buf, at_rsp);
    return 0;
}

static int bt_autoTset_cmd_callback(char *buf, int len, char *rsp, int rsplen)
{
    int ret = -1;
    BTD("pc->engpc:%d number--> %x %x %x %x %x %x %x %x %x %x %x ",
    len, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10]);
    ret = bt_autoTset_parse(buf, len, rsp, rsplen);
    return ret;
}

static int bt_factory_callback(char *diag_cmd, char *at_rsp)
{
    char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
    unsigned int length = strlen(p);
    static char buf[1024];

    if (length > sizeof(buf)) {
        BTD("out of memory");
        return -1;
    }

    memcpy(buf, p, length);
//    buf[length - 1] = 0;
    BTD("[%s], len: %d", buf, strlen(buf));
    bt_factory_parse(buf, at_rsp);
    return 0;
}

void register_this_module_ext(struct eng_callback *reg, int *num)
{
    int moudles_num = 0;
    BTD("%s : for BT EUT", __func__);

    sprintf((reg + moudles_num)->at_cmd, "%s", BT_NPI_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = bt_npi_cmd_callback;
    moudles_num++;

    sprintf((reg + moudles_num)->at_cmd, "%s", BLE_NPI_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = ble_npi_cmd_callback;
    moudles_num++;


    sprintf((reg + moudles_num)->at_cmd, "%s", BT_CUS_GETBTSTATUS_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = bt_cus_getbtstatus_callback;
    moudles_num++;

    sprintf((reg + moudles_num)->at_cmd, "%s", BT_GETBTADDR_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = bt_npi_cmd_callback;
    moudles_num++;

    sprintf((reg + moudles_num)->at_cmd, "%s", BT_SETBTADDR_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = bt_npi_cmd_callback;
    moudles_num++;

    (reg + moudles_num)->type = 0x38;
    (reg + moudles_num)->subtype = 0x0F;
    (reg + moudles_num)->eng_diag_func = bt_autoTset_cmd_callback;
    moudles_num++;

    sprintf((reg + moudles_num)->at_cmd, "%s", BT_FACTORY_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = bt_factory_callback;
    moudles_num++;

    *num = moudles_num;
    BTD("%s %d register complete", __func__, moudles_num);
}

void register_fw_function(struct fw_callback *reg)
{
    BTD("%s register_fw_function", __func__);
    BT_ptfQueryInterface = reg->ptfQueryInterface;

    if (BT_ptfQueryInterface("BT scan devices", &ptrNotify) != 0)
    {
        BTE("%s query interface(send at command) fail!", __func__);
    }
    BTD("%s register_fw_function: ptrNotify = %x", __func__, ptrNotify);
}