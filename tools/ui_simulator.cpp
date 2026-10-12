// Actual LVGL software renderer. Demo values exist only in this desktop preview.
#include "ui.h"
#include "ui_palette.h"
#include "assets.h"
#include "video_geometry.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <initializer_list>
#include <string>
extern "C" int __lsan_is_turned_off(){return 1;} // restricted executor has no /proc thread access
static lv_color_t nativePixels[240*320];static uint32_t flushCount=0;
static void flush(lv_disp_drv_t*d,const lv_area_t*a,lv_color_t*p){assert(a->x1>=0&&a->x2<240&&a->y1>=0&&a->y2<320);int width=lv_area_get_width(a);for(int y=a->y1;y<=a->y2;y++)for(int x=a->x1;x<=a->x2;x++)nativePixels[y*240+x]=p[(y-a->y1)*width+x-a->x1];flushCount++;lv_disp_flush_ready(d);}
static Command sent[32];static int sentCount=0;static bool capture=false;
static lv_point_t pointerPos={0,0};static bool pointerDown=false;
static void pointerRead(lv_indev_drv_t*,lv_indev_data_t*d){d->point=pointerPos;d->state=pointerDown?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;}
static void pointerStep(int x,int y,bool down){if(lv_disp_get_rotation(nullptr)==LV_DISP_ROT_90)pointerPos={lv_coord_t(y),lv_coord_t(319-x)};else pointerPos={lv_coord_t(x),lv_coord_t(y)};pointerDown=down;lv_tick_inc(40);lv_timer_handler();}
static void tap(int x,int y){pointerStep(x,y,true);pointerStep(x,y,true);pointerStep(x,y,false);pointerStep(x,y,false);}
static lv_obj_t*scrollBody(){for(uint32_t i=0;i<lv_obj_get_child_cnt(lv_scr_act());i++){auto*c=lv_obj_get_child(lv_scr_act(),i);if(lv_obj_check_type(c,&lv_obj_class)&&lv_obj_has_flag(c,LV_OBJ_FLAG_SCROLLABLE)&&lv_obj_get_scroll_dir(c)==LV_DIR_VER)return c;}return nullptr;}
static void checkTargets(lv_obj_t*p){
 if(lv_obj_check_type(p,&lv_keyboard_class)){auto*kb=(lv_btnmatrix_t*)p;assert(kb->btn_cnt==20);for(int i=0;i<20;i++){assert(lv_area_get_width(&kb->button_areas[i])>=44);assert(lv_area_get_height(&kb->button_areas[i])>=44);}}
 else if(lv_obj_has_flag(p,LV_OBJ_FLAG_CLICKABLE)&&p->spec_attr&&p->spec_attr->event_dsc_cnt){lv_area_t a;lv_obj_get_click_area(p,&a);if(lv_area_get_width(&a)<44||lv_area_get_height(&a)<44)fprintf(stderr,"Small target %dx%d\n",lv_area_get_width(&a),lv_area_get_height(&a));assert(lv_area_get_width(&a)>=44&&lv_area_get_height(&a)>=44);}
 for(uint32_t i=0;i<lv_obj_get_child_cnt(p);i++)checkTargets(lv_obj_get_child(p,i));
}
static void command(Action a,int value,const char*t){if(capture){assert(sentCount<32);sent[sentCount]={a,value,{0}};snprintf(sent[sentCount++].text,128,"%s",t?t:"");}}
static lv_obj_t*findLabel(lv_obj_t*parent,const char*text){if(lv_obj_check_type(parent,&lv_label_class)&&!strcmp(lv_label_get_text(parent),text))return parent;for(uint32_t i=0;i<lv_obj_get_child_cnt(parent);i++)if(auto*o=findLabel(lv_obj_get_child(parent,i),text))return o;return nullptr;}
static int countLabel(lv_obj_t*p,const char*text){int count=lv_obj_check_type(p,&lv_label_class)&&!strcmp(lv_label_get_text(p),text);for(uint32_t i=0;i<lv_obj_get_child_cnt(p);i++)count+=countLabel(lv_obj_get_child(p,i),text);return count;}
static lv_obj_t*findType(lv_obj_t*parent,const lv_obj_class_t*type){if(lv_obj_check_type(parent,type))return parent;for(uint32_t i=0;i<lv_obj_get_child_cnt(parent);i++)if(auto*o=findType(lv_obj_get_child(parent,i),type))return o;return nullptr;}
static void dumpLabels(lv_obj_t*p){if(lv_obj_check_type(p,&lv_label_class))fprintf(stderr,"Label: %s\n",lv_label_get_text(p));for(uint32_t i=0;i<lv_obj_get_child_cnt(p);i++)dumpLabels(lv_obj_get_child(p,i));}
static void click(const char*text){lv_obj_update_layout(lv_scr_act());auto*l=findLabel(lv_scr_act(),text);if(!l)fprintf(stderr,"Missing UI target: %s\n",text);assert(l);lv_event_send(lv_obj_get_parent(l),LV_EVENT_CLICKED,nullptr);}
static lv_obj_t*findImage(lv_obj_t*p,const lv_img_dsc_t*src){if(lv_obj_check_type(p,&lv_img_class)&&lv_img_get_src(p)==src)return p;for(uint32_t i=0;i<lv_obj_get_child_cnt(p);i++)if(auto*o=findImage(lv_obj_get_child(p,i),src))return o;return nullptr;}
static void checkHeader(const AppState&s){lv_obj_update_layout(lv_scr_act());char text[12];if(s.batteryPct>=0)snprintf(text,sizeof text,"%d%%",s.batteryPct);else strcpy(text,"—");
 auto*pct=findLabel(lv_scr_act(),text);auto*clock=findLabel(lv_scr_act(),s.timeText);auto*wifi=findImage(lv_scr_act(),&img_wifi);auto*bolt=findImage(lv_scr_act(),&img_bolt);assert(pct&&clock&&wifi&&bolt);auto*holder=lv_obj_get_parent(bolt);auto*shell=lv_obj_get_child(holder,0);auto*cap=lv_obj_get_child(holder,1);lv_area_t p,t,w,b,c;lv_obj_get_coords(pct,&p);lv_obj_get_coords(clock,&t);lv_obj_get_coords(wifi,&w);lv_obj_get_coords(holder,&b);lv_obj_get_coords(cap,&c);
 int textWidth=lv_txt_get_width(text,strlen(text),lv_obj_get_style_text_font(pct,0),0,LV_TEXT_FLAG_NONE),wifiWidth=28*lv_img_get_zoom(wifi)/256;int textLeft=p.x2+1-textWidth,visibleRight=lv_obj_has_flag(bolt,LV_OBJ_FLAG_HIDDEN)?c.x2+1:b.x2+1;
 assert(textWidth<=lv_obj_get_width(pct)&&p.x2==lv_disp_get_hor_res(nullptr)-9&&textLeft-visibleRight==4&&b.x1-(w.x1+wifiWidth)==8);
 assert(lv_txt_get_width(s.timeText,strlen(s.timeText),lv_obj_get_style_text_font(clock,0),0,LV_TEXT_FLAG_NONE)<=lv_obj_get_width(clock)&&t.x2<w.x1);
 assert(b.y1>=0&&b.y2<(Ui::current()==Screen::Home?36:24));
 lv_area_t shellArea;lv_obj_get_coords(shell,&shellArea);int center=(shellArea.y1+shellArea.y2)/2;
 for(int i=0;i<4;i++){lv_area_t cell;lv_obj_get_coords(lv_obj_get_child(shell,i),&cell);assert(cell.x1>shellArea.x1&&cell.x2<shellArea.x2&&cell.y1>shellArea.y1&&cell.y2<shellArea.y2&&abs((cell.y1+cell.y2)/2-center)<=1);}
 // Inspect the real rendered glyph pixels, independently of the positioning
 // function: percentage and clock must be level with the outline's center.
 auto*snapshot=lv_snapshot_take(lv_scr_act(),LV_IMG_CF_TRUE_COLOR);assert(snapshot);auto*pixels=(lv_color_t*)snapshot->data;
 for(auto*label:{pct,clock}){lv_area_t area;lv_obj_get_coords(label,&area);auto ink=lv_color_to32(lv_obj_get_style_text_color(label,0));int top=1000,bottom=-1;
  for(int y=std::max(0,int(area.y1));y<=area.y2&&y<int(snapshot->header.h);y++)for(int x=area.x1;x<=area.x2;x++){lv_color32_t pixel;pixel.full=lv_color_to32(pixels[y*snapshot->header.w+x]);lv_color32_t target;target.full=ink;int delta=abs(int(pixel.ch.red)-target.ch.red)+abs(int(pixel.ch.green)-target.ch.green)+abs(int(pixel.ch.blue)-target.ch.blue);if(delta<50){top=std::min(top,y);bottom=std::max(bottom,y);}}
  if(bottom<0||abs((top+bottom)/2-center)>1)fprintf(stderr,"Header misalignment: %s visible y=%d..%d battery center=%d\n",lv_label_get_text(label),top,bottom,center);assert(bottom>=0&&abs((top+bottom)/2-center)<=1);
 }lv_snapshot_free(snapshot);
}
static void clickImage(const lv_img_dsc_t*src){auto*i=findImage(lv_scr_act(),src);assert(i);lv_event_send(lv_obj_get_parent(i),LV_EVENT_CLICKED,nullptr);}
static void pressKey(lv_obj_t*kb,const char*key){for(uint16_t i=0;i<100;i++){const char*t=lv_btnmatrix_get_btn_text(kb,i);if(!t)break;if(!strcmp(t,key)){lv_btnmatrix_set_selected_btn(kb,i);lv_event_send(kb,LV_EVENT_VALUE_CHANGED,nullptr);return;}}assert(false);}
static void save(const char*name){lv_obj_update_layout(lv_scr_act());lv_refr_now(nullptr);auto*s=lv_snapshot_take(lv_scr_act(),LV_IMG_CF_TRUE_COLOR);if(!s){fprintf(stderr,"snapshot failed %s\n",name);exit(1);}FILE*f=fopen(name,"wb");fprintf(f,"P6\n%d %d\n255\n",s->header.w,s->header.h);auto*p=(lv_color_t*)s->data;for(unsigned i=0;i<s->header.w*s->header.h;i++){lv_color32_t c; c.full=lv_color_to32(p[i]);fputc(c.ch.red,f);fputc(c.ch.green,f);fputc(c.ch.blue,f);}fclose(f);lv_snapshot_free(s);}
int main(int argc,char**argv){if(argc!=2)return 1;lv_init();static lv_color_t buf[240*24];static lv_disp_draw_buf_t b;lv_disp_draw_buf_init(&b,buf,nullptr,240*24);static lv_disp_drv_t d;lv_disp_drv_init(&d);d.hor_res=240;d.ver_res=320;d.sw_rotate=1;d.flush_cb=flush;d.draw_buf=&b;lv_disp_drv_register(&d);Ui::begin(command);
 AppState s;assert(!s.settings.ssid[0]&&!s.settings.password[0]&&!s.settings.pairToken[0]);Ui::show(Screen::WifiSettings);Ui::update(s);lv_obj_update_layout(lv_scr_act());assert(findLabel(lv_scr_act(),"Nie ustawiono"));
 s.batteryPct=80;s.batteryV=3.98;s.wifi=true;s.rssi=-54;s.wifiLink=WifiLinkState::Connected;strcpy(s.connectedSsid,"ESP32-BABY-CAM");strcpy(s.settings.ssid,s.connectedSsid);s.wifiScan=WifiScanState::Done;s.wifiScanGeneration=1;s.networkCount=5;
 const char*ssids[]={"ESP32-BABY-CAM","Home-24G","Guest","Office-Enterprise","A-very-long-SSID-with-32-letters"};for(int i=0;i<5;i++){snprintf(s.networks[i].ssid,33,"%s",ssids[i]);s.networks[i].rssi=-42-i*10;s.networks[i].security=i==2?WifiSecurity::Open:i==3?WifiSecurity::Unsupported:WifiSecurity::Wpa2;}
 s.bmeOnline=s.scdOnline=s.bmeFresh=s.scdFresh=true;s.temperature=23.4;s.humidity=54;s.co2=742;s.iaq=42;s.pressure=1013.2;s.eco2=608;s.iaqAccuracy=2;s.historyCount=60;s.sd=true;s.sdFree=12ULL*1024*1024*1024;strcpy(s.timeText,"14:26");strcpy(s.ip,"192.168.4.2");strcpy(s.message,"");
 for(int m=0;m<6;m++)for(int i=0;i<60;i++)s.history[m][i]=metricValue(s,m)+sin(i*.34f)*(m==2?110:m==1?7:m==3?15:2);
 s.sensorCount=4;s.nowMs=10000;const char*sensorNames[]={"Salon","Sypialnia","POGO BME688","BTHome bateria"};const int selected[]={0,2,1,6};
 for(int i=0;i<4;i++){auto&d=s.sensors[i];d.config.source=i==2?SensorRegistry::Source::Bme688:SensorRegistry::Source::Bthome;strcpy(d.config.name,sensorNames[i]);snprintf(d.config.address,18,"aa:bb:cc:dd:ee:%02d",i);d.config.capabilities=i==2?0x3b:0x57;d.config.selected=selected[i];for(int m=0;m<7;m++){d.values[m]=m==6?91:metricValue(s,m);d.updated[m]=10000;}d.historyCount=60;for(int j=0;j<60;j++)d.history[j]=d.values[selected[i]];}
 s.bleCount=1;auto&advertised=s.bleDevices[0];advertised.config=s.sensors[0].config;strcpy(advertised.config.name,"BLE salon");advertised.rssi=-48;s.bleState=BleState::Done;
 Ui::update(s);const Screen screens[]={Screen::Home,Screen::Sensors,Screen::Metric,Screen::Settings,Screen::General,Screen::Display,Screen::CameraSettings,Screen::Network,Screen::Scan,Screen::Battery,Screen::System,Screen::Alerts,Screen::Cameras,Screen::Audio,Screen::Live,Screen::Limits,Screen::SensorSettings,Screen::Recordings,Screen::Pair,Screen::WifiNetworks,Screen::Languages,Screen::Colors,Screen::Appearance,Screen::Update,Screen::AudioSettings,Screen::WifiSettings,Screen::Bluetooth,Screen::Direct,Screen::AddSensor,Screen::SensorValue};
 for(int lang=0;lang<3;lang++){s.settings.language=lang;Ui::update(s);for(unsigned i=0;i<sizeof screens/sizeof screens[0];i++){Ui::show(screens[i],0);Ui::update(s);lv_tick_inc(30);lv_timer_handler();char name[512];snprintf(name,sizeof name,"%s/ui_%d_%02d.ppm",argv[1],lang,i);save(name);}
  const char*settingsNames[]={"Ustawienia","Einstellungen","Settings"};const char*temperatureNames[]={"Salon","Salon","Salon"};const char*dimNames[]={"Przygaś po [s]","Dimmen nach [s]","Dim after [s]"};
  Ui::show(Screen::Home);Ui::update(s);lv_obj_update_layout(lv_scr_act());assert(findLabel(lv_scr_act(),settingsNames[lang]));
  Ui::show(Screen::Sensors);Ui::update(s);lv_obj_update_layout(lv_scr_act());auto*temperature=findLabel(lv_scr_act(),temperatureNames[lang]);assert(temperature);assert(lv_obj_get_height(temperature)<=lv_obj_get_style_text_font(temperature,LV_PART_MAIN)->line_height);
  Ui::show(Screen::Display);Ui::update(s);lv_obj_update_layout(lv_scr_act());auto*dim=findLabel(lv_scr_act(),dimNames[lang]);auto*value=findLabel(lv_scr_act(),"60");assert(dim&&value);lv_area_t a,b;lv_obj_get_coords(dim,&a);lv_obj_get_coords(value,&b);assert(a.y2<b.y1);
  Ui::show(Screen::WifiNetworks);Ui::update(s);click("Home-24G");auto*input=findType(lv_scr_act(),&lv_textarea_class);assert(input&&lv_textarea_get_password_mode(input));lv_textarea_set_text(input,"Example-Password");char name[512];snprintf(name,sizeof name,"%s/wifi_password_%d.ppm",argv[1],lang);save(name);lv_event_send(findType(lv_scr_act(),&lv_keyboard_class),LV_EVENT_CANCEL,nullptr);Ui::update(s);
 }
 // Render every light screen in PL/DE/EN and check actual target geometry.
 s.theme=ThemeMode::Light;
 for(int lang=0;lang<3;lang++){s.settings.language=lang;Ui::update(s);
  for(unsigned i=0;i<sizeof screens/sizeof screens[0];i++){
   Ui::show(screens[i]);Ui::update(s);lv_obj_update_layout(lv_scr_act());checkTargets(lv_scr_act());
   assert(lv_color_to32(lv_obj_get_style_bg_color(lv_scr_act(),0))==lv_color_to32(lv_color_hex(screens[i]==Screen::Live?0x000000:UiColors::get(s.theme,screens[i]==Screen::Home).background)));
   if(auto*panel=scrollBody()){
    for(uint32_t a=0;a<lv_obj_get_child_cnt(panel);a++)for(uint32_t b=a+1;b<lv_obj_get_child_cnt(panel);b++){lv_area_t x,y;lv_obj_get_coords(lv_obj_get_child(panel,a),&x);lv_obj_get_coords(lv_obj_get_child(panel,b),&y);assert(!(x.x1<=y.x2&&y.x1<=x.x2&&x.y1<=y.y2&&y.y1<=x.y2));}
    if(lv_obj_get_scroll_bottom(panel)>0){lv_obj_scroll_to_y(panel,lv_obj_get_scroll_y(panel)+lv_obj_get_scroll_bottom(panel),LV_ANIM_OFF);lv_obj_update_layout(lv_scr_act());lv_area_t a,b;lv_obj_get_coords(lv_obj_get_child(panel,-1),&a);lv_obj_get_coords(panel,&b);assert(a.y2<=b.y2&&a.y2>=b.y1);lv_obj_scroll_to_y(panel,0,LV_ANIM_OFF);}
   }
   char name[512];snprintf(name,sizeof name,"%s/light_ui_%d_%02d.ppm",argv[1],lang,i);save(name);
  }
  Ui::show(Screen::WifiNetworks);Ui::update(s);click("Home-24G");auto*input=findType(lv_scr_act(),&lv_textarea_class);lv_textarea_set_text(input,"Example-Password");checkTargets(lv_scr_act());char name[512];snprintf(name,sizeof name,"%s/light_wifi_password_%d.ppm",argv[1],lang);save(name);lv_event_send(findType(lv_scr_act(),&lv_keyboard_class),LV_EVENT_CANCEL,nullptr);Ui::update(s);
 }
 s.theme=ThemeMode::Dark;Ui::update(s);
 // Sensor overview readings update without navigating or rebuilding the page.
 s.settings.language=0;Ui::update(s);Ui::show(Screen::Sensors);Ui::update(s);
 s.sensors[0].values[0]=27.1f;s.sensors[1].values[2]=987;Ui::update(s);assert(findLabel(lv_scr_act(),"27.1 °C")&&findLabel(lv_scr_act(),"987 ppm"));
 s.sensors[0].values[0]=NAN;Ui::update(s);assert(findLabel(lv_scr_act(),"—"));
 s.sensors[0].values[0]=23.4f;s.sensors[1].values[2]=742;Ui::update(s);
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
 s.settings.language=0;Ui::update(s);Ui::show(Screen::Network);sentCount=0;assert(findLabel(lv_scr_act(),"WLAN")&&findLabel(lv_scr_act(),"Bluetooth")&&findLabel(lv_scr_act(),"ESP"));click("WLAN");Ui::update(s);assert(Ui::current()==Screen::WifiSettings&&sentCount==0);click("Wyszukaj Wi-Fi");assert(sentCount==1&&sent[0].action==Action::WifiScan);Ui::update(s);assert(Ui::current()==Screen::WifiNetworks);
 sentCount=0;click("Home-24G");auto*input=findType(lv_scr_act(),&lv_textarea_class);auto*kb=findType(lv_scr_act(),&lv_keyboard_class);assert(input&&kb&&lv_textarea_get_password_mode(input));assert(!strlen(lv_textarea_get_text(input)));
 pressKey(kb,"a");pressKey(kb,"b");pressKey(kb,LV_SYMBOL_BACKSPACE);assert(!strcmp(lv_textarea_get_text(input),"a"));pressKey(kb,"ABC");pressKey(kb,"A");pressKey(kb,"1#");pressKey(kb,"1");assert(!strcmp(lv_textarea_get_text(input),"aA1"));pressKey(kb,"abc");
 lv_textarea_set_text(input,"short");lv_event_send(kb,LV_EVENT_READY,nullptr);assert(sentCount==0&&findType(lv_scr_act(),&lv_textarea_class));
 clickImage(&img_eye);assert(!lv_textarea_get_password_mode(input));clickImage(&img_eye_off);assert(lv_textarea_get_password_mode(input));
 lv_textarea_set_text(input,"Example-Password");lv_event_send(kb,LV_EVENT_READY,nullptr);assert(sentCount==4&&sent[0].value==NetworkMode&&!strcmp(sent[0].text,"1")&&sent[1].value==WifiSsid&&!strcmp(sent[1].text,"Home-24G")&&sent[2].value==WifiPassword&&!strcmp(sent[2].text,"Example-Password")&&sent[3].action==Action::Connect);Ui::update(s);assert(Ui::current()==Screen::WifiSettings);
 Ui::show(Screen::WifiNetworks);sentCount=0;click("Home-24G");clickImage(&img_close);assert(sentCount==0);Ui::update(s);
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
 Ui::show(Screen::System);Ui::update(s);click("Reset ustawień");lv_obj_update_layout(lv_scr_act());checkTargets(lv_scr_act());char resetName[512];snprintf(resetName,sizeof resetName,"%s/reset_dialog.ppm",argv[1]);save(resetName);assert(sentCount==0);Ui::show(Screen::Home);
 capture=false;puts("Large keyboard: all ASCII, real pointer taps, disabled controls, drag scrolling and reset confirmation OK.");
 const WifiScanState scans[]={WifiScanState::Scanning,WifiScanState::Done,WifiScanState::Failed};const char*scanNames[]={"wifi_scanning","wifi_empty","wifi_error"};
 for(int i=0;i<3;i++){s.wifiScan=scans[i];s.networkCount=0;s.wifiScanGeneration++;Ui::update(s);Ui::show(Screen::WifiNetworks);char name[512];snprintf(name,sizeof name,"%s/%s.ppm",argv[1],scanNames[i]);save(name);}
 // No measurement choice before registration, then only the added device's
 // actual capabilities. MAC in commands binds selection to a stable identity.
 capture=true;s.settings.language=0;AppState saved=s;s.sensorCount=0;s.sensorGeneration++;Ui::update(s);Ui::show(Screen::Sensors);assert(findLabel(lv_scr_act(),"Dodaj sensor")&&!findLabel(lv_scr_act(),"23.4 °C"));click("Dodaj sensor");Ui::update(s);assert(Ui::current()==Screen::AddSensor);click("Bluetooth");Ui::update(s);assert(Ui::current()==Screen::Bluetooth&&!findLabel(lv_scr_act(),"Telefon"));sentCount=0;click("Wyszukaj sensory");assert(sentCount==1&&sent[0].action==Action::BleScan);sentCount=0;click("BLE salon");assert(sentCount==1&&sent[0].action==Action::SensorAdd&&!strcmp(sent[0].text,"aa:bb:cc:dd:ee:00"));
 s.sensorCount=1;s.sensors[0]=saved.sensors[0];s.sensors[0].config.selected=SensorRegistry::None;s.sensors[0].config.capabilities=3;s.lastAdded=0;s.sensorGeneration++;s.sensorAddedGeneration++;Ui::update(s);assert(Ui::current()==Screen::SensorValue&&findLabel(lv_scr_act(),"Wilgotność")&&!findLabel(lv_scr_act(),"Ciśnienie"));
 char newName[512];snprintf(newName,sizeof newName,"%s/sensor_choose.ppm",argv[1]);save(newName);sentCount=0;click("Wilgotność");assert(sentCount==1&&sent[0].action==Action::SensorSelect&&sent[0].value==1&&!strcmp(sent[0].text,s.sensors[0].config.address));s.sensors[0].config.selected=1;s.sensorGeneration++;Ui::update(s);assert(Ui::current()==Screen::Sensors&&findLabel(lv_scr_act(),"54 %"));
 s=saved;Ui::update(s);Ui::show(Screen::Home);Ui::update(s);auto*tiles=findType(lv_scr_act(),&lv_tileview_class);assert(tiles&&lv_obj_get_x(lv_tileview_get_tile_act(tiles))==0);sentCount=0;
 for(int x=220;x>=20;x-=25)pointerStep(x,175,true);pointerStep(20,175,false);for(int i=0;i<20;i++)pointerStep(20,175,false);Ui::update(s);assert(Ui::current()==Screen::Home&&sentCount==0&&lv_obj_get_x(lv_tileview_get_tile_act(tiles))==240);
 snprintf(newName,sizeof newName,"%s/home_second.ppm",argv[1]);save(newName);s.sensors[0].values[0]=26.2;Ui::update(s);assert(findLabel(lv_scr_act(),"26.2 °C")&&lv_obj_get_x(lv_tileview_get_tile_act(tiles))==240);
 s.theme=ThemeMode::Light;Ui::update(s);tiles=findType(lv_scr_act(),&lv_tileview_class);assert(lv_obj_get_x(lv_tileview_get_tile_act(tiles))==240);snprintf(newName,sizeof newName,"%s/light_home_second.ppm",argv[1]);save(newName);
 for(int x=20;x<=220;x+=25)pointerStep(x,175,true);pointerStep(220,175,false);for(int i=0;i<20;i++)pointerStep(220,175,false);assert(lv_obj_get_x(lv_tileview_get_tile_act(tiles))==0&&sentCount==0);
 s.theme=ThemeMode::Dark;s.sensors[0].values[0]=23.4;Ui::update(s);Ui::show(Screen::System);click("Software");Ui::update(s);assert(Ui::current()==Screen::Update);tap(23,46);Ui::update(s);assert(Ui::current()==Screen::System);capture=false;
 puts("Sensors: add-first BLE workflow, capability-bound reading, stable device address, saved cards, real horizontal swipe without click-through, live second page and Software navigation OK.");
 // Actual OTA UI callbacks: check does not install; install requires two clicks
 // bound to the displayed version. Busy states disable controls and back navigation.
 capture=true;
 for(int theme=0;theme<2;theme++){s.theme=ThemeSettings::normalize(theme);
 for(int lang=0;lang<3;lang++){
  s.settings.language=lang;s.ota=Ota::Status{};Ui::show(Screen::Update);Ui::update(s);
  const char*checkTexts[]={"Sprawdź","Prüfen","Check"};
  const char*installTexts[]={"Aktualizuj","Update","Update"};
  const char*confirmTexts[]={"Potwierdź","Bestätigen","Confirm"};
  char idleName[512];snprintf(idleName,sizeof idleName,"%s/%sota_idle_%d.ppm",argv[1],theme?"light_":"",lang);save(idleName);
  sentCount=0;click(checkTexts[lang]);assert(sentCount==1&&sent[0].action==Action::OtaCheck);
  s.ota.state=Ota::State::Available;strcpy(s.ota.version,"1.4.0");strcpy(s.ota.notes[lang],"Test release");Ui::update(s);sentCount=0;
  tap(178,289);assert(sentCount==0);assert(findLabel(lv_scr_act(),confirmTexts[lang]));
  char name[512];snprintf(name,sizeof name,"%s/%sota_confirm_%d.ppm",argv[1],theme?"light_":"",lang);save(name);
  tap(178,289);assert(sentCount==1&&sent[0].action==Action::OtaInstall&&!strcmp(sent[0].text,"1.4.0"));
  s.ota.state=Ota::State::Downloading;s.ota.progress=62;Ui::update(s);lv_obj_update_layout(lv_scr_act());assert(findLabel(lv_scr_act(),"62%"));
  auto*progress=findType(lv_scr_act(),&lv_bar_class);assert(progress);assert(lv_obj_get_style_bg_opa(progress,LV_PART_MAIN)==LV_OPA_COVER&&lv_obj_get_style_bg_opa(progress,LV_PART_INDICATOR)==LV_OPA_COVER);
  auto*checkLabel=findLabel(lv_scr_act(),checkTexts[lang]);if(!checkLabel)dumpLabels(lv_scr_act());assert(checkLabel);assert(lv_obj_has_state(lv_obj_get_parent(checkLabel),LV_STATE_DISABLED));
  assert(lv_obj_has_state(lv_obj_get_parent(findLabel(lv_scr_act(),installTexts[lang])),LV_STATE_DISABLED));sentCount=0;tap(178,289);tap(60,289);assert(sentCount==0);
  auto*back=findType(lv_scr_act(),&lv_btn_class);assert(back);lv_event_send(back,LV_EVENT_CLICKED,nullptr);Ui::update(s);assert(Ui::current()==Screen::Update);
  snprintf(name,sizeof name,"%s/%sota_progress_%d.ppm",argv[1],theme?"light_":"",lang);save(name);
  for(int error=1;error<=int(Ota::Error::Redirect);error++){s.ota.state=Ota::State::Error;s.ota.error=Ota::Error(error);Ui::update(s);lv_obj_update_layout(lv_scr_act());lv_refr_now(nullptr);
   for(uint32_t i=0;i<lv_obj_get_child_cnt(lv_scr_act());i++){auto*l=lv_obj_get_child(lv_scr_act(),i);if(lv_obj_check_type(l,&lv_label_class)&&lv_obj_get_y(l)==180){lv_point_t size;lv_txt_get_size(&size,lv_label_get_text(l),lv_obj_get_style_text_font(l,0),0,lv_obj_get_style_text_line_space(l,0),lv_obj_get_width(l),LV_TEXT_FLAG_NONE);if(size.y>lv_obj_get_height(l))fprintf(stderr,"Clipped OTA error %d lang %d: %s (%d > %d)\n",error,lang,lv_label_get_text(l),size.y,lv_obj_get_height(l));assert(size.y<=lv_obj_get_height(l));}}
  }
  s.ota.error=Ota::Error::Tls;s.ota.detail=-9984;Ui::update(s);
  const char*tlsTexts[]={"Błąd HTTPS/TLS. Kod: -9984","HTTPS/TLS-Fehler. Code: -9984","HTTPS/TLS error. Code: -9984"};assert(findLabel(lv_scr_act(),tlsTexts[lang]));
  snprintf(name,sizeof name,"%s/%sota_error_%d.ppm",argv[1],theme?"light_":"",lang);save(name);
  s.ota.error=Ota::Error::Memory;s.ota.detail=32512;Ui::update(s);const char*ramTexts[]={"Brak pamięci dla HTTPS.\nKod: 32512","HTTPS: Speicher fehlt.\nCode: 32512","HTTPS: not enough RAM.\nCode: 32512"};assert(findLabel(lv_scr_act(),ramTexts[lang]));
  snprintf(name,sizeof name,"%s/%sota_memory_%d.ppm",argv[1],theme?"light_":"",lang);save(name);
  s.ota=Ota::Status{};Ui::update(s);
 }
 }
 s.theme=ThemeMode::Dark;Ui::update(s);capture=false;
 puts("OTA PL/DE/EN, light/dark: explicit check, two-click version-bound install, progress, disabled controls, blocked back and error states OK.");
 // Actual theme picker actions; no credentials/defaults are rewritten by UI.
 capture=true;s.settings.language=0;s.theme=ThemeMode::Dark;s.networkCount=5;s.wifiScan=WifiScanState::Done;s.wifiScanGeneration++;Ui::update(s);Ui::show(Screen::Settings);click("Wygląd");Ui::update(s);assert(Ui::current()==Screen::Appearance);
 sentCount=0;tap(40,115);assert(sentCount==1&&sent[0].action==Action::Setting&&sent[0].value==Appearance&&!strcmp(sent[0].text,"1"));s.theme=ThemeMode::Light;Ui::update(s);assert(Ui::current()==Screen::Appearance);
 sentCount=0;tap(120,115);assert(sentCount==0);tap(120,215);assert(sentCount==1&&sent[0].value==Appearance&&!strcmp(sent[0].text,"0"));s.theme=ThemeMode::Dark;Ui::update(s);tap(23,46);Ui::update(s);assert(Ui::current()==Screen::Settings);
 Ui::show(Screen::Display);click("Wygląd");Ui::update(s);assert(Ui::current()==Screen::Appearance);tap(23,46);Ui::update(s);assert(Ui::current()==Screen::Display);
 // Preserve a Wi-Fi password draft, masking, selected network and symbol page
 // if the theme changes while an editor is open.
 Ui::show(Screen::WifiNetworks);click("Home-24G");input=findType(lv_scr_act(),&lv_textarea_class);kb=findType(lv_scr_act(),&lv_keyboard_class);lv_textarea_set_text(input,"Example-Password");pressKey(kb,"1#");pressKey(kb,"#+");s.theme=ThemeMode::Light;Ui::update(s);input=findType(lv_scr_act(),&lv_textarea_class);kb=findType(lv_scr_act(),&lv_keyboard_class);assert(input&&kb&&!strcmp(lv_textarea_get_text(input),"Example-Password")&&lv_textarea_get_password_mode(input)&&findLabel(lv_scr_act(),"Home-24G"));pressKey(kb,"!");sentCount=0;pressKey(kb,LV_SYMBOL_OK);assert(sentCount==4&&sent[1].value==WifiSsid&&!strcmp(sent[1].text,"Home-24G")&&!strcmp(sent[2].text,"Example-Password!"));Ui::update(s);
 Ui::show(Screen::Metric);s.theme=ThemeMode::Dark;Ui::update(s);assert(Ui::current()==Screen::Metric);s.theme=ThemeMode::Light;Ui::update(s);assert(Ui::current()==Screen::Metric);Ui::show(Screen::Scan);s.theme=ThemeMode::Dark;Ui::update(s);assert(Ui::current()==Screen::Scan);capture=false;
 puts("Themes: real touch, no duplicate save, immediate repaint, back paths and preserved Wi-Fi draft/context OK.");
 // Battery labels must agree after every update, in all locales and themes.
 capture=false;
 for(int theme=0;theme<2;theme++)for(int lang=0;lang<3;lang++){
  s.theme=ThemeSettings::normalize(theme);s.settings.language=lang;s.batteryPct=87;s.batteryV=4.106f;s.batteryPower=Battery::Power::Battery;Ui::update(s);Ui::show(Screen::Battery);Ui::update(s);
  assert(countLabel(lv_scr_act(),"87%")==2);assert(lv_obj_has_flag(findImage(lv_scr_act(),&img_bolt),LV_OBJ_FLAG_HIDDEN));
  s.batteryPct=90;s.batteryV=4.120f;Ui::update(s);assert(countLabel(lv_scr_act(),"90%")==2&&!findLabel(lv_scr_act(),"87%"));
  s.batteryPct=100;Ui::update(s);assert(countLabel(lv_scr_act(),"100%")==2);auto*small=findLabel(lv_scr_act(),"100%");assert(lv_txt_get_width("100%",4,lv_obj_get_style_text_font(small,0),0,LV_TEXT_FLAG_NONE)<=lv_obj_get_width(small));
  s.batteryV=4.02f;s.batteryPct=Battery::percentage(s.batteryV);s.batteryPower=Battery::Power::Charging;Ui::update(s);assert(!lv_obj_has_flag(findImage(lv_scr_act(),&img_bolt),LV_OBJ_FLAG_HIDDEN));
  auto*a=lv_snapshot_take(lv_scr_act(),LV_IMG_CF_TRUE_COLOR);assert(a);lv_tick_inc(450);Ui::update(s);auto*b=lv_snapshot_take(lv_scr_act(),LV_IMG_CF_TRUE_COLOR);assert(b&&a->header.w==240&&b->header.w==240&&memcmp(a->data,b->data,240*320*sizeof(lv_color_t)));lv_snapshot_free(a);lv_snapshot_free(b);
  char n[512];snprintf(n,sizeof n,"%s/battery_charging_%d_%d.ppm",argv[1],theme,lang);save(n);
  s.batteryPower=Battery::Power::External;s.batteryV=4.12f;s.batteryPct=Battery::percentage(s.batteryV);Ui::update(s);a=lv_snapshot_take(lv_scr_act(),LV_IMG_CF_TRUE_COLOR);lv_tick_inc(450);Ui::update(s);b=lv_snapshot_take(lv_scr_act(),LV_IMG_CF_TRUE_COLOR);if(a&&b&&memcmp(a->data,b->data,240*320*sizeof(lv_color_t))){int left=240,right=0,top=320,bottom=0,count=0;for(int i=0;i<240*320;i++)if(((lv_color_t*)a->data)[i].full!=((lv_color_t*)b->data)[i].full){left=std::min(left,i%240);right=std::max(right,i%240);top=std::min(top,i/240);bottom=std::max(bottom,i/240);count++;}fprintf(stderr,"Powered battery moved: theme=%d lang=%d pixels=%d area=%d,%d..%d,%d\n",theme,lang,count,left,top,right,bottom);}assert(a&&b&&a->header.w==240&&b->header.w==240&&!memcmp(a->data,b->data,240*320*sizeof(lv_color_t)));lv_snapshot_free(a);lv_snapshot_free(b);
  snprintf(n,sizeof n,"%s/battery_external_%d_%d.ppm",argv[1],theme,lang);save(n);lv_obj_update_layout(lv_scr_act());if(scrollBody()&&lv_obj_get_scroll_bottom(scrollBody())>0)fprintf(stderr,"Battery overflow theme=%d lang=%d scroll=%d\n",theme,lang,int(lv_obj_get_scroll_bottom(scrollBody())));assert(scrollBody()&&lv_obj_get_scroll_bottom(scrollBody())<=0);
  s.batteryPct=-1;s.batteryV=NAN;s.batteryPower=Battery::Power::Battery;Ui::update(s);assert(countLabel(lv_scr_act(),"—")==2&&findLabel(lv_scr_act(),"— V"));
  s.batteryPct=80;s.batteryV=3.98f;Ui::update(s);Ui::show(Screen::Home);Ui::update(s);assert(!findLabel(lv_scr_act(),"12.10.2026")&&!findLabel(lv_scr_act(),"Gotowy"));
  Ui::show(Screen::Sensors);Ui::update(s);lv_obj_update_layout(lv_scr_act());checkTargets(lv_scr_act());
  for(const char*v:{"23.4 °C","54 %","742 ppm","91 %"}){auto*l=findLabel(lv_scr_act(),v);assert(l);lv_area_t area;lv_obj_get_coords(l,&area);assert(area.y1>=72&&area.y2<320);}
  s.nowMs=200000;Ui::update(s);assert(countLabel(lv_scr_act(),"—")==4);s.nowMs=10000;Ui::update(s);

 }
 s.theme=ThemeMode::Dark;s.settings.language=0;s.batteryPct=80;s.batteryV=3.98f;s.batteryPower=Battery::Power::Battery;Ui::update(s);
 puts("Battery UI: synchronized percentages, 100% width, animated charging, static powered bolt, invalid values and one-page diagnostics PL/DE/EN. Saved sensor readings update live and expire individually.");
 // Percentages from unknown through 100%, with and without the bolt, must
 // fit immediately in every header; no delayed Wi-Fi shift on the next tick.
 capture=false;s.settings.language=0;strcpy(s.timeText,"23:59");
 for(int theme=0;theme<2;theme++)for(auto screen:{Screen::Home,Screen::Settings,Screen::Battery,Screen::Live})for(auto power:{Battery::Power::Battery,Battery::Power::External})for(int pct:{0,9,49,100,-1}){
  s.theme=ThemeSettings::normalize(theme);s.batteryPower=power;s.batteryPct=pct;Ui::update(s);Ui::show(screen);Ui::update(s);checkHeader(s);
 }
 s.theme=ThemeMode::Dark;s.batteryPower=Battery::Power::Battery;s.batteryPct=80;strcpy(s.timeText,"14:26");Ui::update(s);
 puts("Battery header: immediate 4px percentage gap, 8px Wi-Fi gap, no overlap/clipping through 100%, centered segments and actual glyph alignment in both themes OK.");
 // Camera orientation uses the real LVGL software rotation and raw portrait
 // touch coordinates. The SPI spy framebuffer must match every video pixel.
 capture=true;s.settings.language=0;s.theme=ThemeMode::Dark;s.camera=s.audioCap=s.motionCap=s.sd=true;s.playing=s.recording=s.listening=false;Ui::update(s);
 Ui::show(Screen::Sensors);sentCount=0;tap(60,175);Ui::update(s);assert(Ui::current()==Screen::Metric&&findLabel(lv_scr_act(),"Temperatura"));
 Ui::show(Screen::CameraSettings);assert(!findLabel(lv_scr_act(),"Alarm dźwięku")&&!findLabel(lv_scr_act(),"Głośność"));Ui::show(Screen::AudioSettings);assert(findLabel(lv_scr_act(),"Alarm dźwięku")&&findLabel(lv_scr_act(),"Głośność"));
 static uint16_t fixture[Video::pixels];for(int y=0;y<Video::height;y++)for(int x=0;x<Video::width;x++){uint32_t c=x<48?0xb9364b:x>=272?0x386daa:((x/32+y/24)%2?0x8ba79a:0xd1dfce);fixture[y*Video::width+x]=lv_color_hex(c).full;}
 for(int theme=0;theme<2;theme++)for(int lang=0;lang<3;lang++){
  s.settings.language=lang;s.theme=ThemeSettings::normalize(theme);Ui::update(s);Ui::show(Screen::Live);Ui::frame(fixture);Ui::update(s);lv_obj_update_layout(lv_scr_act());checkTargets(lv_scr_act());assert(lv_disp_get_rotation(nullptr)==LV_DISP_ROT_NONE&&lv_obj_get_width(lv_scr_act())==240);
  char name[512];snprintf(name,sizeof name,"%s/camera_portrait_%d_%d.ppm",argv[1],theme,lang);save(name);
  sentCount=0;tap(90,54);Ui::update(s);lv_obj_update_layout(lv_scr_act());assert(sentCount==0&&lv_disp_get_rotation(nullptr)==LV_DISP_ROT_90&&lv_obj_get_width(lv_scr_act())==320&&lv_obj_get_height(lv_scr_act())==240);
  assert(lv_obj_has_flag(lv_obj_get_parent(lv_obj_get_parent(findImage(lv_scr_act(),&img_back))),LV_OBJ_FLAG_HIDDEN));lv_refr_now(nullptr);
  for(int y=0;y<240;y++)for(int x=0;x<320;x++)assert(nativePixels[(319-x)*240+y].full==fixture[y*320+x]);
  snprintf(name,sizeof name,"%s/camera_landscape_%d_%d.ppm",argv[1],theme,lang);save(name);
  tap(160,125);checkHeader(s);assert(!lv_obj_has_flag(lv_obj_get_parent(lv_obj_get_parent(findImage(lv_scr_act(),&img_back))),LV_OBJ_FLAG_HIDDEN));checkTargets(lv_scr_act());snprintf(name,sizeof name,"%s/camera_landscape_controls_%d_%d.ppm",argv[1],theme,lang);save(name);
  tap(290,54);auto*sl=findType(lv_scr_act(),&lv_slider_class);assert(sl);checkTargets(lv_scr_act());lv_slider_set_value(sl,72,LV_ANIM_OFF);lv_event_send(sl,LV_EVENT_VALUE_CHANGED,nullptr);assert(findLabel(lv_scr_act(),"72%"));sentCount=0;lv_event_send(sl,LV_EVENT_RELEASED,nullptr);assert(sentCount==1&&sent[0].action==Action::Setting&&sent[0].value==Volume&&!strcmp(sent[0].text,"72"));
  snprintf(name,sizeof name,"%s/camera_landscape_volume_%d_%d.ppm",argv[1],theme,lang);save(name);clickImage(&img_close);
  sentCount=0;tap(200,210);assert(sentCount==1&&sent[0].action==Action::Record&&sent[0].value==1);s.recording=true;s.clipMs=12000;Ui::update(s);assert(findImage(lv_scr_act(),&img_stop)&&findLabel(lv_scr_act(),"00:12"));sentCount=0;tap(200,210);assert(sentCount==1&&sent[0].action==Action::Record&&sent[0].value==0);s.recording=false;Ui::update(s);
  sentCount=0;tap(120,210);assert(sentCount==1&&sent[0].action==Action::Snapshot);sentCount=0;tap(280,210);assert(sentCount==1&&sent[0].action==Action::Listen&&sent[0].value==1);
  sentCount=0;pointerStep(40,210,true);pointerStep(40,210,true);pointerStep(40,210,false);pointerStep(40,210,false);assert(sentCount==2&&sent[0].action==Action::Talk&&sent[0].value==1&&sent[1].value==0);
  // Rebuild/theme changes retain the landscape state and an open volume draft.
  tap(290,54);sl=findType(lv_scr_act(),&lv_slider_class);lv_slider_set_value(sl,81,LV_ANIM_OFF);s.theme=theme?ThemeMode::Dark:ThemeMode::Light;Ui::update(s);sl=findType(lv_scr_act(),&lv_slider_class);assert(sl&&lv_slider_get_value(sl)==81&&lv_disp_get_rotation(nullptr)==LV_DISP_ROT_90);clickImage(&img_close);s.theme=ThemeSettings::normalize(theme);Ui::update(s);
  tap(234,54);Ui::update(s);assert(Ui::current()==Screen::CameraSettings&&lv_disp_get_rotation(nullptr)==LV_DISP_ROT_NONE);tap(30,46);Ui::update(s);assert(Ui::current()==Screen::Live&&lv_disp_get_rotation(nullptr)==LV_DISP_ROT_90);
  tap(90,54);Ui::update(s);assert(lv_disp_get_rotation(nullptr)==LV_DISP_ROT_NONE&&lv_obj_get_width(lv_scr_act())==240);sentCount=0;tap(150,290);assert(sentCount==1&&sent[0].action==Action::Record);tap(30,54);Ui::update(s);assert(Ui::current()==Screen::Cameras&&lv_disp_get_rotation(nullptr)==LV_DISP_ROT_NONE);
 }
 for(int lang=0;lang<3;lang++){s.settings.language=lang;s.theme=ThemeMode::Light;s.camera=s.audioCap=false;Ui::update(s);Ui::show(Screen::Live);Ui::update(s);assert(findLabel(lv_scr_act(),lang==0?"Brak obrazu":lang==1?"Kein Bild":"No video"));sentCount=0;tap(150,290);assert(sentCount==0);
  char n[512];snprintf(n,sizeof n,"%s/camera_offline_portrait_%d.ppm",argv[1],lang);save(n);tap(90,54);Ui::update(s);assert(lv_disp_get_rotation(nullptr)==LV_DISP_ROT_90);snprintf(n,sizeof n,"%s/camera_offline_landscape_%d.ppm",argv[1],lang);save(n);sentCount=0;tap(200,210);assert(sentCount==0);
  click(lang==0?"Połączenie":lang==1?"Verbindung":"Connection");Ui::update(s);assert(Ui::current()==Screen::Network&&lv_disp_get_rotation(nullptr)==LV_DISP_ROT_NONE);tap(23,46);Ui::update(s);assert(Ui::current()==Screen::Live&&lv_disp_get_rotation(nullptr)==LV_DISP_ROT_90);tap(30,54);Ui::update(s);
 }
 s.settings.language=0;s.playing=true;Ui::update(s);Ui::show(Screen::Live);Ui::frame(fixture);sentCount=0;tap(30,54);Ui::update(s);assert(sentCount==2&&sent[0].action==Action::Play&&sent[0].value==0&&sent[1].action==Action::LoadFiles&&Ui::current()==Screen::Cameras);s.playing=false;s.theme=ThemeMode::Dark;Ui::update(s);capture=false;
 puts("Camera PL/DE/EN light/dark: real rotation/flush pixel mapping, native touch coordinates, portrait restoration, independent toolbars, volume draft, snapshot/record/audio and offline connection action OK.");
 // Stress rebuilds and missing values, including high CO2 (32-bit chart coordinates).
 for(int i=0;i<150;i++){s.camera=i%2;s.alertCount=i%3;s.co2=5000;s.history[2][59]=5000;Ui::update(s);Ui::show(Screen::Metric,2);Ui::update(s);lv_tick_inc(20);lv_timer_handler();}
 s.bmeFresh=s.scdFresh=false;s.temperature=s.humidity=s.co2=NAN;Ui::show(Screen::Metric);Ui::update(s);char name[512];snprintf(name,sizeof name,"%s/ui_offline.ppm",argv[1]);save(name);
 printf("Rendered 150 light/dark screens, 6 password views, 3 scan states, offline and 150 transitions. Wi-Fi/password/language callbacks OK.\n");return 0;}
