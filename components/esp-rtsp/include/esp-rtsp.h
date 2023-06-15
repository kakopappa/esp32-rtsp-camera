//
// Created by Hugo Trippaers on 17/05/2021.
//

#ifndef ESPCAM_ESP_RTSP_H
#define ESPCAM_ESP_RTSP_H

typedef void* esp_rtsp_server_handle_t;

esp_err_t esp_rtsp_server_start(esp_rtsp_server_handle_t *handle);
esp_err_t esp_rtsp_server_stop(esp_rtsp_server_handle_t handle);

#endif //ESPCAM_ESP_RTSP_H