#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
namespace SensorRegistry {
constexpr uint8_t Capacity=4,Measurements=7,None=255;
enum class Source:uint8_t { Bme688,Scd41,Bthome,Ess };
struct Config {
 Source source=Source::Bme688;uint8_t addressType=0,capabilities=0,selected=None;
 char address[18]={},name[32]={};
};
struct Device {
 Config config;
 float values[Measurements]={NAN,NAN,NAN,NAN,NAN,NAN,NAN};
 uint32_t updated[Measurements]={};
 int16_t rssi=-127;
 float history[60]={};uint8_t historyCount=0;
};
struct Registry {Config devices[Capacity];uint8_t count=0;};
inline bool supported(const Config&c,uint8_t m){return m<Measurements&&(c.capabilities&(1u<<m));}
inline bool valid(const Config&c){if(unsigned(c.source)>unsigned(Source::Ess)||!c.capabilities||(c.capabilities&0x80)||c.name[31]||c.address[17]||(c.selected!=None&&!supported(c,c.selected)))return false;
 if(c.source==Source::Bthome||c.source==Source::Ess){if(c.addressType>3||strlen(c.address)!=17)return false;for(int i=0;i<17;i++){char v=c.address[i];if(i%3==2){if(v!=':')return false;}else if(!((v>='0'&&v<='9')||(v>='a'&&v<='f')||(v>='A'&&v<='F')))return false;}}return true;}
inline bool same(const Config&a,const Config&b){return a.source==b.source&&!strcmp(a.address,b.address);}
inline int add(Registry&r,const Config&c){if(!valid(c))return -1;for(int i=0;i<r.count;i++)if(same(r.devices[i],c))return i;if(r.count>=Capacity)return -1;r.devices[r.count]=c;return r.count++;}
inline bool valid(const Registry&r){if(r.count>Capacity)return false;for(int i=0;i<r.count;i++){if(!valid(r.devices[i]))return false;for(int j=0;j<i;j++)if(same(r.devices[i],r.devices[j]))return false;}return true;}
inline bool fresh(const Device&d,uint8_t m,uint32_t now){uint32_t ttl=d.config.source==Source::Bthome?180000:30000;return supported(d.config,m)&&d.updated[m]&&now-d.updated[m]<ttl&&isfinite(d.values[m]);}
struct Reading {uint8_t mask=0;float values[Measurements]={NAN,NAN,NAN,NAN,NAN,NAN,NAN};};
enum class Decode:uint8_t { Valid,Unsupported,Encrypted,Malformed };
inline uint32_t little(const uint8_t*p,size_t n){uint32_t v=0;for(size_t i=0;i<n;i++)v|=uint32_t(p[i])<<(8*i);return v;}
// UUID FCD2 service data excludes the UUID itself. Stop at unknown object IDs
// as required by BTHome; never guess a length or consume a partial object.
inline Decode bthome(const uint8_t*p,size_t n,Reading&out){
 Reading r;if(!p||n<1)return Decode::Malformed;if((p[0]>>5)!=2)return Decode::Unsupported;if(p[0]&1)return Decode::Encrypted;
 size_t i=1;while(i<n){uint8_t id=p[i++],bytes=0,m=None;float scale=1;bool signedValue=false;
 switch(id){case 0:bytes=1;break;case 1:bytes=1;m=6;break;case 2:bytes=2;m=0;scale=.01f;signedValue=true;break;case 3:bytes=2;m=1;scale=.01f;break;case 4:bytes=3;m=4;scale=.01f;break;case 0x12:bytes=2;m=2;break;case 0x2e:bytes=1;m=1;break;case 0x45:bytes=2;m=0;scale=.1f;signedValue=true;break;
 // Common non-displayed fields may occur before a supported measurement.
 case 5:case 0x0a:case 0x0b:bytes=3;break;
 case 6:case 7:case 8:case 0x0c:case 0x0d:case 0x0e:case 0x13:case 0x14:bytes=2;break;
 case 9:case 0x0f:case 0x10:case 0x11:case 0x15:case 0x16:case 0x17:case 0x18:case 0x19:case 0x1a:case 0x1b:case 0x1c:case 0x1d:case 0x1e:case 0x1f:case 0x20:case 0x21:case 0x22:case 0x23:case 0x24:case 0x25:case 0x26:case 0x27:case 0x28:case 0x29:case 0x2a:case 0x2b:case 0x2c:case 0x2d:case 0x2f:bytes=1;break;
 default:out=r;return r.mask?Decode::Valid:Decode::Unsupported;
 }
 if(n-i<bytes)return Decode::Malformed;
 if(m!=None){uint32_t raw=little(p+i,bytes);float v=signedValue?float(int16_t(raw)):float(raw);v*=scale;if((m==1||m==6)&&v>100)return Decode::Malformed;if(!(r.mask&(1u<<m)))r.values[m]=v;r.mask|=1u<<m;}i+=bytes;
 }out=r;return r.mask?Decode::Valid:Decode::Unsupported;
}
// Bluetooth SIG Environmental Sensing and Battery characteristics.
inline bool ess(uint16_t uuid,const uint8_t*p,size_t n,Reading&r){uint8_t m=None;size_t size=0;float scale=1;bool sign=false;
 switch(uuid){case 0x2a6e:m=0;size=2;scale=.01f;sign=true;break;case 0x2a6f:m=1;size=2;scale=.01f;break;case 0x2a6d:m=4;size=4;scale=.001f;break;case 0x2a19:m=6;size=1;break;default:return false;}
 if(!p||n!=size)return false;uint32_t raw=little(p,n);float v=(sign?float(int16_t(raw)):float(raw))*scale;if((m==1||m==6)&&v>100)return false;r.mask|=1u<<m;r.values[m]=v;return true;
}
}
