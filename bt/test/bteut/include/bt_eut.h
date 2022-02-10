#ifndef __LIBBT_SPRD_BT_EUT_H__
#define __LIBBT_SPRD_BT_EUT_H__

#define BT_NPI_ATKEY "AT+SPBTTEST"
#define BLE_NPI_ATKEY "AT+SPBLETEST"
#define BT_CUS_GETBTSTATUS_ATKEY "AT+GETBTSTATUS"
#define BT_GETBTADDR_ATKEY "AT+GETBTADDR"
#define BT_SETBTADDR_ATKEY "AT+SETBTADDR"
#define BT_FACTORY_ATKEY "AT+BTTEST"
#include "vendor_suite.h"

typedef int (*BT_Factory_Notify)(bdremote_t *bdremote);

typedef int(*cmd_handle_t)(char *arg, char *res);

typedef struct {
    char *cmd;
    cmd_handle_t cmd_req;
    cmd_handle_t cmd_set;
} bt_eut_action_t;


#endif
