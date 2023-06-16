//
// Created by Hugo Trippaers on 20/04/2021.
//

#ifndef ESPCAM_ESP_RTSP_COMMON_H
#define ESPCAM_ESP_RTSP_COMMON_H


typedef struct {
    void *buffer;
    size_t len;
} espcam_binary_data_t;


typedef struct {
    unsigned char *essid;
    unsigned char *essid_secret;
} espcam_wifi_config_t;

typedef struct {
    espcam_wifi_config_t wifi_config; 
} app_config_t;

esp_err_t esp32cam_camera_init();
esp_err_t esp32cam_camera_capture(esp_err_t(*handler)(uint8_t *fb, size_t fb_len));
esp_err_t esp32cam_wifi_init(espcam_wifi_config_t *wifi_config); 
esp_err_t esp32cam_sntp_init();
#endif //ESPCAM_ESP_RTSP_COMMON_H