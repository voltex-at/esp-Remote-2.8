#pragma once
// Profile: AI Thinker ESP32-CAM with OV2640. Change camera pins for other boards.
#define CAM_PWDN 32
#define CAM_RESET -1
#define CAM_XCLK 0
#define CAM_SDA 26
#define CAM_SCL 27
#define CAM_D0 5
#define CAM_D1 18
#define CAM_D2 19
#define CAM_D3 21
#define CAM_D4 36
#define CAM_D5 39
#define CAM_D6 34
#define CAM_D7 35
#define CAM_VSYNC 25
#define CAM_HREF 23
#define CAM_PCLK 22
// Direct link always available. Set these for the same 2.4GHz router as remote.
#define CAM_ROUTER_SSID ""
#define CAM_ROUTER_PASSWORD ""
#define CAM_DIRECT_SSID "ESP32-BABY-CAM"
#define CAM_DIRECT_PASSWORD "ChangeMe-Cam-2026"
// Shared pairing token, also entered in remote Settings > Connection.
#define CAM_PAIR_TOKEN "change-this-pair-token"
// Classic AI Thinker has no microphone or speaker. Audio needs external modules.
// These example pins replace its SD/flash use. Do not connect camera-side SD.
#ifndef CAM_AUDIO_ENABLED
#define CAM_AUDIO_ENABLED 0
#endif
#define CAM_AUDIO_BCLK 14
#define CAM_AUDIO_LRCLK 15
#define CAM_AUDIO_MIC 4
#define CAM_AUDIO_SPEAKER 13
// I2S1, INMP441/ICS-43434 microphone + MAX98357A amplifier. Half duplex PTT.
#define CAM_MIC_SHIFT 14
#define CAM_NOISE_THRESHOLD 1800
// No IR LED on AI Thinker: grayscale/exposure mode only. Optional IR enable pin.
#define CAM_IR_PIN -1
