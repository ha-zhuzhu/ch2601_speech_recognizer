/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdlib.h>
#include <string.h>
#include <aos/list.h>
#include <aos/debug.h>

#include <uservice/uservice.h>
#include <uservice/eventid.h>
#include <yoc/sysinfo.h>
#include <board.h>
#include "player_demo.h"

#include "app_main.h"

#define TAG "APP"

static void network_event(uint32_t event_id, const void *param, void *context)
{
    switch(event_id) {
    case EVENT_NETMGR_GOT_IP:
        LOGD(TAG, "net got ip");
        break;
    case EVENT_NETMGR_NET_DISCON:
        LOGD(TAG, "net disconnect");
        break;
    }

    /*do exception process */
    app_exception_event(event_id);
}

int main(void)
{
    board_yoc_init();

    // player_init();

    cli_reg_cmd_player();

    /* Subscribe */
    event_subscribe(EVENT_NETMGR_GOT_IP, network_event, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, network_event, NULL);
}
