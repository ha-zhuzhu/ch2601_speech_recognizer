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
#include "welcome_mp3.h"
#include "drv/codec.h"
#include "drv/dma.h"
#include <drv/ringbuffer.h>

#define TAG "player_demo"
// static player_t *g_player;

#include <http_client.h>
#include <aos/kernel.h>
#include <aos/debug.h>

#define TAG "https_example"

#define MAX_HTTP_RECV_BUFFER 512

#define FILE_FORMAT_START "\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\n"
#define FILE_FORMAT_END "\r\nContent-Disposition: %s; name=\"%s\"; filename=\"\"\r\n"
#define FILE_FORMAT_CONTENT_TYPE_START "\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n"
static const char *boundary = "----WebKitFormBoundarypNjgoVtFRlzPquKE";
#define MY_FORMAT "------WebKitFormBoundarypNjgoVtFRlzPquKE\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n%s\r\n------WebKitFormBoundarypNjgoVtFRlzPquKE--\r\n"

#define MY_FORMAT_START "------WebKitFormBoundarypNjgoVtFRlzPquKE\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n"
#define MY_FORMAT_END "\r\n------WebKitFormBoundarypNjgoVtFRlzPquKE--\r\n"

static int e_count = 0;

csi_codec_t codec;
csi_codec_input_t codec_input_ch;
csi_dma_ch_t dma_ch_input_handle;
#define INPUT_BUF_SIZE 2048
uint8_t input_buf[INPUT_BUF_SIZE];
ringbuffer_t input_ring_buffer;
volatile uint32_t cb_input_transfer_flag = 0;
volatile uint32_t new_data_flag = 0U;
#define WAV_HEAD_LEN 44
const uint8_t wav_head[WAV_HEAD_LEN] = {0x52, 0x49, 0x46, 0x46, 0x24, 0xc0, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00, 0x80, 0x3e, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0xc0, 0x00, 0x00};
uint8_t repeater_data_addr[49152+WAV_HEAD_LEN];

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

static void test_post()
{
    http_client_config_t config = {
        .url = "http://upload.hazhuzhu.com/upload.php",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
    };
    // init 中会对 header 中的 User-Agent 和 Host 赋予默认值
    http_client_handle_t client = http_client_init(&config);
    http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=----WebKitFormBoundarypNjgoVtFRlzPquKE");

    // const char *post_data_content = "hello, hazhuzhu";
    // const unsigned char *post_content = _welcome_mp3;
    const unsigned char *post_content = repeater_data_addr;
    const char *content_disposition = "form-data";
    const char *name = "file";
    const char *filename = "text1.txt";
    const char *content_type = "application/octet-stream";
    int boundary_len = strlen(boundary);

    // int post_data_len=strlen(MY_FORMAT)-10+ strlen(content_disposition)+strlen(name)+strlen(filename)+strlen(content_type)+strlen(post_data_content)+1;
    // int post_data_len=strlen(MY_FORMAT)-10+ strlen(content_disposition)+strlen(name)+strlen(filename)+strlen(content_type)+_welcome_mp3_len+1;
    // char *post_data = (char *)malloc(post_data_len + 1);
    // memset(post_data, 0, sizeof(post_data_len));
    // snprintf(post_data,post_data_len,MY_FORMAT,content_disposition,name,filename,content_type,post_data_content);

    // 制备开头
    int post_start_len = strlen(MY_FORMAT_START) - 8 + strlen(content_disposition) + strlen(name) + strlen(filename) + strlen(content_type) + 1;
    char *post_start = (char *)malloc(post_start_len + 1);
    memset(post_start, 0, sizeof(post_start_len));
    snprintf(post_start, post_start_len, MY_FORMAT_START, content_disposition, name, filename, content_type);
    const char *post_end = MY_FORMAT_END;

    // http_client_set_post_len(client, strlen(post_start) + _welcome_mp3_len + strlen(post_end));
    http_client_set_post_len(client, strlen(post_start) + 49152 + WAV_HEAD_LEN + strlen(post_end));

    my_post_data_t post_data = {
        .start = post_start,
        .end = post_end,
        .content = post_content,
        .start_len = strlen(post_start),
        .end_len = strlen(post_end),
        .content_len = 49152 + WAV_HEAD_LEN,
    };

    // http_client_set_post_field(client, post_data, strlen(post_data));

    http_errors_t err = http_client_myperform(client, &post_data);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP POST Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP POST request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    http_client_cleanup(client);
}

static void test_post2()
{
    http_client_config_t config = {
        .url = "http://upload.hazhuzhu.com/upload.php",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
    };
    // init 中会对 header 中的 User-Agent 和 Host 赋予默认值
    http_client_handle_t client = http_client_init(&config);
    // http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=----WebKitFormBoundarypNjgoVtFRlzPquKE");

    const char *post_data_content = "hello, hazhuzhu";
    const char *content_disposition = "form-data";
    const char *name = "text";
    const char *filename = "text1.txt";
    int boundary_len = strlen(boundary);
    // int post_data_len = 2 + boundary_len + strlen(FILE_FORMAT_START) - 6 + strlen(content_disposition) + strlen(name) + strlen(filename) + 2 + strlen(post_data_content) + 2 + 2 + boundary_len + 2;
    int post_data_len = strlen(MY_FORMAT) - 6 + strlen(content_disposition) + strlen(name) + strlen(post_data_content) + 1;

    const char *post_data = "field1=value1&field2=value2";
    // printf(post_data);
    http_client_set_post_field(client, post_data, strlen(post_data));

    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP POST Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
        LOGI(TAG, *post_data);
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

static void cmd_mic_handler(uint32_t channel_status)
{   
    // 如果一开始就把 wav_head 填入 repeater_data_addr，会作为data填入spiflash，空间不够
    memcpy(repeater_data_addr,wav_head,WAV_HEAD_LEN);
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

    // es7201 才是adc es8156是dac
    // =0即左边的麦克风
    // if (channel_status == 0)
    // {
    //     es8156_all_data_left_channel(&es8156_dev);
    // }
    // else
    // {
    //     es8156_all_data_right_channel(&es8156_dev);
    // }

    printf("start repeater\n");
    csi_codec_input_start(&codec_input_ch);

    // 麦克风记录
    while (new_data_flag < 48)
    {
        if (cb_input_transfer_flag)
        {
            csi_codec_input_read_async(&codec_input_ch, repeater_data_addr + WAV_HEAD_LEN + (new_data_flag * 1024), 1024);
            cb_input_transfer_flag = 0U; // 有回调函数会在读取结束后将其置为一的
            new_data_flag++;
        }
    }

    new_data_flag = 0;

    printf("stop repeater\n");
    csi_codec_input_stop(&codec_input_ch);
    csi_codec_input_link_dma(&codec_input_ch, NULL);
    csi_codec_input_detach_callback(&codec_input_ch);
    csi_codec_uninit(&codec);

    return;
}

/*
static void _player_event(player_t *player, uint8_t type, const void *data, uint32_t len)
{
    int rc;
    UNUSED(len);
    UNUSED(data);
    UNUSED(handle);
    LOGD(TAG, "=====%s, %d, type = %d", __FUNCTION__, __LINE__, type);

    switch (type)
    {
    case PLAYER_EVENT_ERROR:
        rc = player_stop(player);
        break;

    case PLAYER_EVENT_START:
    {
        media_info_t minfo;
        memset(&minfo, 0, sizeof(media_info_t));
        rc = player_get_media_info(player, &minfo);
        LOGD(TAG, "=====rc = %d, duration = %llums, bps = %llu, size = %u", rc, minfo.duration, minfo.bps, minfo.size);
        break;
    }

    case PLAYER_EVENT_FINISH:
        player_stop(player);
        break;

    default:
        break;
    }
}

static char *g_url;
static void _webtask(void *arg)
{
    int cnt = 0, rc, total;
    char *val, buf[128];
    wsession_t *session;
    session = wsession_create();

    rc = wsession_get(session, g_url, 3);
    if (rc)
    {
        LOGE(TAG, "wsession_get fail. rc = %d, code = %d, phrase = %s", rc, session->code, session->phrase);
        goto err;
    }

    val = (char *)dict_get_val(&session->hdrs, "Content-Length");
    CHECK_RET_TAG_WITH_GOTO(val, err);
    total = atoi(val);

    printf("##total cnt = %d Bytes, read percent = %3d %%", total, cnt * 100 / total);
    for (;;)
    {
        rc = wsession_read(session, buf, sizeof(buf), 3 * 1000);
        if (rc <= 0)
        {
            LOGI(TAG, "read may be eof. rc = %d, read cnt = %d", rc, cnt);
            break;
        }
        else
        {
            cnt += rc;
            printf("\b\b\b\b\b%3d %%", cnt * 100 / total);
        }
    }
    // LOGD(TAG, "rc = %8d, cnt = %8d", rc, cnt);

err:
    wsession_destroy(session);
    aos_freep(&g_url);
    return;
}

static void cmd_ipc_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "play") == 0)
    {
        char url[128];

        if (strcmp(argv[2], "welcom") == 0)
        {
            snprintf(url, sizeof(url), "mem://addr=%u&size=%u", (uint32_t)&_welcome_mp3, _welcome_mp3_len);
            player_play(get_player_demo(), url, 0);
        }
        else
        {
            player_play(get_player_demo(), argv[2], 0);
        }
    }
    else if (argc == 2 && strcmp(argv[1], "stop") == 0)
    {
        player_stop(get_player_demo());
    }
    else if (argc == 2 && strcmp(argv[1], "pause") == 0)
    {
        player_pause(get_player_demo());
    }
    else if (argc == 2 && strcmp(argv[1], "resume") == 0)
    {
        player_resume(get_player_demo());
    }
    else if (argc == 3 && strcmp(argv[1], "web") == 0)
    {
        g_url = strdup(argv[2]);
        LOGD(TAG, "g_url = %s", g_url);
        aos_task_new("web_task", _webtask, NULL, 6 * 1024);
    }
    else
    {
        printf("\tplayer play welcom/url[http://]\n");
        printf("\tplayer pause\n");
        printf("\tplayer resume\n");
        printf("\tplayer stop\n");
        printf("\tplayer help");
    }
}
*/
static void cmd_http_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "post") == 0)
    {
        test_post();
    }
    else if (argc == 2 && strcmp(argv[1], "post2") == 0)
    {
        test_post2();
    }
    else
    {
        printf("\thttp_test post\n");
        printf("\thttp_test post2\n");
    }
}

static void cmd_mic_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "left") == 0)
    {
        cmd_mic_handler(0);
    }
    else if (argc == 2 && strcmp(argv[1], "right") == 0)
    {
        cmd_mic_handler(1);
    }
    else
    {
        printf("\tmic left\n");
        printf("\tmic right\n");
    }
}
/*
player_t *get_player_demo()
{
    if (!g_player)
    {
        ply_conf_t ply_cnf;

        player_conf_init(&ply_cnf);
        ply_cnf.vol_en = 1;
        ply_cnf.vol_index = 160; // 0~255
        ply_cnf.event_cb = _player_event;
        ply_cnf.period_num = 12;        // 12 * 5 period_ms = 60ms
        ply_cnf.cache_size = 32 * 1024; // web cache size

        g_player = player_new(&ply_cnf);
    }

    return g_player;
}
*/
int cli_reg_cmd_player(void)
{
    char url[128];
    // static const struct cli_command cmd_info = {
    //     "player",
    //     "player example",
    //     cmd_ipc_func,
    // };

    static const struct cli_command http_cmd_info = {
        "http_test",
        "http_test all",
        cmd_http_func,
    };

    static const struct cli_command mic_cmd_info = {
        "mic",
        "mic left",
        cmd_mic_func,
    };

    // aos_cli_register_command(&cmd_info);
    aos_cli_register_command(&http_cmd_info);
    aos_cli_register_command(&mic_cmd_info);

    // 初始化后默认播放欢迎语句
    snprintf(url, sizeof(url), "mem://addr=%u&size=%u", (uint32_t)&_welcome_mp3, _welcome_mp3_len);
    // player_play(get_player_demo(), url, 0);

    return 0;
}
