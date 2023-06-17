#message(FATAL_ERROR "AAAA: ${CMAKE_CURRENT_SOURCE_DIR}/src/camera_pins.h")

set(COMPONENT_SRCS "esp-rtsp.c" "rtsp-server.c" "rtsp-parser.c" "rtp-udp.c" "jpeg.c" "h264.c" "rtp.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_PRIV_INCLUDEDIRS "priv")

set(COMPONENT_REQUIRES lwip esp32-camera esp_h264)
set(COMPONENT_PRIV_REQUIRES freertos nvs_flash esp_timer esp_netif)

register_component()