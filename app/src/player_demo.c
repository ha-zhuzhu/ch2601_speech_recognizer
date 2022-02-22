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

#define TAG "player_demo"
static player_t *g_player;

#include <http_client.h>
#include <aos/kernel.h>
#include <aos/debug.h>

#define TAG "https_example"

static char ca_crt_rsa[] = {
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\r\n"
    "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\r\n"
    "DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\r\n"
    "SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\r\n"
    "GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\r\n"
    "AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\r\n"
    "q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\r\n"
    "SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\r\n"
    "Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\r\n"
    "a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\r\n"
    "/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\r\n"
    "AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\r\n"
    "CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\r\n"
    "bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\r\n"
    "c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\r\n"
    "VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\r\n"
    "ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\r\n"
    "MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\r\n"
    "Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\r\n"
    "AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\r\n"
    "uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\r\n"
    "wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\r\n"
    "X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\r\n"
    "PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\r\n"
    "KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\r\n"
    "-----END CERTIFICATE-----\r\n"
    " "};

#define MAX_HTTP_RECV_BUFFER 512

#define FILE_FORMAT_START "\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\n"
#define FILE_FORMAT_END "\r\nContent-Disposition: %s; name=\"%s\"; filename=\"\"\r\n"
#define FILE_FORMAT_CONTENT_TYPE_START "\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n"
static const char *boundary = "----WebKitFormBoundarypNjgoVtFRlzPquKE";
#define MY_FORMAT "------WebKitFormBoundarypNjgoVtFRlzPquKE\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n%s\r\n------WebKitFormBoundarypNjgoVtFRlzPquKE--\r\n"

#define MY_FORMAT_START "------WebKitFormBoundarypNjgoVtFRlzPquKE\r\nContent-Disposition: %s; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n"
#define MY_FORMAT_END "\r\n------WebKitFormBoundarypNjgoVtFRlzPquKE--\r\n"

static int e_count = 0;

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
    const unsigned char *post_content = _welcome_mp3;
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

    http_client_set_post_len(client, strlen(post_start) + _welcome_mp3_len + strlen(post_end));

    my_post_data_t post_data = {
        .start = post_start,
        .end = post_end,
        .content = post_content,
        .start_len = strlen(post_start),
        .end_len = strlen(post_end),
        .content_len = _welcome_mp3_len,
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

static void http_rest_with_url()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);

    // GET
    http_errors_t err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP GET Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP GET request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    // POST
    const char *post_data = "field1=value1&field2=value2";
    http_client_set_url(client, "http://httpbin.org/post");
    http_client_set_method(client, HTTP_METHOD_POST);
    http_client_set_post_field(client, post_data, strlen(post_data));
    err = http_client_perform(client);
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

    // PUT
    http_client_set_url(client, "http://httpbin.org/put");
    http_client_set_method(client, HTTP_METHOD_PUT);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP PUT Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP PUT request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    // PATCH
    http_client_set_url(client, "http://httpbin.org/patch");
    http_client_set_method(client, HTTP_METHOD_PATCH);
    http_client_set_post_field(client, NULL, 0);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP PATCH Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP PATCH request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    // DELETE
    http_client_set_url(client, "http://httpbin.org/delete");
    http_client_set_method(client, HTTP_METHOD_DELETE);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP DELETE Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP DELETE request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    // HEAD
    http_client_set_url(client, "http://httpbin.org/head");
    http_client_set_method(client, HTTP_METHOD_HEAD);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP HEAD Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP HEAD request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    http_client_cleanup(client);
}

static void http_rest_with_hostname_path()
{
    http_client_config_t config = {
        .host = "httpbin.org",
        .path = "/get",
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);

    // GET
    http_errors_t err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP GET Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP GET request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    // POST
    const char *post_data = "field1=value1&field2=value2";
    http_client_set_url(client, "/post");
    http_client_set_method(client, HTTP_METHOD_POST);
    http_client_set_post_field(client, post_data, strlen(post_data));
    err = http_client_perform(client);
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

    // PUT
    http_client_set_url(client, "/put");
    http_client_set_method(client, HTTP_METHOD_PUT);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP PUT Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP PUT request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    // PATCH
    http_client_set_url(client, "/patch");
    http_client_set_method(client, HTTP_METHOD_PATCH);
    http_client_set_post_field(client, NULL, 0);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP PATCH Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP PATCH request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    // DELETE
    http_client_set_url(client, "/delete");
    http_client_set_method(client, HTTP_METHOD_DELETE);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP DELETE Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP DELETE request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    // HEAD
    http_client_set_url(client, "/get");
    http_client_set_method(client, HTTP_METHOD_HEAD);
    err = http_client_perform(client);
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP HEAD Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "HTTP HEAD request failed: 0x%x @#@@@@@@", (err));
        e_count++;
    }

    http_client_cleanup(client);
}

static void http_auth_basic()
{
    http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/basic-auth/user/passwd",
        .event_handler = _http_event_handler,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP Basic Auth Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void http_auth_basic_redirect()
{
    http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/basic-auth/user/passwd",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP Basic Auth redirect Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void http_auth_digest()
{
    http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/digest-auth/auth/user/passwd/MD5/never",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP Digest Auth Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void https_with_url()
{
    http_client_config_t config = {
        .url = "https://www.howsmyssl.com",
        .event_handler = _http_event_handler,
        .cert_pem = ca_crt_rsa,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTPS Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void https_with_hostname_path()
{
    http_client_config_t config = {
        .host = "www.howsmyssl.com",
        .path = "/",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = ca_crt_rsa,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTPS Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void http_relative_redirect()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/relative-redirect/3",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP Relative path redirect Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void http_absolute_redirect()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/absolute-redirect/3",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP Absolute path redirect Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void http_redirect_to_https()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/redirect-to?url=https%3A%2F%2Fwww.howsmyssl.com",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP redirect to HTTPS Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void http_download_chunk()
{
    http_client_config_t config = {
        .url = "http://httpbin.org/stream-bytes/8912",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err = http_client_perform(client);

    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTP chunk encoding Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

static void http_perform_as_stream_reader()
{
    char *buffer = aos_malloc(MAX_HTTP_RECV_BUFFER + 1);
    if (buffer == NULL)
    {
        LOGE(TAG, "Cannot aos_malloc http receive buffer");
        return;
    }
    http_client_config_t config = {
        .url = "http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err;
    if ((err = http_client_open(client, 0)) != HTTP_CLI_OK)
    {
        LOGE(TAG, "Failed to open HTTP connection: 0x%x @#@@@@@@", (err));
        aos_free(buffer);
        return;
    }
    int content_length = http_client_fetch_headers(client);
    int total_read_len = 0, read_len;
    if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER)
    {
        read_len = http_client_read(client, buffer, content_length);
        if (read_len <= 0)
        {
            LOGE(TAG, "Error read data");
        }
        buffer[read_len] = 0;
        LOGD(TAG, "read_len = %d", read_len);
    }
    LOGI(TAG, "HTTP Stream reader Status = %d, content_length = %d \r\n",
         http_client_get_status_code(client),
         http_client_get_content_length(client));
    http_client_close(client);
    http_client_cleanup(client);
    aos_free(buffer);
}

static void https_async()
{
    http_client_config_t config = {
        .url = "https://postman-echo.com/post",
        .event_handler = _http_event_handler,
        .is_async = true,
        .timeout_ms = 5000,
    };
    http_client_handle_t client = http_client_init(&config);
    http_errors_t err;
    const char *post_data = "Using a Palantír requires a person with great strength of will and wisdom. The Palantíri were meant to "
                            "be used by the Dúnedain to communicate throughout the Realms in Exile. During the War of the Ring, "
                            "the Palantíri were used by many individuals. Sauron used the Ithil-stone to take advantage of the users "
                            "of the other two stones, the Orthanc-stone and Anor-stone, but was also susceptible to deception himself.";
    http_client_set_method(client, HTTP_METHOD_POST);
    http_client_set_post_field(client, post_data, strlen(post_data));
    while (1)
    {
        err = http_client_perform(client);
        if (err != EAGAIN)
        {
            break;
        }
    }
    if (err == HTTP_CLI_OK)
    {
        LOGI(TAG, "HTTPS Status = %d, content_length = %d \r\n",
             http_client_get_status_code(client),
             http_client_get_content_length(client));
    }
    else
    {
        LOGE(TAG, "Error perform http request 0x%x @#@@@@@@", (err));
        e_count++;
    }
    http_client_cleanup(client);
}

void test_https(void)
{
    ca_crt_rsa[sizeof(ca_crt_rsa) - 1] = 0;
    http_rest_with_url();
    http_rest_with_hostname_path();
    http_auth_basic();
    http_auth_basic_redirect();
    http_auth_digest();
    http_relative_redirect();
    http_absolute_redirect();
    https_with_url();
    https_with_hostname_path();
    http_redirect_to_https();
    http_download_chunk();
    http_perform_as_stream_reader();
    https_async();

    LOGI(TAG, "Finish http example [%d]", e_count);
}

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

static void cmd_http_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "all") == 0)
    {
        test_https();
    }
    else if (argc == 2 && strcmp(argv[1], "post") == 0)
    {
        test_post();
    }
    else if (argc == 2 && strcmp(argv[1], "post2") == 0)
    {
        test_post2();
    }
    else
    {
        printf("\thttp_test all\n");
        printf("\thttp_test post\n");
    }
}

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

int cli_reg_cmd_player(void)
{
    char url[128];
    static const struct cli_command cmd_info = {
        "player",
        "player example",
        cmd_ipc_func,
    };

    static const struct cli_command http_cmd_info = {
        "http_test",
        "http_test all",
        cmd_http_func,
    };

    aos_cli_register_command(&cmd_info);
    aos_cli_register_command(&http_cmd_info);

    // 初始化后默认播放欢迎语句
    snprintf(url, sizeof(url), "mem://addr=%u&size=%u", (uint32_t)&_welcome_mp3, _welcome_mp3_len);
    // player_play(get_player_demo(), url, 0);

    return 0;
}
