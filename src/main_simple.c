#include "string.h"
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"


//ESP32-CAM
#define CAM_PIN_PWDN    32 
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22


#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

#define SOFT_AP_SSID "ESP32 SoftAP"
#define SOFT_AP_PASSWORD "Password"
 
#define SOFT_AP_IP_ADDRESS_1 192
#define SOFT_AP_IP_ADDRESS_2 168
#define SOFT_AP_IP_ADDRESS_3 5
#define SOFT_AP_IP_ADDRESS_4 18
 
#define SOFT_AP_GW_ADDRESS_1 192
#define SOFT_AP_GW_ADDRESS_2 168
#define SOFT_AP_GW_ADDRESS_3 5
#define SOFT_AP_GW_ADDRESS_4 20
 
#define SOFT_AP_NM_ADDRESS_1 255
#define SOFT_AP_NM_ADDRESS_2 255
#define SOFT_AP_NM_ADDRESS_3 255
#define SOFT_AP_NM_ADDRESS_4 0
 
#define SERVER_PORT 8081
#define HTTP_METHOD HTTP_GET
#define URI_STRING "/capture.jpg"

#define TAG "MAIN"
 
static httpd_handle_t httpServerInstance = NULL;
 
static esp_err_t methodHandler(httpd_req_t* httpRequest){
    ESP_LOGI("HANDLER", "This is the handler for the <%s> URI", httpRequest->uri);
    return ESP_OK;
}

static esp_err_t camera_init(){

    camera_config_t camera_config = {
        .pin_pwdn  = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sscb_sda = CAM_PIN_SIOD,
        .pin_sscb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
        .xclk_freq_hz = 10000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
        .frame_size = FRAMESIZE_QVGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

        .jpeg_quality = 12, //0-63 lower number means higher quality
        .fb_count = 2 //if more than one, i2s runs in continuous mode. Use only with JPEG
    };


    // // ?????????????????? ???????????? ???????? ?????????? PWDN ??????????????????
    // if(CAM_PIN_PWDN != -1){
    //     gpio_pad_select_gpio(CAM_PIN_PWDN);
    //     gpio_set_direction((gpio_num_t)CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
    //     gpio_set_level((gpio_num_t)CAM_PIN_PWDN, 0);
    //     // pinMode(CAM_PIN_PWDN, OUTPUT);
    //     // digitalWrite(CAM_PIN_PWDN, LOW);
    // }

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

static esp_err_t jpg_stream_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}
 
static httpd_uri_t streamCaptureUri = {
    .uri       = URI_STRING,
    .method    = HTTP_METHOD,
    // .handler   = methodHandler,
    .handler   = jpg_stream_httpd_handler,
    .user_ctx  = NULL,
};
 
static void startHttpServer(void){
    httpd_config_t httpServerConfiguration = HTTPD_DEFAULT_CONFIG();
    httpServerConfiguration.server_port = SERVER_PORT;
    if(httpd_start(&httpServerInstance, &httpServerConfiguration) == ESP_OK){
        ESP_ERROR_CHECK(httpd_register_uri_handler(httpServerInstance, &streamCaptureUri));
    }
}
 
static void stopHttpServer(void){
    if(httpServerInstance != NULL){
        ESP_ERROR_CHECK(httpd_stop(httpServerInstance));
    }
}
 
static esp_err_t wifiEventHandler(void* userParameter, system_event_t *event) {
    switch(event->event_id){
    case SYSTEM_EVENT_AP_STACONNECTED:
        startHttpServer();
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        stopHttpServer();
        break;
    default:
        break;
    }
    return ESP_OK;
}
 
static void launchSoftAp(){
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    tcpip_adapter_ip_info_t ipAddressInfo;
    memset(&ipAddressInfo, 0, sizeof(ipAddressInfo));
    IP4_ADDR(
        &ipAddressInfo.ip,
        SOFT_AP_IP_ADDRESS_1,
        SOFT_AP_IP_ADDRESS_2,
        SOFT_AP_IP_ADDRESS_3,
        SOFT_AP_IP_ADDRESS_4);
    IP4_ADDR(
        &ipAddressInfo.gw,
        SOFT_AP_GW_ADDRESS_1,
        SOFT_AP_GW_ADDRESS_2,
        SOFT_AP_GW_ADDRESS_3,
        SOFT_AP_GW_ADDRESS_4);
    IP4_ADDR(
        &ipAddressInfo.netmask,
        SOFT_AP_NM_ADDRESS_1,
        SOFT_AP_NM_ADDRESS_2,
        SOFT_AP_NM_ADDRESS_3,
        SOFT_AP_NM_ADDRESS_4);
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ipAddressInfo));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));
    wifi_init_config_t wifiConfiguration = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiConfiguration));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_config_t apConfiguration = {
        .ap = {
            .ssid = SOFT_AP_SSID,
            .password = SOFT_AP_PASSWORD,
            .ssid_len = 0,
            //.channel = default,
            .channel = 0,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = 4,
            .beacon_interval = 150,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apConfiguration));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_start());
}
 
void app_main(void){
    ESP_ERROR_CHECK(camera_init());
    launchSoftAp();
    while(1) vTaskDelay(10);
}