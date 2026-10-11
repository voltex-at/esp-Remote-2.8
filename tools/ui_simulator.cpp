// Actual LVGL software renderer. Demo values exist only in this desktop preview.
#include "ui.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <initializer_list>
extern "C" int __lsan_is_turned_off(){return 1;} // restricted executor has no /proc thread access
static void flush(lv_disp_drv_t*d,const lv_area_t*,lv_color_t*){lv_disp_flush_ready(d);}
static Command sent[32];static int sentCount=0;static bool capture=false;
static void command(Action a,int value,const char*t){if(capture){assert(sentCount<32);sent[sentCount]={a,value,{0}};snprintf(sent[sentCount++].text,128,"%s",t?t:"");}}
static lv_obj_t*findLabel(lv_obj_t*parent,const char*text){if(lv_obj_check_type(parent,&lv_label_class)&&!strcmp(lv_label_get_text(parent),text))return parent;for(uint32_t i=0;i<lv_obj_get_child_cnt(parent);i++)if(auto*o=findLabel(lv_obj_get_child(parent,i),text))return o;return nullptr;}
static lv_obj_t*findType(lv_obj_t*parent,const lv_obj_class_t*type){if(lv_obj_check_type(parent,type))return parent;for(uint32_t i=0;i<lv_obj_get_child_cnt(parent);i++)if(auto*o=findType(lv_obj_get_child(parent,i),type))return o;return nullptr;}
static void dumpLabels(lv_obj_t*p){if(lv_obj_check_type(p,&lv_label_class))fprintf(stderr,"Label: %s\n",lv_label_get_text(p));for(uint32_t i=0;i<lv_obj_get_child_cnt(p);i++)dumpLabels(lv_obj_get_child(p,i));}
static void click(const char*text){lv_obj_update_layout(lv_scr_act());auto*l=findLabel(lv_scr_act(),text);if(!l)fprintf(stderr,"Missing UI target: %s\n",text);assert(l);lv_event_send(lv_obj_get_parent(l),LV_EVENT_CLICKED,nullptr);}
static void pressKey(lv_obj_t*kb,const char*key){for(uint16_t i=0;i<100;i++){const char*t=lv_btnmatrix_get_btn_text(kb,i);if(!t)break;if(!strcmp(t,key)){lv_btnmatrix_set_selected_btn(kb,i);lv_event_send(kb,LV_EVENT_VALUE_CHANGED,nullptr);return;}}assert(false);}
static void save(const char*name){lv_obj_update_layout(lv_scr_act());lv_refr_now(nullptr);auto*s=lv_snapshot_take(lv_scr_act(),LV_IMG_CF_TRUE_COLOR);if(!s){fprintf(stderr,"snapshot failed %s\n",name);exit(1);}FILE*f=fopen(name,"wb");fprintf(f,"P6\n240 320\n255\n");auto*p=(lv_color_t*)s->data;for(int i=0;i<240*320;i++){lv_color32_t c; c.full=lv_color_to32(p[i]);fputc(c.ch.red,f);fputc(c.ch.green,f);fputc(c.ch.blue,f);}fclose(f);lv_snapshot_free(s);}
int main(int argc,char**argv){if(argc!=2)return 1;lv_init();static lv_color_t buf[240*20];static lv_disp_draw_buf_t b;lv_disp_draw_buf_init(&b,buf,nullptr,240*20);static lv_disp_drv_t d;lv_disp_drv_init(&d);d.hor_res=240;d.ver_res=320;d.flush_cb=flush;d.draw_buf=&b;lv_disp_drv_register(&d);Ui::begin(command);
 AppState s;assert(!s.settings.ssid[0]&&!s.settings.password[0]&&!s.settings.pairToken[0]);Ui::show(Screen::Network);Ui::update(s);lv_obj_update_layout(lv_scr_act());assert(findLabel(lv_scr_act(),"Nie ustawiono"));
 s.batteryPct=80;s.batteryV=3.98;s.wifi=true;s.rssi=-54;s.wifiLink=WifiLinkState::Connected;strcpy(s.connectedSsid,"ESP32-BABY-CAM");strcpy(s.settings.ssid,s.connectedSsid);s.wifiScan=WifiScanState::Done;s.wifiScanGeneration=1;s.networkCount=5;
 const char*ssids[]={"ESP32-BABY-CAM","Home-24G","Guest","Office-Enterprise","A-very-long-SSID-with-32-letters"};for(int i=0;i<5;i++){snprintf(s.networks[i].ssid,33,"%s",ssids[i]);s.networks[i].rssi=-42-i*10;s.networks[i].security=i==2?WifiSecurity::Open:i==3?WifiSecurity::Unsupported:WifiSecurity::Wpa2;}
 s.bmeOnline=s.scdOnline=s.bmeFresh=s.scdFresh=true;s.temperature=23.4;s.humidity=54;s.co2=742;s.iaq=42;s.pressure=1013.2;s.eco2=608;s.iaqAccuracy=2;s.historyCount=60;s.sd=true;s.sdFree=12ULL*1024*1024*1024;strcpy(s.timeText,"14:26");strcpy(s.ip,"192.168.4.2");strcpy(s.message,"");
 for(int m=0;m<6;m++)for(int i=0;i<60;i++)s.history[m][i]=metricValue(s,m)+sin(i*.34f)*(m==2?110:m==1?7:m==3?15:2);
 Ui::update(s);const Screen screens[]={Screen::Home,Screen::Sensors,Screen::Metric,Screen::Settings,Screen::General,Screen::Display,Screen::CameraSettings,Screen::Network,Screen::Scan,Screen::Battery,Screen::System,Screen::Alerts,Screen::Cameras,Screen::Audio,Screen::Live,Screen::Limits,Screen::SensorSettings,Screen::Recordings,Screen::Pair,Screen::WifiNetworks,Screen::Languages,Screen::Colors};
 for(int lang=0;lang<3;lang++){s.settings.language=lang;Ui::update(s);for(unsigned i=0;i<sizeof screens/sizeof screens[0];i++){Ui::show(screens[i],0);Ui::update(s);lv_tick_inc(30);lv_timer_handler();char name[512];snprintf(name,sizeof name,"%s/ui_%d_%02d.ppm",argv[1],lang,i);save(name);}
  const char*settingsNames[]={"Ustawienia","Einstellungen","Settings"};const char*temperatureNames[]={"Temperatura","Temperatur","Temperature"};const char*dimNames[]={"Przygaszanie [s]","Dimmen [s]","Dim after [s]"};
  Ui::show(Screen::Home);Ui::update(s);lv_obj_update_layout(lv_scr_act());assert(findLabel(lv_scr_act(),settingsNames[lang]));
  Ui::show(Screen::Sensors);Ui::update(s);lv_obj_update_layout(lv_scr_act());auto*temperature=findLabel(lv_scr_act(),temperatureNames[lang]);assert(temperature);assert(lv_obj_get_height(temperature)<=lv_obj_get_style_text_font(temperature,LV_PART_MAIN)->line_height);
  Ui::show(Screen::Display);Ui::update(s);lv_obj_update_layout(lv_scr_act());auto*dim=findLabel(lv_scr_act(),dimNames[lang]);auto*value=findLabel(lv_scr_act(),"60");assert(dim&&value);lv_area_t a,b;lv_obj_get_coords(dim,&a);lv_obj_get_coords(value,&b);assert(a.y2<b.y1);
  Ui::show(Screen::WifiNetworks);Ui::update(s);click("Home-24G");auto*input=findType(lv_scr_act(),&lv_textarea_class);assert(input&&lv_textarea_get_password_mode(input));lv_textarea_set_text(input,"Example-Password");char name[512];snprintf(name,sizeof name,"%s/wifi_password_%d.ppm",argv[1],lang);save(name);lv_event_send(findType(lv_scr_act(),&lv_keyboard_class),LV_EVENT_CANCEL,nullptr);Ui::update(s);
 }
 // Exercise the actual LVGL callbacks, without any real Wi-Fi or saved credentials.
 capture=true;s.settings.language=0;Ui::update(s);const char*choices[]={"Deutsch","Polski","English"};const int codes[]={1,0,2};
 // The theme is disabled, so slider track, indicator and knob must have
 // explicit opacity. Check the real widget and its value/save callback.
 Ui::show(Screen::Display);Ui::update(s);lv_obj_update_layout(lv_scr_act());auto*brightness=findType(lv_scr_act(),&lv_slider_class);assert(brightness);
 for(auto part:{LV_PART_MAIN,LV_PART_INDICATOR,LV_PART_KNOB})assert(lv_obj_get_style_bg_opa(brightness,part)==LV_OPA_COVER);
 sentCount=0;lv_slider_set_value(brightness,65,LV_ANIM_OFF);lv_event_send(brightness,LV_EVENT_VALUE_CHANGED,nullptr);assert(findLabel(lv_scr_act(),"65%"));lv_event_send(brightness,LV_EVENT_RELEASED,nullptr);
 assert(sentCount==1&&sent[0].action==Action::Setting&&sent[0].value==Brightness&&!strcmp(sent[0].text,"65"));
 click("Test kolorów");Ui::update(s);assert(Ui::current()==Screen::Colors);puts("Readability: visible slider parts, live percentage and brightness save callback; color-test navigation OK.");
 for(int i=0;i<3;i++){Ui::show(Screen::Languages);sentCount=0;click(choices[i]);assert(sentCount==1&&sent[0].action==Action::Setting&&sent[0].value==Language&&atoi(sent[0].text)==codes[i]);s.settings.language=codes[i];Ui::update(s);assert(Ui::current()==Screen::Languages);}
 s.settings.language=0;Ui::update(s);Ui::show(Screen::Network);sentCount=0;click("Wyszukaj sieci Wi-Fi");assert(sentCount==1&&sent[0].action==Action::WifiScan);Ui::update(s);assert(Ui::current()==Screen::WifiNetworks);
 sentCount=0;click("Home-24G");auto*input=findType(lv_scr_act(),&lv_textarea_class);auto*kb=findType(lv_scr_act(),&lv_keyboard_class);assert(input&&kb&&lv_textarea_get_password_mode(input));assert(!strlen(lv_textarea_get_text(input)));
 pressKey(kb,"a");pressKey(kb,"b");pressKey(kb,LV_SYMBOL_BACKSPACE);assert(!strcmp(lv_textarea_get_text(input),"a"));pressKey(kb,"ABC");pressKey(kb,"A");pressKey(kb,"1#");pressKey(kb,"1");assert(!strcmp(lv_textarea_get_text(input),"aA1"));pressKey(kb,"abc");
 lv_textarea_set_text(input,"short");lv_event_send(kb,LV_EVENT_READY,nullptr);assert(sentCount==0&&findType(lv_scr_act(),&lv_textarea_class));
 click("Pokaż");assert(!lv_textarea_get_password_mode(input));click("Ukryj");assert(lv_textarea_get_password_mode(input));
 lv_textarea_set_text(input,"Example-Password");lv_event_send(kb,LV_EVENT_READY,nullptr);assert(sentCount==4&&sent[0].value==NetworkMode&&!strcmp(sent[0].text,"1")&&sent[1].value==WifiSsid&&!strcmp(sent[1].text,"Home-24G")&&sent[2].value==WifiPassword&&!strcmp(sent[2].text,"Example-Password")&&sent[3].action==Action::Connect);Ui::update(s);assert(Ui::current()==Screen::Network);
 Ui::show(Screen::WifiNetworks);sentCount=0;click("Home-24G");click("×");assert(sentCount==0);Ui::update(s);
 Ui::show(Screen::WifiNetworks);sentCount=0;click("Home-24G");input=findType(lv_scr_act(),&lv_textarea_class);kb=findType(lv_scr_act(),&lv_keyboard_class);lv_textarea_set_text(input,"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz");lv_event_send(kb,LV_EVENT_READY,nullptr);assert(sentCount==0);lv_textarea_set_text(input,"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");pressKey(kb,LV_SYMBOL_OK);assert(sentCount==4&&strlen(sent[2].text)==64);Ui::update(s);
 Ui::show(Screen::WifiNetworks);sentCount=0;click("Guest");assert(sentCount==4&&sent[2].value==WifiPassword&&!strlen(sent[2].text)&&sent[3].action==Action::Connect);Ui::update(s);capture=false;
 const WifiScanState scans[]={WifiScanState::Scanning,WifiScanState::Done,WifiScanState::Failed};const char*scanNames[]={"wifi_scanning","wifi_empty","wifi_error"};
 for(int i=0;i<3;i++){s.wifiScan=scans[i];s.networkCount=0;s.wifiScanGeneration++;Ui::update(s);Ui::show(Screen::WifiNetworks);char name[512];snprintf(name,sizeof name,"%s/%s.ppm",argv[1],scanNames[i]);save(name);}
 // Actual OTA UI callbacks: check does not install; install requires two clicks
 // bound to the displayed version. Busy states disable controls and back navigation.
 capture=true;
 for(int lang=0;lang<3;lang++){
  s.settings.language=lang;s.ota=Ota::Status{};Ui::show(Screen::Update);Ui::update(s);
  const char*checkTexts[]={"Sprawdź aktualizacje","Updates prüfen","Check updates"};
  const char*installTexts[]={"Zainstaluj","Installieren","Install"};
  const char*confirmTexts[]={"Potwierdź instalację","Installation bestätigen","Confirm install"};
  sentCount=0;click(checkTexts[lang]);assert(sentCount==1&&sent[0].action==Action::OtaCheck);
  s.ota.state=Ota::State::Available;strcpy(s.ota.version,"1.4.0");strcpy(s.ota.notes[lang],"Test release");Ui::update(s);sentCount=0;
  click(installTexts[lang]);assert(sentCount==0);assert(findLabel(lv_scr_act(),confirmTexts[lang]));
  char name[512];snprintf(name,sizeof name,"%s/ota_confirm_%d.ppm",argv[1],lang);save(name);
  click(confirmTexts[lang]);assert(sentCount==1&&sent[0].action==Action::OtaInstall&&!strcmp(sent[0].text,"1.4.0"));
  s.ota.state=Ota::State::Downloading;s.ota.progress=62;Ui::update(s);lv_obj_update_layout(lv_scr_act());assert(findLabel(lv_scr_act(),"62%"));
  auto*progress=findType(lv_scr_act(),&lv_bar_class);assert(progress);assert(lv_obj_get_style_bg_opa(progress,LV_PART_MAIN)==LV_OPA_COVER&&lv_obj_get_style_bg_opa(progress,LV_PART_INDICATOR)==LV_OPA_COVER);
  auto*checkLabel=findLabel(lv_scr_act(),checkTexts[lang]);if(!checkLabel)dumpLabels(lv_scr_act());assert(checkLabel);assert(lv_obj_has_state(lv_obj_get_parent(checkLabel),LV_STATE_DISABLED));
  assert(lv_obj_has_state(lv_obj_get_parent(findLabel(lv_scr_act(),installTexts[lang])),LV_STATE_DISABLED));
  auto*back=findType(lv_scr_act(),&lv_btn_class);assert(back);lv_event_send(back,LV_EVENT_CLICKED,nullptr);Ui::update(s);assert(Ui::current()==Screen::Update);
  snprintf(name,sizeof name,"%s/ota_progress_%d.ppm",argv[1],lang);save(name);
  for(int error=1;error<=int(Ota::Error::Battery);error++){s.ota.state=Ota::State::Error;s.ota.error=Ota::Error(error);Ui::update(s);lv_obj_update_layout(lv_scr_act());lv_refr_now(nullptr);}
  snprintf(name,sizeof name,"%s/ota_error_%d.ppm",argv[1],lang);save(name);
  s.ota=Ota::Status{};Ui::update(s);
 }
 capture=false;
 puts("OTA PL/DE/EN: explicit check, two-click version-bound install, progress, disabled controls, blocked back and error states OK.");
 // Stress rebuilds and missing values, including high CO2 (32-bit chart coordinates).
 for(int i=0;i<150;i++){s.camera=i%2;s.alertCount=i%3;s.co2=5000;s.history[2][59]=5000;Ui::update(s);Ui::show(Screen::Metric,2);Ui::update(s);lv_tick_inc(20);lv_timer_handler();}
 s.bmeFresh=s.scdFresh=false;s.temperature=s.humidity=s.co2=NAN;Ui::show(Screen::Metric);Ui::update(s);char name[512];snprintf(name,sizeof name,"%s/ui_offline.ppm",argv[1]);save(name);
 printf("Rendered 66 screens, 3 password views, 3 scan states, offline and 150 transitions. Wi-Fi/password/language callbacks OK.\n");return 0;}
