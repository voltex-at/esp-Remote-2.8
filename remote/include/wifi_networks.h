#pragma once
#include "app_state.h"
#include <string.h>
// Keep the strongest AP per SSID/security, then the 16 strongest entries.
// Different security modes remain separate, so an open AP cannot hide a secured AP.
inline void addWifiNetwork(WifiNetwork*list,uint8_t&count,const WifiNetwork&candidate){
 if(!candidate.ssid[0])return; // Hidden SSIDs are entered manually.
 int index=-1;
 for(int i=0;i<count;i++)if(!strcmp(list[i].ssid,candidate.ssid)&&list[i].security==candidate.security){index=i;break;}
 if(index>=0){if(list[index].rssi>=candidate.rssi)return;list[index]=candidate;}
 else if(count<WIFI_NETWORK_COUNT){index=count++;list[index]=candidate;}
 else {index=count-1;if(candidate.rssi<=list[index].rssi)return;list[index]=candidate;}
 while(index>0&&list[index].rssi>list[index-1].rssi){auto previous=list[index-1];list[index-1]=list[index];list[index]=previous;--index;}
}
