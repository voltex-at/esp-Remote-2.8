#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace Ota {
constexpr char REPOSITORY[]="voltex-at/esp-Remote-2.8";
constexpr char TARGET[]="espremote-es3c28p-16mb-portrait";
constexpr char RELEASE_PREFIX[]="https://github.com/voltex-at/esp-Remote-2.8/releases/";
constexpr char MANIFEST[]="https://github.com/voltex-at/esp-Remote-2.8/releases/latest/download/ota.json";
constexpr char ASSET_PREFIX[]="https://release-assets.githubusercontent.com/";
inline bool versionParts(const char*s,uint32_t v[3]){
 if(!s)return false;for(int i=0;i<3;i++){if(*s<'0'||*s>'9')return false;uint32_t n=0;do{n=n*10+(*s++-'0');if(n>65535)return false;}while(*s>='0'&&*s<='9');v[i]=n;if(i<2){if(*s++!='.')return false;}else if(*s)return false;}return true;
}
inline bool newer(const char*a,const char*b){uint32_t x[3],y[3];if(!versionParts(a,x)||!versionParts(b,y))return false;for(int i=0;i<3;i++){if(x[i]!=y[i])return x[i]>y[i];}return false;}
inline bool validHash(const char*s){if(!s||strlen(s)!=64)return false;for(int i=0;i<64;i++)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;return true;}
inline bool allowedUrl(const char*s){if(!s)return false;return !strncmp(s,RELEASE_PREFIX,sizeof(RELEASE_PREFIX)-1)||!strncmp(s,ASSET_PREFIX,sizeof(ASSET_PREFIX)-1);}
inline bool appHeader(const uint8_t*p,size_t n){return n>=36&&p[0]==0xe9&&p[12]==9&&p[13]==0&&p[32]==0x32&&p[33]==0x54&&p[34]==0xcd&&p[35]==0xab;}
enum class State:uint8_t { Idle,Checking,Current,Available,Downloading,Verifying,Restarting,Error };
enum class Error:uint8_t { None,Network,Clock,Http,Manifest,Memory,Partition,Expired,Download,Checksum,Image,Write,Busy,Battery,Dns,Connection,Tls,NotFound,RateLimit,Redirect };
struct Status {State state=State::Idle;Error error=Error::None;uint8_t progress=0;int32_t detail=0;char version[24]={};char notes[3][192]={};};
inline bool installing(State s){return s==State::Downloading||s==State::Verifying||s==State::Restarting;}
}
