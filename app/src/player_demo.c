/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <aos/aos.h>
#include <aos/cli.h>
#include <avutil/common.h>
#include <avutil/misc.h>
#include "avutil/socket_rw.h"
#include "avutil/web.h"
#include "avutil/named_straightfifo.h"

#include "player_demo.h"
#include "drv/codec.h"
#include "drv/dma.h"
#include <drv/ringbuffer.h>

#define TAG "player_demo"
// static player_t *g_player;

#include <http_client.h>
#include <aos/kernel.h>
#include <aos/debug.h>

#include "string.h"

#define TAG "https_example"

#define MAX_HTTP_RECV_BUFFER 512

// 请求体格式
static const char *boundary = "----WebKitFormBoundarypNjgoVtFRlzPquKE";
#define MY_FORMAT_START "------WebKitFormBoundarypNjgoVtFRlzPquKE\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n"
#define MY_FORMAT_END "\r\n------WebKitFormBoundarypNjgoVtFRlzPquKE--\r\n"

static int e_count = 0; // http请求错误次数

// recorder
csi_codec_t codec;
csi_codec_input_t codec_input_ch;
csi_dma_ch_t dma_ch_input_handle;
#define INPUT_BUF_SIZE 2048
uint8_t input_buf[INPUT_BUF_SIZE];
ringbuffer_t input_ring_buffer;
volatile uint32_t cb_input_transfer_flag = 0;
volatile uint32_t new_data_flag = 0U;
#define PCM_LEN 49152                // 录音总字节数
uint8_t repeater_data_addr[PCM_LEN]; // 录音数据
uint8_t start_to_record = 0;         // 开始录音标志位

uint8_t result_len = 0; // 语音识别结果长度

int _http_event_handler(http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!http_client_is_chunked_response(evt->client))
        {
            // Write out data
            // printf("%.*s", evt->data_len, (char*)evt->data);
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return 0;
}

static void post_audio()
{
    http_client_config_t config = {
        .url = "http://upload.hazhuzhu.com/myasr.php",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
    };

    http_client_handle_t client = http_client_init(&config);
    // 设置 Content-Type
    http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=----WebKitFormBoundarypNjgoVtFRlzPquKE");

    const unsigned char *post_content = repeater_data_addr; // POST 请求体的内容为录音数据
    // 请求体格式相关变量
    const char *content_disposition = "form-data";
    const char *name = "file";
    const char *filename = "raw_recording";
    const char *content_type = "application/octet-stream";
    int boundary_len = strlen(boundary);

    // 制备请求体头部
    int post_start_len = strlen(MY_FORMAT_START) - 8 + strlen(content_disposition) + strlen(name) + strlen(filename) + strlen(content_type) + 1;
    char *post_start = (char *)malloc(post_start_len + 1);
    memset(post_start, 0, sizeof(post_start_len));
    snprintf(post_start, post_start_len, MY_FORMAT_START, content_disposition, name, filename, content_type);
    const char *post_end = MY_FORMAT_END;
    // 设置 Content-Length
    http_client_set_post_len(client, strlen(post_start) + PCM_LEN + strlen(post_end));

    // 设置请求体相关变量
    my_post_data_t post_data = {
        .start = post_start,
        .end = post_end,
        .content = post_content,
        .start_len = strlen(post_start),
        .end_len = strlen(post_end),
        .content_len = PCM_LEN,
    };

    // 进行一次 POST 请求
    http_errors_t err = http_client_myperform(client, &post_data);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP POST Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
        char *raw_data_p;
        result_len = http_client_get_response_raw_data(client, &raw_data_p);
        // 输出 response
        if (result_len)
        {
            char *result = (char *)malloc(result_len + 1);
            memcpy(result, raw_data_p, result_len);
            memcpy(result + result_len, "\0", 1);
            printf("%d\r\n", result_len);
            printf("%s\r\n", result);
            result_len = 0;
            free(result);
        }
    }
    else
    {
        LOGE(TAG, "HTTP POST request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    http_client_cleanup(client);
}

static void codec_input_event_cb_fun(csi_codec_input_t *i2s, csi_codec_event_t event, void *arg)
{
    if (event == CODEC_EVENT_PERIOD_READ_COMPLETE)
    {
        cb_input_transfer_flag += 1;
    }
}

/* 录音 */
static void cmd_mic_handler()
{
    csi_error_t ret;
    csi_codec_input_config_t input_config;
    ret = csi_codec_init(&codec, 0);

    if (ret != CSI_OK)
    {
        printf("csi_codec_init error\n");
        return;
    }

    codec_input_ch.ring_buf = &input_ring_buffer;
    csi_codec_input_open(&codec, &codec_input_ch, 0);
    /* input ch config */
    csi_codec_input_attach_callback(&codec_input_ch, codec_input_event_cb_fun, NULL);
    input_config.bit_width = 16;
    input_config.sample_rate = 8000;
    input_config.buffer = input_buf;
    input_config.buffer_size = INPUT_BUF_SIZE;
    input_config.period = 1024;
    input_config.mode = CODEC_INPUT_DIFFERENCE;
    csi_codec_input_config(&codec_input_ch, &input_config);
    csi_codec_input_analog_gain(&codec_input_ch, 0xbf);
    csi_codec_input_link_dma(&codec_input_ch, &dma_ch_input_handle);

    printf("start recoder\n");
    csi_codec_input_start(&codec_input_ch);

    // 麦克风录音写入数据 48x1024=49152
    while (new_data_flag < 48)
    {
        if (cb_input_transfer_flag)
        {
            csi_codec_input_read_async(&codec_input_ch, repeater_data_addr + (new_data_flag * 1024), 1024);
            cb_input_transfer_flag = 0U; // 回调函数将其置 1
            new_data_flag++;
        }
    }

    new_data_flag = 0;

    printf("stop recoder\n");
    csi_codec_input_stop(&codec_input_ch);
    csi_codec_input_link_dma(&codec_input_ch, NULL);
    csi_codec_input_detach_callback(&codec_input_ch);
    csi_codec_uninit(&codec);

    return;
}


static void mic_task(void *arg)
{
    while (1)
    {
        // 按下按钮后录制并上传
        if (start_to_record == 1)
        {
            cmd_mic_handler();
            post_audio();
            start_to_record = 0;
        }
        aos_msleep(100);
    }
}


static void cmd_http_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "post") == 0)
    {
        post_audio();
    }
    else
    {
        printf("\thttp post\n");
    }
}

static void cmd_mic_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "record") == 0)
    {
        cmd_mic_handler();
    }
    else
    {
        printf("\tmic record\n");
    }
}

int cli_reg_cmd_asr(void)
{
    char url[128];

    // POST 录音
    static const struct cli_command http_cmd_info = {
        "http",
        "http post",
        cmd_http_func,
    };

    // 录音命令
    static const struct cli_command mic_cmd_info = {
        "mic",
        "mic record",
        cmd_mic_func,
    };

    // 注册命令
    aos_cli_register_command(&http_cmd_info);
    aos_cli_register_command(&mic_cmd_info);

    // 新建麦克风任务
    aos_task_new("mic", mic_task, NULL, 10 * 1024);

    return 0;
}
