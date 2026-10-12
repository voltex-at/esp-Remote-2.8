// Actual LVGL software renderer. Demo values exist only in this desktop preview.
#include "ui.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <initializer_list>
#include <string>
extern "C" int __lsan_is_turned_off(){return 1;} // restricted executor has no /proc thread access
static void flush(lv_disp_drv_t*d,const lv_area_t*,lv_color_t*){lv_disp_flush_ready(d);}
static Command sent[32];static int sentCount=0;static bool capture=false;
static lv_point_t pointerPos={0,0};static bool pointerDown=false;
static void pointerRead(lv_indev_drv_t*,lv_indev_data_t*d){d->point=pointerPos;d->state=pointerDown?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;}
static void pointerStep(int x,int y,bool down){pointerPos={lv_coord_t(x),lv_coord_t(y)};pointerDown=down;lv_tick_inc(40);lv_timer_handler();}
static void tap(int x,int y){pointerStep(x,y,true);pointerStep(x,y,true);pointerStep(x,y,false);pointerStep(x,y,false);}
static lv_obj_t*scrollBody(){for(uint32_t i=0;i<lv_obj_get_child_cnt(lv_scr_act());i++){auto*c=lv_obj_get_child(lv_scr_act(),i);if(lv_obj_check_type(c,&lv_obj_class)&&lv_obj_has_flag(c,LV_OBJ_FLAG_SCROLLABLE)&&lv_obj_get_scroll_dir(c)==LV_DIR_VER)return c;}return nullptr;}
static void checkTargets(lv_obj_t*p){
 if(lv_obj_check_type(p,&lv_keyboard_class)){auto*kb=(lv_btnmatrix_t*)p;assert(kb->btn_cnt==20);for(int i=0;i<20;i++){assert(lv_area_get_width(&kb->button_areas[i])>=44);assert(lv_area_get_height(&kb->button_areas[i])>=44);}}
 else if(lv_obj_has_flag(p,LV_OBJ_FLAG_CLICKABLE)&&p->spec_attr&&p->spec_attr->event_dsc_cnt){lv_area_t a;lv_obj_get_click_area(p,&a);if(lv_area_get_width(&a)<44||lv_area_get_height(&a)<44)fprintf(stderr,"Small target %dx%d\n",lv_area_get_width(&a),lv_area_get_height(&a));assert(lv_area_get_width(&a)>=44&&lv_area_get_height(&a)>=44);}
 for(uint32_t i=0;i<lv_obj_get_child_cnt(p);i++)checkTargets(lv_obj_get_child(p,i));
}
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
 // Every submenu target, bottom-of-list reachability and sibling spacing in
 // all languages. Home is independently compared pixel-for-pixel by Python.
 for(int lang=0;lang<3;lang++){
  s.settings.language=lang;Ui::update(s);
  for(auto screen:screens){if(screen==Screen::Home)continue;Ui::show(screen);Ui::update(s);lv_obj_update_layout(lv_scr_act());checkTargets(lv_scr_act());
   if(auto*panel=scrollBody()){
    for(uint32_t a=0;a<lv_obj_get_child_cnt(panel);a++)for(uint32_t b=a+1;b<lv_obj_get_child_cnt(panel);b++){
     lv_area_t x,y;lv_obj_get_coords(lv_obj_get_child(panel,a),&x);lv_obj_get_coords(lv_obj_get_child(panel,b),&y);
     bool overlap=x.x1<=y.x2&&y.x1<=x.x2&&x.y1<=y.y2&&y.y1<=x.y2;
     if(overlap)fprintf(stderr,"Overlapping siblings screen=%d language=%d children=%u,%u\n",int(screen),lang,a,b);assert(!overlap);
    }
    if(lv_obj_get_scroll_bottom(panel)>0){lv_obj_scroll_to_y(panel,lv_obj_get_scroll_y(panel)+lv_obj_get_scroll_bottom(panel),LV_ANIM_OFF);lv_obj_update_layout(lv_scr_act());
     auto*last=lv_obj_get_child(panel,-1);lv_area_t a,b;lv_obj_get_coords(last,&a);lv_obj_get_coords(panel,&b);assert(a.y2<=b.y2&&a.y2>=b.y1);
     char name[512];snprintf(name,sizeof name,"%s/scroll_%d_%d.ppm",argv[1],lang,int(screen));save(name);
    }
   }
  }
 }
 puts("Large UI: all submenu touch targets >=44px, sibling spacing and last-item scroll reachability PL/DE/EN OK.");
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
 click(LV_SYMBOL_EYE_OPEN);assert(!lv_textarea_get_password_mode(input));click(LV_SYMBOL_EYE_CLOSE);assert(lv_textarea_get_password_mode(input));
 lv_textarea_set_text(input,"Example-Password");lv_event_send(kb,LV_EVENT_READY,nullptr);assert(sentCount==4&&sent[0].value==NetworkMode&&!strcmp(sent[0].text,"1")&&sent[1].value==WifiSsid&&!strcmp(sent[1].text,"Home-24G")&&sent[2].value==WifiPassword&&!strcmp(sent[2].text,"Example-Password")&&sent[3].action==Action::Connect);Ui::update(s);assert(Ui::current()==Screen::Network);
 Ui::show(Screen::WifiNetworks);sentCount=0;click("Home-24G");click("×");assert(sentCount==0);Ui::update(s);
 Ui::show(Screen::WifiNetworks);sentCount=0;click("Home-24G");input=findType(lv_scr_act(),&lv_textarea_class);kb=findType(lv_scr_act(),&lv_keyboard_class);lv_textarea_set_text(input,"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz");lv_event_send(kb,LV_EVENT_READY,nullptr);assert(sentCount==0);lv_textarea_set_text(input,"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");pressKey(kb,LV_SYMBOL_OK);assert(sentCount==4&&strlen(sent[2].text)==64);Ui::update(s);
 Ui::show(Screen::WifiNetworks);sentCount=0;click("Guest");assert(sentCount==4&&sent[2].value==WifiPassword&&!strlen(sent[2].text)&&sent[3].action==Action::Connect);Ui::update(s);
 // Every printable ASCII character is reachable on the large paged keyboard;
 // page controls never become part of the password, and each key is >=44x44.
 Ui::show(Screen::WifiNetworks);click("Home-24G");input=findType(lv_scr_act(),&lv_textarea_class);kb=findType(lv_scr_act(),&lv_keyboard_class);bool covered[127]={};
 const char*routes[]={"","n-z","ABC","A-M","1#","#+","?+"};
 for(auto*route:routes){lv_textarea_set_text(input,"");if(*route)pressKey(kb,route);assert(!strlen(lv_textarea_get_text(input)));lv_obj_update_layout(lv_scr_act());checkTargets(lv_scr_act());std::string expected;
  for(int i=0;i<20;i++){const char*t=lv_btnmatrix_get_btn_text(kb,i);if(strlen(t)==1){unsigned char c=t[0];assert(c>=32&&c<=126);covered[c]=true;expected+=char(c);pressKey(kb,t);}}
  assert(expected==lv_textarea_get_text(input));
 }
 for(int c=32;c<=126;c++)assert(covered[c]);lv_event_send(kb,LV_EVENT_CANCEL,nullptr);Ui::update(s);capture=false;
 // Real pointer input verifies the enlarged row and the switch itself each
 // dispatch exactly once, dragging scrolls without an accidental activation,
 // and a disabled camera control cannot be enabled by tapping its label.
 static lv_indev_drv_t ptr;lv_indev_drv_init(&ptr);ptr.type=LV_INDEV_TYPE_POINTER;ptr.read_cb=pointerRead;lv_indev_drv_register(&ptr);capture=true;
 Ui::show(Screen::Alerts);Ui::update(s);lv_obj_update_layout(lv_scr_act());auto*sw=findType(lv_scr_act(),&lv_switch_class);assert(sw);auto*r=lv_obj_get_parent(sw);lv_area_t ra,sa;lv_obj_get_coords(r,&ra);lv_obj_get_coords(sw,&sa);bool old=lv_obj_has_state(sw,LV_STATE_CHECKED);sentCount=0;tap(ra.x1+70,ra.y1+24);assert(sentCount==1&&sent[0].action==Action::Setting&&sent[0].value==Notifications&&atoi(sent[0].text)==!old);
 sentCount=0;tap((sa.x1+sa.x2)/2,(sa.y1+sa.y2)/2);assert(sentCount==1&&atoi(sent[0].text)==old);
 Ui::show(Screen::CameraSettings);Ui::update(s);lv_obj_update_layout(lv_scr_act());sw=findType(lv_scr_act(),&lv_switch_class);assert(lv_obj_has_state(sw,LV_STATE_DISABLED));lv_obj_get_coords(lv_obj_get_parent(sw),&ra);sentCount=0;tap(ra.x1+70,ra.y1+24);assert(sentCount==0);
 Ui::show(Screen::Settings);Ui::update(s);auto*panel=scrollBody();assert(panel);sentCount=0;pointerStep(90,220,true);pointerStep(90,190,true);pointerStep(90,155,true);pointerStep(90,120,true);pointerStep(90,100,false);pointerStep(90,100,false);assert(lv_obj_get_scroll_y(panel)>20&&sentCount==0&&Ui::current()==Screen::Settings);
 Ui::show(Screen::System);Ui::update(s);click("Ustawienia fabryczne");lv_obj_update_layout(lv_scr_act());checkTargets(lv_scr_act());char resetName[512];snprintf(resetName,sizeof resetName,"%s/reset_dialog.ppm",argv[1]);save(resetName);assert(sentCount==0);Ui::show(Screen::Home);
 capture=false;puts("Large keyboard: all ASCII, real pointer taps, disabled controls, drag scrolling and reset confirmation OK.");
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
  for(int error=1;error<=int(Ota::Error::Redirect);error++){s.ota.state=Ota::State::Error;s.ota.error=Ota::Error(error);Ui::update(s);lv_obj_update_layout(lv_scr_act());lv_refr_now(nullptr);}
  s.ota.error=Ota::Error::Tls;s.ota.detail=-9984;Ui::update(s);
  const char*tlsTexts[]={"Błąd HTTPS/TLS. Kod: -9984","HTTPS/TLS-Fehler. Code: -9984","HTTPS/TLS error. Code: -9984"};assert(findLabel(lv_scr_act(),tlsTexts[lang]));
  snprintf(name,sizeof name,"%s/ota_error_%d.ppm",argv[1],lang);save(name);
  s.ota.error=Ota::Error::Memory;s.ota.detail=32512;Ui::update(s);const char*ramTexts[]={"Brak RAM dla HTTPS.\nKod: 32512","Zu wenig RAM für HTTPS.\nCode: 32512","Not enough RAM for HTTPS.\nCode: 32512"};assert(findLabel(lv_scr_act(),ramTexts[lang]));
  snprintf(name,sizeof name,"%s/ota_memory_%d.ppm",argv[1],lang);save(name);
  s.ota=Ota::Status{};Ui::update(s);
 }
 capture=false;
 puts("OTA PL/DE/EN: explicit check, two-click version-bound install, progress, disabled controls, blocked back and error states OK.");
 // Stress rebuilds and missing values, including high CO2 (32-bit chart coordinates).
 for(int i=0;i<150;i++){s.camera=i%2;s.alertCount=i%3;s.co2=5000;s.history[2][59]=5000;Ui::update(s);Ui::show(Screen::Metric,2);Ui::update(s);lv_tick_inc(20);lv_timer_handler();}
 s.bmeFresh=s.scdFresh=false;s.temperature=s.humidity=s.co2=NAN;Ui::show(Screen::Metric);Ui::update(s);char name[512];snprintf(name,sizeof name,"%s/ui_offline.ppm",argv[1]);save(name);
 printf("Rendered 66 screens, 3 password views, 3 scan states, offline and 150 transitions. Wi-Fi/password/language callbacks OK.\n");return 0;}
