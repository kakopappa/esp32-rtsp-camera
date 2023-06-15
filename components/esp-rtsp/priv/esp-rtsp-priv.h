//
// Created by Hugo Trippaers on 21/05/2021.
//

#ifndef ESPCAM_ESP_RTSP_PRIV_H
#define ESPCAM_ESP_RTSP_PRIV_H

#define SERVER_STACKSIZE (16 * 1024)
#define SERVER_PRIORITY 2

typedef struct {
    bool running;
    TaskHandle_t server_taskhandle;
} esp_rtsp_server_t;



#endif //ESPCAM_ESP_RTSP_PRIV_H
