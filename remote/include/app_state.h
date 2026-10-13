#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "ota_policy.h"
#include "theme_settings.h"
#include "battery_monitor.h"
#include "sensor_registry.h"
#include "signal_led.h"
enum class Screen:uint8_t { Home,Cameras,Live,Sensors,Metric,Settings,General,Display,Network,Pair,Alerts,Audio,CameraSettings,SensorSettings,Scan,Recordings,Battery,System,Limits,WifiNetworks,Languages,Update,Colors,Appearance,AudioSettings,WifiSettings,Bluetooth,Direct,AddSensor,SensorValue,Led,LedColor,LedSensor };
enum class Action:uint8_t { Setting,Scan,Connect,Snapshot,Record,Listen,Talk,CameraControl,LoadFiles,Play,Reset,ClearAlerts,Reboot,WifiScan,OtaCheck,OtaInstall,BleScan,SensorAdd,SensorSelect,SensorRemove,LedSensor };
enum class WifiScanState:uint8_t { Idle,Scanning,Done,Failed };
enum class WifiLinkState:uint8_t { Offline,Connecting,Connected,Failed };
enum class BleState:uint8_t { Off,Scanning,Done,Reading,Error };
enum class BleError:uint8_t { None,Memory,Start,Read,Unsupported,Encrypted,Full,Save };
struct BleSensor { SensorRegistry::Config config;int16_t rssi=-127;SensorRegistry::Reading reading;uint32_t updated[SensorRegistry::Measurements]={};bool encrypted=false; };
enum class WifiSecurity:uint8_t { Open,Wpa,Wpa2,Wpa3,WpaWpa2,Wpa2Wpa3,Unsupported };
struct WifiNetwork { char ssid[33]={};int16_t rssi=-127;WifiSecurity security=WifiSecurity::Open;uint8_t channel=0; };
enum SettingKey:uint8_t { Brightness,Timeout,Language,LogInterval,Logging,Notifications,Volume,NetworkMode,WifiSsid,WifiPassword,CameraHost,PairToken,TempMin,TempMax,HumidityMin,HumidityMax,Co2Max,IaqMax,Appearance,LedMode,LedBrightness,LedColor };
struct Settings {
 uint8_t brightness=80,language=0,volume=65,networkMode=0;
 uint16_t timeoutSec=60,logIntervalSec=10;
 bool logging=true,notifications=true;
 float tempMin=16,tempMax=28,humidityMin=30,humidityMax=70,co2Max=1400,iaqMax=150;
 // User configures the network and camera token; firmware has no credentials.
 // The POD layout and existing NVS settings remain compatible with V1.2.
 char ssid[33]={},password[65]={};
 char cameraHost[64]="192.168.4.1",pairToken[65]={};
};
// NVS schema1 stores this exact layout. A new layout needs a real migration.
static_assert(sizeof(Settings)==264&&offsetof(Settings,ssid)==36&&offsetof(Settings,password)==69&&offsetof(Settings,cameraHost)==134&&offsetof(Settings,pairToken)==198,"Preserve Settings NVS schema1 layout");
constexpr int METRIC_COUNT=6,HISTORY_COUNT=60,FILE_COUNT=12,ALERT_COUNT=8,WIFI_NETWORK_COUNT=16;
struct AppState {
 Settings settings;
 SignalLed::Config led;
 uint32_t ledOutput=0;
 ThemeMode theme=ThemeMode::Dark;
 BleState bleState=BleState::Off;BleError bleError=BleError::None;
 SensorRegistry::Device sensors[SensorRegistry::Capacity];uint8_t sensorCount=0,lastAdded=0;
 uint32_t sensorGeneration=0,sensorAddedGeneration=0,bleGeneration=0,nowMs=0;
 BleSensor bleDevices[12];uint8_t bleCount=0;
 Ota::Status ota;
 float temperature=NAN,humidity=NAN,co2=NAN,iaq=NAN,pressure=NAN,eco2=NAN,voc=NAN;
 float scdTemperature=NAN,scdHumidity=NAN;
 float batteryV=0,audioRms=0;
 Battery::Power batteryPower=Battery::Power::Battery;
 int batteryPct=-1,rssi=-127,bsecStatus=0,bmeStatus=0;
 uint8_t iaqAccuracy=0;
 bool bmeOnline=false,scdOnline=false,bmeFresh=false,scdFresh=false,wifi=false,sd=false;
 bool camera=false,audioCap=false,motionCap=false,night=false,motionEnabled=false,noiseEnabled=false;
 bool recording=false,listening=false,talking=false,playing=false,scanRunning=false;
 uint8_t found[128]={0},sensorAddress=0;
 uint32_t sampleMs=0,bmeMs=0,scdMs=0,frames=0,clipMs=0,uptime=0;
 uint64_t sdFree=0;
 uint16_t historyCount=0;
 float history[METRIC_COUNT][HISTORY_COUNT];
 uint32_t clipStarted=0;
 char clipPath[96]={};
 char timeText[8]="--:--",ip[20]="0.0.0.0",message[120]={},cameraName[32]="Kamera 1";
 char files[FILE_COUNT][96]={},alerts[ALERT_COUNT][96]={};
 uint8_t fileCount=0,alertCount=0;
 WifiScanState wifiScan=WifiScanState::Idle;
 WifiLinkState wifiLink=WifiLinkState::Offline;
 WifiNetwork networks[WIFI_NETWORK_COUNT];
 uint8_t networkCount=0;
 uint32_t wifiScanGeneration=0;
 char connectedSsid[33]={};
};
struct Command { Action action;int value;char text[128]; };
using Dispatch=void(*)(Action,int,const char*);
inline float metricValue(const AppState&s,int i){switch(i){case 0:return s.bmeFresh?s.temperature:s.scdTemperature;case 1:return s.bmeFresh?s.humidity:s.scdHumidity;case 2:return s.co2;case 3:return s.iaq;case 4:return s.pressure;default:return s.eco2;}}
inline bool metricValid(const AppState&s,int i){return i<2?(s.bmeFresh||s.scdFresh):i==2?s.scdFresh:s.bmeFresh;}
