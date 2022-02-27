/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#ifndef __PLAYER_DEMO_H__
#define __PLAYER_DEMO_H__

#ifdef __cplusplus
extern "C"
{
#endif
    /* 发送录音文件 */
    static void post_audio();

    /* 录音 */
    static void cmd_mic_handler();

    /* 录音任务 */
    static void mic_task(void *arg);
    static void cmd_http_func(char *wbuf, int wbuf_len, int argc, char **argv);
    static void cmd_mic_func(char *wbuf, int wbuf_len, int argc, char **argv);
    int cli_reg_cmd_asr(void);

#ifdef __cplusplus
}
#endif

#endif /* __PLAYER_DEMO_H__ */
