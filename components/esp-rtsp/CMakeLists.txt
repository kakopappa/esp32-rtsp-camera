set(COMPONENT_SRCS "esp-rtsp.c" "rtsp-server.c" "rtsp-parser.c" "rtp-udp.c" "jpeg.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_PRIV_INCLUDEDIRS "priv")

set(COMPONENT_REQUIRES lwip esp32-camera esp_h264)
set(COMPONENT_PRIV_REQUIRES freertos nvs_flash)

register_component()
