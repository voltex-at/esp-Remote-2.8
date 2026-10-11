#include "ota_policy.h"
#include <cassert>
#include <string>
#include <cstdio>
int main(){
 uint32_t v[3];for(auto*s:{"","1","1.2","v1.3.0","1.2.3.4","-1.2.3","65536.0.0","999999999999.1.2","1.2.3-beta","1.2.3\n"})assert(!Ota::versionParts(s,v));
 assert(!Ota::versionParts(nullptr,v));assert(Ota::versionParts("1.3.0",v)&&v[1]==3);
 assert(Ota::newer("1.10.0","1.9.9"));assert(!Ota::newer("1.3.0","1.3.0"));assert(!Ota::newer("1.2.9","1.3.0"));assert(!Ota::newer("invalid","1.3.0"));
 assert(Ota::validHash(std::string(64,'a').c_str()));for(auto s:{std::string(63,'a'),std::string(65,'a'),std::string(64,'g'),std::string(64,'A')})assert(!Ota::validHash(s.c_str()));
 assert(Ota::allowedUrl(Ota::MANIFEST));assert(Ota::allowedUrl("https://release-assets.githubusercontent.com/file?token=x"));
 for(auto*s:{"http://github.com/voltex-at/esp-Remote-2.8/releases/a","https://github.com.evil/voltex-at/esp-Remote-2.8/releases/a","https://github.com/other/esp-Remote-2.8/releases/a","https://release-assets.githubusercontent.com.evil/a","https://release-assets.githubusercontent.com@evil/a","https://release-assets.githubusercontent.com:443/a","https://release-assets.githubusercontent.com","file:///firmware.bin"})assert(!Ota::allowedUrl(s));
 uint8_t header[36]={0xe9};header[12]=9;header[32]=0x32;header[33]=0x54;header[34]=0xcd;header[35]=0xab;assert(Ota::appHeader(header,36));assert(!Ota::appHeader(header,35));header[12]=0;assert(!Ota::appHeader(header,36));header[12]=9;header[32]=0;assert(!Ota::appHeader(header,36));header[32]=0x32;header[0]=0;assert(!Ota::appHeader(header,36));
 puts("OTA policy: versions, URL boundaries, hash format and ESP32-S3 image target OK.");
}
