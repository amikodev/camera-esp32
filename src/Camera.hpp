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

#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include <stdio.h>
#include <string.h>
#include <math.h>


#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_http_server.h"


#include "esp_camera.h"

class Camera{

private:


public:

    /**
     * Инициализация камеры
     */
    static esp_err_t init();

    /**
     * Захват изображения
     */
    static esp_err_t capture();

    /**
     * Запуск потокового вывода (stream)
     */
    static esp_err_t streamHandler(httpd_req_t* req);


};

#endif      // __CAMERA_HPP__
