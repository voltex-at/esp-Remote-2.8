#include "runtime.h"
#include <time.h>
namespace Storage {
static bool mounted=false;static uint32_t sequence=0;
bool begin(){SD_MMC.setPins(Pins::sdClk,Pins::sdCmd,Pins::sdD0,Pins::sdD1,Pins::sdD2,Pins::sdD3);mounted=SD_MMC.begin("/sdcard",false,false,20000);if(mounted){SD_MMC.mkdir("/babyphone");SD_MMC.mkdir("/logs");}return mounted;}
bool available(){if(!mounted)return false;uint64_t total=SD_MMC.totalBytes(),used=SD_MMC.usedBytes();return total>used&&total-used>SD_RESERVE_BYTES;}
String newPath(const char*dir,const char*ext){
 struct tm t;char name[80];if(getLocalTime(&t,0)){strftime(name,sizeof name,"%Y%m%d_%H%M%S",&t);}else{snprintf(name,sizeof name,"boot_%08lx_%08lx",(unsigned long)esp_random(),(unsigned long)millis());}
 return String(dir)+"/"+name+"_"+String(sequence++)+ext;
}
void logSample(const AppState&s){
 if(!s.settings.logging||!mounted)return;Guard g(sdMutex);if(!available()){message("SD: brak miejsca lub karta wyjeta");return;}
 struct tm t;char date[20]="uptime";if(getLocalTime(&t,0))strftime(date,sizeof date,"%Y%m%d",&t);
 String path=String("/logs/sensors_")+date+".csv";bool header=!SD_MMC.exists(path);File f=SD_MMC.open(path,FILE_APPEND);if(!f){message("SD: blad zapisu logu");return;}
 if(header)f.println("epoch,uptime_ms,temp_C,rh_pct,co2_ppm,iaq,iaq_accuracy,pressure_hPa,eco2_ppm,voc_ppm,bme_fresh,scd_fresh,battery_V,th_source");
 f.printf("%lld,%lu,%.2f,%.2f,%.0f,%.1f,%u,%.2f,%.0f,%.3f,%d,%d,%.3f,%s\n",(long long)time(nullptr),(unsigned long)millis(),metricValue(s,0),metricValue(s,1),s.co2,s.iaq,s.iaqAccuracy,s.pressure,s.eco2,s.voc,s.bmeFresh,s.scdFresh,s.batteryV,s.bmeFresh?"BME688":s.scdFresh?"SCD41":"none");f.close();
}
void logEvent(const char*text){if(!mounted)return;Guard g(sdMutex);if(!available())return;File f=SD_MMC.open("/logs/events.csv",FILE_APPEND);if(f){String safe=text;safe.replace("\"","'");f.printf("%lld,%lu,\"%s\"\n",(long long)time(nullptr),(unsigned long)millis(),safe.c_str());f.close();}}
void files(){char names[FILE_COUNT][96]={};int n=0;{Guard g(sdMutex);if(mounted){File dir=SD_MMC.open("/babyphone");for(File f=dir.openNextFile();f;f=dir.openNextFile()){String p=f.path();if(!f.isDirectory()&&(p.endsWith(".rvc")||p.endsWith(".jpg"))){if(n<FILE_COUNT)strlcpy(names[n++],p.c_str(),96);else{for(int i=1;i<FILE_COUNT;i++)strlcpy(names[i-1],names[i],96);strlcpy(names[FILE_COUNT-1],p.c_str(),96);}}f.close();}dir.close();}}
 Guard g(stateMutex);state.fileCount=n;for(int i=0;i<n;i++)strlcpy(state.files[i],names[n-1-i],96);
}
}
