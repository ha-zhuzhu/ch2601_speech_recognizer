#include <stdbool.h>
#include <aos/kv.h>
#include <yoc/partition.h>
#include <yoc/init.h>
#include <drv/pin.h>
#include <yoc/at_port.h>
#include <devices/w800.h>
#include <devices/drv_snd_alkaid.h>

#include "app_main.h"
#include "board.h"

#define TAG "init"

netmgr_hdl_t app_netmgr_hdl;
extern at_channel_t spi_channel;

static void network_init()
{
    w800_wifi_param_t w800_param;
    /* init wifi driver and network */
    w800_param.reset_pin      = PA21;
    w800_param.baud           = 1*1000000;
    w800_param.cs_pin         = PA15;
    w800_param.wakeup_pin     = PA25;
    w800_param.int_pin        = PA22;
    w800_param.channel_id     = 0;
    w800_param.buffer_size    = 4*1024;

    wifi_w800_register(NULL, &w800_param);
    app_netmgr_hdl = netmgr_dev_wifi_init();

    if (app_netmgr_hdl) {
        utask_t *task = utask_new("netmgr", 2 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
        netmgr_service_init(task);
        //netmgr_config_wifi(app_netmgr_hdl, "TEST", 4, "TEST1234", 10);
        netmgr_start(app_netmgr_hdl);
    }
}

void board_yoc_init(void)
{
    board_init();
    event_service_init(NULL);
    console_init(CONSOLE_UART_IDX, 115200, 512);
    ulog_init();
    aos_set_log_level(AOS_LL_DEBUG);

    int ret = partition_init();
    if (ret <= 0) {
        LOGE(TAG, "partition init failed");
    } else {
        LOGI(TAG, "find %d partitions", ret);
    }

    aos_kv_init("kv");
    snd_card_alkaid_register(NULL);
    network_init();

    board_cli_init();
}
