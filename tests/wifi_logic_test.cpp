#include "wifi_networks.h"
#include <assert.h>
#include <stdio.h>
extern "C" int __lsan_is_turned_off(){return 1;} // Executor has no /proc thread access.
int main(){WifiNetwork list[WIFI_NETWORK_COUNT];uint8_t count=0;
 WifiNetwork n;addWifiNetwork(list,count,n);assert(count==0);
 snprintf(n.ssid,sizeof n.ssid,"Home");n.security=WifiSecurity::Wpa2;n.rssi=-75;addWifiNetwork(list,count,n);
 n.rssi=-85;addWifiNetwork(list,count,n);assert(count==1&&list[0].rssi==-75);
 n.rssi=-40;n.channel=11;addWifiNetwork(list,count,n);assert(count==1&&list[0].rssi==-40&&list[0].channel==11);
 n.security=WifiSecurity::Open;n.rssi=-30;addWifiNetwork(list,count,n);assert(count==2&&list[1].security==WifiSecurity::Wpa2);
 for(int i=0;i<40;i++){snprintf(n.ssid,sizeof n.ssid,"AP-%02d",i);n.rssi=-100+i;addWifiNetwork(list,count,n);}
 assert(count==WIFI_NETWORK_COUNT);for(int i=1;i<count;i++)assert(list[i-1].rssi>=list[i].rssi);
 assert(!strcmp(list[0].ssid,"Home")&&list[0].security==WifiSecurity::Open);
 assert(list[count-1].rssi==-74);printf("Wi-Fi: deduplication, security separation, sorting and capacity OK.\n");
}
