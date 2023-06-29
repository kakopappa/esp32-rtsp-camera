# RTSP Camera for ESP32

Tested on Freenove Camera ESP32-S3 WROOM N8R8 (8MB Flash / 8MB PSRAM)

How to use.

1. git clone this repository.
2. Open with VSCode.
3. Change WIFI_NAME, WIFI_PASSWORD in the main.c
4. Run `pio run -t menuconfig` -> `Component Config` -> `ESP PSRAM, Enable Support for external SPI` then select `SPI RAM config` and select the correct mode for your board. (Octal Mode for Freenove Camera ESP32-S3)

![image](https://github.com/kakopappa/esp32-rtsp-camera/assets/7474406/1b28e9ff-e88a-43b4-9204-fd9983802869)
   
6. PIO Upload.

Limitations:
1. Supports RTSP via UDP only

Credit:
1. Hugo Trippaers - for initial version. https://github.com/spark404/esp32-cam/
2. Boris - for Freenove ESP32 S3 board definition for PIO. https://github.com/sivar2311/freenove-esp32-s3-platformio

esp32 camera with h264 support is in the h264 branch
