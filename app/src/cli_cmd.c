/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */


#include <aos/cli.h>


void cli_reg_cmd_ifconfig(void);

void board_cli_init()
{
    aos_cli_init();
    cli_reg_cmd_ifconfig();
}
