/*
amikodev/camera-esp32 - Camera support for ESP32-CAM module for industrial factory
Copyright © 2021 Prihodko Dmitriy - asketcnc@yandex.ru
*/

/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>

#include "esp_log.h"

#include "wifi.hpp"
#include "httprequest.hpp"
#include "httpresponce.hpp"

#include "Camera.hpp"


#define SERVER_PORT 8081


#define TAG "Camera_ESP32 main"

extern "C" {
    void app_main(void);
}


static httpd_handle_t httpServerInstance = NULL;
 

static httpd_uri_t streamCaptureUri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = Camera::streamHandler,
    .user_ctx  = NULL,
};
 
static void wifiConnect(){
    ESP_LOGI(TAG, "wifiConnect");

    // start http server
    httpd_config_t httpServerConfiguration = HTTPD_DEFAULT_CONFIG();
    httpServerConfiguration.server_port = SERVER_PORT;
    if(httpd_start(&httpServerInstance, &httpServerConfiguration) == ESP_OK){
        ESP_ERROR_CHECK(httpd_register_uri_handler(httpServerInstance, &streamCaptureUri));
    }
}

static void wifiDisconnect(){
    ESP_LOGI(TAG, "wifiDisconnect");

    // stop http server
    if(httpServerInstance != NULL){
        ESP_ERROR_CHECK(httpd_stop(httpServerInstance));
    }
}

void app_main() {
    ESP_LOGI(TAG, "Project Camera");

    Wifi wifi;
    wifi.setHostname((char *) "Camera-ESP32");
    wifi.setup();

    if(Camera::init() == ESP_OK){
        ESP_LOGI(TAG, "Camera init success");
        Camera::capture();

        wifi.apStaConnect(&wifiConnect);
        wifi.apStaDisconnect(&wifiDisconnect);
    }

}




