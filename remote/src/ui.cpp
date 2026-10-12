#include "ui.h"
#include "assets.h"
#include "board_config.h"
#include "ui_palette.h"
#include "video_geometry.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
extern "C" { LV_FONT_DECLARE(font_ui12);LV_FONT_DECLARE(font_ui14);LV_FONT_DECLARE(font_ui16);LV_FONT_DECLARE(font_ui18);LV_FONT_DECLARE(font_ui20);LV_FONT_DECLARE(font_ui22);LV_FONT_DECLARE(font_body18); }
namespace Ui {
static AppState data;static Dispatch send=nullptr;static Screen page=Screen::Home,pending=Screen::Home;
static int metric=0,pendingMetric=0;static bool rebuild=false;static Screen appearanceBack=Screen::Settings,cameraSettingsBack=Screen::Settings;
static lv_obj_t *root,*body,*clockLabel,*batteryLabel,*wifiIcon,*notice,*image,*valueLabel,*onlineLabel,*minLabel,*maxLabel,*chart,*recordLabel,*sensorInfo,*wave;
static lv_obj_t*sensorValues[6]={};static lv_obj_t*sensorStates[6]={};
static lv_chart_series_t*series;static lv_color_t*imageBuf=nullptr;
static bool frameReady=false,liveFit=false,hudVisible=true,volumeOpen=false;
static lv_obj_t*liveHud=nullptr,*liveEmpty=nullptr,*liveEmptyTitle=nullptr,*liveEmptyHint=nullptr,*liveStatus=nullptr,*liveSettings=nullptr,*liveButtons[4]={},*liveGlyphs[4]={},*volumeSlider=nullptr;
static void openVolume();static void closeVolume();static void refreshLive();static void positionLive();
static lv_obj_t *otaCheckButton=nullptr,*otaInstallButton=nullptr,*otaProgress=nullptr,*otaPercent=nullptr;
static bool otaConfirm=false;static char otaSelected[24]={};
static lv_img_dsc_t liveImage={{LV_IMG_CF_TRUE_COLOR,0,0,Video::width,Video::height},Video::bytes,nullptr};
static uint32_t lastFrames=0,noticeUntil=0;static char lastMessage[120]={};
static lv_obj_t *editor=nullptr,*keyboard=nullptr,*overlay=nullptr;static int editorKey=0;
static lv_obj_t *editorHint=nullptr,*revealLabel=nullptr;static bool joiningWifi=false;static char joinSsid[33]={};
// Semantic colors cover controls, states and the keyboard in both themes.
// The default dark home retains its approved colors and geometry.
static uint32_t BG,CARD,BORDER,TEXT,MUTED,LIME,RED,BLUE,SELECTED,PRESSED,BUTTON_PRESSED,ACCENT_PRESSED,DISABLED,TRACK,KNOB,ACCENT_INK,ERROR_INK,KNOB_DISABLED,KEY_BG,KEY_PRESSED,INACTIVE_DOT;
static void palette(){auto p=UiColors::get(data.theme,page==Screen::Home);
 BG=p.background;CARD=p.card;BORDER=p.border;TEXT=p.text;MUTED=p.muted;LIME=p.accent;RED=p.error;BLUE=p.blue;SELECTED=p.selected;PRESSED=p.pressed;BUTTON_PRESSED=p.buttonPressed;ACCENT_PRESSED=p.accentPressed;DISABLED=p.disabled;TRACK=p.track;KNOB=p.knob;ACCENT_INK=p.accentInk;ERROR_INK=p.errorInk;KNOB_DISABLED=p.knobDisabled;KEY_BG=p.key;KEY_PRESSED=p.keyPressed;INACTIVE_DOT=p.inactiveDot;
}
static const char*themeName(){return data.theme==ThemeMode::Light?(data.settings.language==1?"Hell":data.settings.language==2?"Light":"Jasny"):(data.settings.language==1?"Dunkel":data.settings.language==2?"Dark":"Ciemny");}
struct Binding{int kind,key;Screen screen;Action action;char text[80];};static Binding binds[110];static int bindCount=0;
static const char*tr(const char*pl,const char*de,const char*en){return data.settings.language==1?de:data.settings.language==2?en:pl;}
static lv_color_t col(uint32_t c){return lv_color_hex(c);}
static const lv_font_t*rowFont(const char*s,int width){for(const char*p=s;*p;){const char*end=p;while(*end&&*end!=' '&&*end!='\n')end++;if(lv_txt_get_width(p,end-p,&font_ui20,0,LV_TEXT_FLAG_NONE)>width)return &font_ui18;p=*end?end+1:end;}return &font_ui20;}
static const char*names(int m){switch(m){case 0:return tr("Temperatura","Temperatur","Temperature");case 1:return tr("Wilgotność","Luftfeuchtigkeit","Humidity");case 2:return "CO₂ · SCD41";case 3:return "IAQ · BME688";case 4:return tr("Ciśnienie","Luftdruck","Pressure");default:return "eCO₂ · BME688";}}
static const char*unit(int m){const char*u[]={"°C","%","ppm","IAQ","hPa","ppm"};return u[m];}
static const lv_img_dsc_t*metricIcon(int m){const lv_img_dsc_t*a[]={&img_temp,&img_drop,&img_co2,&img_air,&img_pressure,&img_co2};return a[m];}
static void style(lv_obj_t*o,uint32_t bg,int radius=8,int border=1){if(page!=Screen::Home){if(radius==8)radius=14;if(border==1)border=data.theme==ThemeMode::Light?2:0;}lv_obj_set_style_bg_color(o,col(bg),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_border_color(o,col(BORDER),0);lv_obj_set_style_border_width(o,data.theme==ThemeMode::Light&&page==Screen::Home&&border==1?2:border,0);lv_obj_set_style_radius(o,radius,0);lv_obj_set_style_pad_all(o,0,0);lv_obj_set_style_shadow_width(o,0,0);}
static lv_obj_t*label(lv_obj_t*p,const char*s,int x,int y,int width=0,const lv_font_t*f=&font_ui20,uint32_t c=TEXT){auto*o=lv_label_create(p);lv_label_set_text(o,s);lv_obj_set_style_text_font(o,f,0);lv_obj_set_style_text_color(o,col(c),0);lv_obj_set_pos(o,x,y);if(width){lv_obj_set_width(o,width);if(strchr(s,'\n'))lv_label_set_long_mode(o,LV_LABEL_LONG_WRAP);else{lv_label_set_long_mode(o,LV_LABEL_LONG_DOT);lv_obj_set_height(o,f->line_height);}}return o;}
static int wrappedHeight(const char*s,int width,const lv_font_t*f){lv_point_t size;lv_txt_get_size(&size,s,f,0,2,width,LV_TEXT_FLAG_NONE);return size.y;}
static lv_obj_t*wrapped(lv_obj_t*p,const char*s,int x,int y,int width,const lv_font_t*f=&font_body18,uint32_t c=TEXT){auto*l=label(p,s,x,y,width,f,c);lv_obj_set_style_text_line_space(l,2,0);lv_label_set_long_mode(l,LV_LABEL_LONG_WRAP);lv_obj_set_height(l,LV_SIZE_CONTENT);return l;}
// List pages grow vertically; controls are never squeezed to fit 240 x 320.
static int flowY(){lv_obj_update_layout(body);int end=-8;for(uint32_t i=0;i<lv_obj_get_child_cnt(body);i++){auto*c=lv_obj_get_child(body,i);end=std::max(end,int(lv_obj_get_y(c)+lv_obj_get_height(c)));}return end+8;}
static lv_obj_t*flowText(const char*s,const lv_font_t*f=&font_body18,uint32_t c=MUTED){return wrapped(body,s,8,flowY(),208,f,c);}
static lv_obj_t*icon(lv_obj_t*p,const lv_img_dsc_t*i,int x,int y,uint32_t c=TEXT,int zoom=256){auto*o=lv_img_create(p);lv_img_set_src(o,i);lv_obj_clear_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_pos(o,x,y);lv_img_set_zoom(o,zoom);lv_obj_set_style_img_recolor(o,col(c),0);lv_obj_set_style_img_recolor_opa(o,LV_OPA_COVER,0);return o;}
static lv_obj_t*box(lv_obj_t*p,int x,int y,int w,int h,uint32_t bg=CARD){auto*o=lv_obj_create(p);style(o,bg);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;}
static void next(Screen s,int m=0){pending=s;pendingMetric=m;rebuild=true;}
static void connectSelected(const char*password){
 bool direct=!strcmp(joinSsid,"ESP32-BABY-CAM")||(!strcmp(joinSsid,data.settings.ssid)&&data.settings.networkMode==0);
 send(Action::Setting,NetworkMode,direct?"0":"1");send(Action::Setting,WifiSsid,joinSsid);send(Action::Setting,WifiPassword,password);send(Action::Connect,0,"");next(Screen::Network);
}
static bool validPassword(const char*p){size_t n=strlen(p);if(n>=8&&n<=63)return true;if(n!=64)return false;for(size_t i=0;i<n;i++)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')||(p[i]>='A'&&p[i]<='F')))return false;return true;}
static void closeEdit(){if(overlay)lv_obj_del(overlay);overlay=editor=keyboard=editorHint=revealLabel=nullptr;joiningWifi=false;}
static void commitEdit(){if(!editor)return;const char*t=lv_textarea_get_text(editor);
 if(joiningWifi&&!validPassword(t)){lv_label_set_text(editorHint,tr("Nieprawidłowe hasło","Ungültiges Passwort","Invalid password"));lv_obj_set_style_text_color(editorHint,col(RED),0);return;}
 if(editorKey==WifiSsid&&(!t[0]||strlen(t)>32)){lv_label_set_text(editorHint,tr("SSID: 1–32 bajty","SSID: 1–32 Bytes","SSID: 1–32 bytes"));return;}
 if(strlen(t)>(editorKey==WifiPassword?64:editorKey==WifiSsid?32:63)){lv_label_set_text(editorHint,tr("Za długa wartość","Wert zu lang","Value too long"));return;}
 if(joiningWifi)connectSelected(t);else{send(Action::Setting,editorKey,t);next(page,metric);}closeEdit();
}
// Five columns keep every key at least 44 px wide. Alphabet and symbols have
// pages rather than shrinking a ten-column desktop keyboard to the tiny LCD.
static const char*keysLowerA[]={"a","b","c","d","e","\n","f","g","h","i","j","\n","k","l","m","n-z","ABC","\n","1#",LV_SYMBOL_BACKSPACE," ",LV_SYMBOL_CLOSE,LV_SYMBOL_OK,""};
static const char*keysLowerN[]={"n","o","p","q","r","\n","s","t","u","v","w","\n","x","y","z","a-m","ABC","\n","1#",LV_SYMBOL_BACKSPACE," ",LV_SYMBOL_CLOSE,LV_SYMBOL_OK,""};
static const char*keysUpperA[]={"A","B","C","D","E","\n","F","G","H","I","J","\n","K","L","M","N-Z","abc","\n","1#",LV_SYMBOL_BACKSPACE," ",LV_SYMBOL_CLOSE,LV_SYMBOL_OK,""};
static const char*keysUpperN[]={"N","O","P","Q","R","\n","S","T","U","V","W","\n","X","Y","Z","A-M","abc","\n","1#",LV_SYMBOL_BACKSPACE," ",LV_SYMBOL_CLOSE,LV_SYMBOL_OK,""};
static const char*keysNumbers[]={"0","1","2","3","4","\n","5","6","7","8","9","\n",".","-","_","@","#+","\n","abc",LV_SYMBOL_BACKSPACE," ",LV_SYMBOL_CLOSE,LV_SYMBOL_OK,""};
static const char*keysSymbolsA[]={"!","\"","#","$","%","\n","&","'","(",")","*","\n","+",",",":",";","?+","\n","abc",LV_SYMBOL_BACKSPACE," ",LV_SYMBOL_CLOSE,LV_SYMBOL_OK,""};
static const char*keysSymbolsB[]={"<","=",">","?","[","\n","\\","]","^","`","{","\n","|","}","~","/","0-9","\n","abc",LV_SYMBOL_BACKSPACE," ",LV_SYMBOL_CLOSE,LV_SYMBOL_OK,""};
static lv_btnmatrix_ctrl_t keyControls[20];static int keyPage=0;static bool keyUpper=false;
static void keyboardMap(){const char**map=keyPage==0?(keyUpper?keysUpperA:keysLowerA):keyPage==1?(keyUpper?keysUpperN:keysLowerN):keyPage==2?keysNumbers:keyPage==3?keysSymbolsA:keysSymbolsB;lv_keyboard_set_map(keyboard,LV_KEYBOARD_MODE_USER_1,map,keyControls);lv_keyboard_set_mode(keyboard,LV_KEYBOARD_MODE_USER_1);}
static void keyboardEvent(lv_event_t*e){auto code=lv_event_get_code(e);if(code==LV_EVENT_READY){commitEdit();return;}if(code==LV_EVENT_CANCEL){closeEdit();next(page,metric);return;}if(code!=LV_EVENT_VALUE_CHANGED||!keyboard||!editor)return;
 auto id=lv_btnmatrix_get_selected_btn(keyboard);if(id==LV_BTNMATRIX_BTN_NONE)return;const char*t=lv_btnmatrix_get_btn_text(keyboard,id);if(!t)return;
 if(!strcmp(t,LV_SYMBOL_OK)){lv_event_send(keyboard,LV_EVENT_READY,nullptr);return;}
 if(!strcmp(t,LV_SYMBOL_CLOSE)){lv_event_send(keyboard,LV_EVENT_CANCEL,nullptr);return;}
 if(!strcmp(t,LV_SYMBOL_BACKSPACE)){lv_textarea_del_char(editor);return;}
 if(!strcmp(t,"ABC")){keyUpper=true;keyboardMap();return;}if(!strcmp(t,"abc")){keyUpper=false;if(keyPage>=2)keyPage=0;keyboardMap();return;}
 if(!strcmp(t,"n-z")||!strcmp(t,"N-Z")){keyPage=1;keyboardMap();return;}if(!strcmp(t,"a-m")||!strcmp(t,"A-M")){keyPage=0;keyboardMap();return;}
 if(!strcmp(t,"1#")||!strcmp(t,"0-9")){keyPage=2;keyboardMap();return;}if(!strcmp(t,"#+")){keyPage=3;keyboardMap();return;}if(!strcmp(t,"?+")){keyPage=4;keyboardMap();return;}
 lv_textarea_add_text(editor,t);
}
static const char*editorTitle(int key){switch(key){case WifiSsid:return tr("Nazwa sieci","Netzname","Network name");case WifiPassword:return tr("Hasło Wi-Fi","WLAN-Passwort","Wi-Fi password");case CameraHost:return tr("Adres kamery","Kamera-Adresse","Camera address");case PairToken:return tr("Klucz parowania","Kopplungsschlüssel","Pairing key");default:return tr("Zmień wartość","Wert ändern","Change value");}}
static void edit(int key,const char*current,const char*title){
 joiningWifi=false;
 overlay=box(root,0,22,240,298,BG);style(overlay,BG,0,0);lv_obj_move_foreground(overlay);label(overlay,title,12,2,174,lv_txt_get_width(title,strlen(title),&font_ui20,0,LV_TEXT_FLAG_NONE)<=174?&font_ui20:&font_body18);
 auto*cancel=lv_btn_create(overlay);style(cancel,BG,4,0);lv_obj_set_pos(cancel,190,0);lv_obj_set_size(cancel,44,44);icon(cancel,&img_close,8,8,MUTED);lv_obj_add_event_cb(cancel,[](lv_event_t*){closeEdit();next(page,metric);},LV_EVENT_CLICKED,nullptr);
 bool secret=key==WifiPassword||key==PairToken;editor=lv_textarea_create(overlay);style(editor,CARD);lv_obj_set_pos(editor,8,44);lv_obj_set_size(editor,secret?164:224,48);lv_obj_set_style_pad_all(editor,6,0);lv_textarea_set_one_line(editor,true);lv_textarea_set_text(editor,current);lv_obj_set_style_text_font(editor,&font_ui20,0);lv_obj_set_style_text_color(editor,col(TEXT),0);
 lv_textarea_set_password_mode(editor,secret);lv_textarea_set_password_show_time(editor,0);lv_textarea_set_max_length(editor,key==WifiSsid?32:key==WifiPassword?64:63);
 editorHint=label(overlay,"",12,25,174,&font_ui14,MUTED);
 if(secret){auto*b=lv_btn_create(overlay);style(b,CARD);lv_obj_set_pos(b,176,44);lv_obj_set_size(b,56,48);revealLabel=icon(b,&img_eye,14,10);lv_obj_add_event_cb(b,[](lv_event_t*){bool hide=!lv_textarea_get_password_mode(editor);lv_textarea_set_password_mode(editor,hide);lv_img_set_src(revealLabel,hide?&img_eye:&img_eye_off);},LV_EVENT_CLICKED,nullptr);}
 editorKey=key;keyboard=lv_keyboard_create(overlay);lv_obj_remove_event_cb(keyboard,lv_keyboard_def_event_cb);lv_obj_set_align(keyboard,LV_ALIGN_TOP_LEFT);lv_obj_set_size(keyboard,240,206);lv_obj_set_pos(keyboard,0,92);lv_keyboard_set_textarea(keyboard,editor);lv_obj_set_style_bg_color(keyboard,col(BG),LV_PART_MAIN);lv_obj_set_style_bg_opa(keyboard,LV_OPA_COVER,LV_PART_ITEMS);lv_obj_set_style_bg_color(keyboard,col(KEY_BG),LV_PART_ITEMS);lv_obj_set_style_text_color(keyboard,col(TEXT),LV_PART_ITEMS);lv_obj_set_style_text_font(keyboard,&lv_font_montserrat_18,LV_PART_ITEMS);lv_obj_set_style_pad_all(keyboard,2,LV_PART_MAIN);lv_obj_set_style_pad_row(keyboard,2,LV_PART_MAIN);lv_obj_set_style_pad_column(keyboard,2,LV_PART_MAIN);lv_obj_set_style_border_width(keyboard,0,LV_PART_MAIN);lv_obj_set_style_border_color(keyboard,col(BORDER),LV_PART_ITEMS);lv_obj_set_style_border_width(keyboard,0,LV_PART_ITEMS);lv_obj_set_style_radius(keyboard,8,LV_PART_ITEMS);lv_obj_set_style_bg_color(keyboard,col(KEY_PRESSED),LV_PART_ITEMS|LV_STATE_PRESSED);for(auto&control:keyControls)control=1;keyUpper=false;keyPage=key<=IaqMax&&key>=TempMin?2:0;keyboardMap();
 lv_obj_add_event_cb(keyboard,keyboardEvent,LV_EVENT_ALL,nullptr);
}
static void joinNetwork(int index){if(index<0||index>=data.networkCount)return;auto&n=data.networks[index];if(n.security==WifiSecurity::Unsupported)return;
 snprintf(joinSsid,sizeof joinSsid,"%s",n.ssid);
 if(n.security==WifiSecurity::Open){connectSelected("");return;}
 edit(WifiPassword,!strcmp(n.ssid,data.settings.ssid)?data.settings.password:"",tr("Hasło Wi-Fi","WLAN-Passwort","Wi-Fi password"));joiningWifi=true;
 lv_label_set_text(editorHint,joinSsid);lv_label_set_long_mode(editorHint,LV_LABEL_LONG_SCROLL_CIRCULAR);
}
static void event(lv_event_t*e){auto*b=(Binding*)lv_event_get_user_data(e);auto code=lv_event_get_code(e);auto*o=lv_event_get_target(e);
 if(b->kind==0&&code==LV_EVENT_CLICKED&&!Ota::installing(data.ota.state)){if(b->screen==Screen::Appearance)appearanceBack=page;if(b->screen==Screen::CameraSettings)cameraSettingsBack=page;next(b->screen,b->key);}
 if(b->kind==1&&code==LV_EVENT_VALUE_CHANGED){char n[16];snprintf(n,sizeof n,"%d",lv_obj_has_state(o,LV_STATE_CHECKED));send(Action::Setting,b->key,n);}
 if(b->kind==2&&code==LV_EVENT_CLICKED)send(b->action,b->key,b->text);
 if(b->kind==3&&code==LV_EVENT_VALUE_CHANGED)send(Action::CameraControl,lv_obj_has_state(o,LV_STATE_CHECKED),b->text);
 if(b->kind==4&&code==LV_EVENT_RELEASED){char n[16];snprintf(n,sizeof n,"%d",lv_slider_get_value(o));send(Action::Setting,b->key,n);}
 if(b->kind==5&&code==LV_EVENT_CLICKED)edit(b->key,b->text,editorTitle(b->key));
 if(b->kind==6){if(code==LV_EVENT_PRESSED)send(Action::Talk,1,"");if(code==LV_EVENT_RELEASED||code==LV_EVENT_PRESS_LOST)send(Action::Talk,0,"");}
 if(b->kind==7&&code==LV_EVENT_CLICKED){bool on=b->action==Action::Record?data.recording:data.listening;send(b->action,!on,"");}
 if(b->kind==8&&code==LV_EVENT_CLICKED){
  // Explicit geometry/opacity: the optional LVGL theme is disabled.
  auto*shade=box(root,0,22,240,298,BG);style(shade,BG,0,0);lv_obj_move_foreground(shade);auto*dlg=box(shade,8,12,224,274,CARD);label(dlg,tr("Reset ustawień","Zurücksetzen","Reset settings"),12,14,150,&font_ui20);
  auto*close=lv_btn_create(dlg);style(close,CARD,4,0);lv_obj_set_pos(close,168,8);lv_obj_set_size(close,44,44);icon(close,&img_close,8,8);lv_obj_add_event_cb(close,[](lv_event_t*e){lv_obj_del((lv_obj_t*)lv_event_get_user_data(e));},LV_EVENT_CLICKED,shade);
  wrapped(dlg,tr("Usunąć ustawienia pilota?","Einstellungen löschen?","Erase remote settings?"),12,68,200,&font_ui20);
  wrapped(dlg,tr("Pliki na SD pozostają.","Dateien auf SD bleiben.","SD files are kept."),12,144,200,&font_body18,MUTED);
  auto*bt=lv_btn_create(dlg);lv_obj_set_size(bt,200,64);lv_obj_set_pos(bt,12,202);style(bt,RED);auto*l=wrapped(bt,tr("Usuń ustawienia","Löschen","Erase settings"),8,0,184,&font_ui20,ERROR_INK);lv_obj_center(l);lv_obj_add_event_cb(bt,[](lv_event_t*){send(Action::Reset,0,"");},LV_EVENT_CLICKED,nullptr);
 }
 if(b->kind==9&&code==LV_EVENT_CLICKED){send(Action::WifiScan,0,"");next(Screen::WifiNetworks);}
 if(b->kind==10&&code==LV_EVENT_CLICKED){char n[4];snprintf(n,sizeof n,"%d",b->key);send(Action::Setting,Language,n);next(Screen::Languages);}
 if(b->kind==11&&code==LV_EVENT_CLICKED)joinNetwork(b->key);
 if(b->kind==12&&code==LV_EVENT_CLICKED){send(Action::Setting,NetworkMode,data.settings.networkMode?"0":"1");next(Screen::Network);}
 if(b->kind==13&&code==LV_EVENT_CLICKED){otaConfirm=false;send(Action::OtaCheck,0,"");}
 if(b->kind==14&&code==LV_EVENT_CLICKED&&data.ota.state==Ota::State::Available){
  if(!otaConfirm){otaConfirm=true;snprintf(otaSelected,sizeof otaSelected,"%s",data.ota.version);lv_label_set_text(lv_obj_get_child(otaInstallButton,0),tr("Potwierdź","Bestätigen","Confirm"));lv_label_set_text(onlineLabel,tr("Zainstalować tę wersję?","Diese Version installieren?","Install this version?"));}
  else {otaConfirm=false;send(Action::OtaInstall,0,otaSelected);}
 }
 if(b->kind==15&&code==LV_EVENT_CLICKED){auto selected=ThemeSettings::normalize(b->key);if(selected==data.theme)return;send(Action::Setting,Appearance,selected==ThemeMode::Light?"1":"0");next(Screen::Appearance);}

 if(b->kind==16&&code==LV_EVENT_CLICKED){liveFit=!liveFit;positionLive();}
 if(b->kind==17&&code==LV_EVENT_CLICKED)openVolume();

}
static void bind(lv_obj_t*o,int kind,int key=0,Screen s=Screen::Home,Action a=Action::Setting,const char*t=""){
 if(bindCount>=110)return;auto&b=binds[bindCount++];b.kind=kind;b.key=key;b.screen=s;b.action=a;snprintf(b.text,sizeof b.text,"%s",t);lv_obj_add_event_cb(o,event,LV_EVENT_ALL,&b);
 if(page!=Screen::Home&&lv_obj_check_type(o,&lv_obj_class)){lv_obj_set_style_bg_color(o,col(PRESSED),LV_STATE_PRESSED);lv_obj_set_style_opa(o,LV_OPA_60,LV_STATE_DISABLED);}
}
static lv_obj_t*button(lv_obj_t*p,const char*s,int x,int y,int w,int h,bool accent=false){if(p==body&&page!=Screen::Home){y=std::max(y,flowY());h=std::max(h,64);}auto*o=lv_btn_create(p);style(o,accent?LIME:CARD);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_set_style_bg_color(o,col(accent?ACCENT_PRESSED:BUTTON_PRESSED),LV_STATE_PRESSED);lv_obj_set_style_bg_color(o,col(DISABLED),LV_STATE_DISABLED);lv_obj_set_style_border_color(o,col(BORDER),LV_STATE_DISABLED);lv_obj_set_style_opa(o,page==Screen::Home?LV_OPA_80:LV_OPA_60,LV_STATE_DISABLED);lv_obj_set_style_text_color(o,col(accent?ACCENT_INK:TEXT),0);lv_obj_set_style_text_color(o,col(MUTED),LV_STATE_DISABLED);
 const lv_font_t*f=page==Screen::Home?(lv_txt_get_width(s,strlen(s),&font_ui16,0,LV_TEXT_FLAG_NONE)<=w-12?&font_ui16:&font_ui14):rowFont(s,w-16);
 if(page!=Screen::Home&&lv_txt_get_width(s,strlen(s),f,0,LV_TEXT_FLAG_NONE)>w-16&&lv_txt_get_width(s,strlen(s),&font_body18,0,LV_TEXT_FLAG_NONE)<=w-16)f=&font_body18;
 if(page!=Screen::Home&&wrappedHeight(s,w-16,f)>h-6)f=&font_body18;
 auto*l=page==Screen::Home?label(o,s,4,0,w-8,f):wrapped(o,s,8,0,w-16,f);lv_obj_remove_local_style_prop(l,LV_STYLE_TEXT_COLOR,LV_PART_MAIN);lv_obj_center(l);return o;}
static lv_obj_t*row(const lv_img_dsc_t*i,const char*s,const char*right,int y,int h=64,int reserveRight=0){(void)y;const int left=i?44:12;bool value=right&&right[0]&&strcmp(right,"›");bool arrow=right&&!strcmp(right,"›");const int width=224-left-12-reserveRight-(arrow?18:0);const auto*f=rowFont(s,width);const int textHeight=wrappedHeight(s,width,f);h=std::max(std::max(64,h),textHeight+(value?font_ui18.line_height+22:20));
 auto*o=box(body,0,flowY(),224,h);if(i)icon(o,i,9,(h-28)/2);auto*l=wrapped(o,s,left,value?8:(h-textHeight)/2,width,f);
 if(value){auto*v=label(o,right,left,12+textHeight,212-left-reserveRight,&font_body18,MUTED);lv_label_set_long_mode(v,LV_LABEL_LONG_SCROLL_CIRCULAR);}
 if(arrow)icon(o,&img_right,195,(h-28)/2,MUTED,200);(void)l;return o;}
static void chevron(lv_obj_t*r){lv_obj_update_layout(r);icon(r,&img_right,195,(lv_obj_get_height(r)-28)/2,MUTED,200);}
static void navRow(const lv_img_dsc_t*i,const char*s,Screen dest,int y,const char*right="›",int key=0){auto*r=row(i,s,right,y,64,strcmp(right,"›")?18:0);if(strcmp(right,"›"))chevron(r);bind(r,0,key,dest);}
static void editRow(const lv_img_dsc_t*i,const char*s,int key,const char*value,int y,bool hide=false){const char*display=value[0]?(hide?"••••":value):tr("Nie ustawiono","Nicht gesetzt","Not set");auto*r=row(i,s,display,y,64,18);chevron(r);bind(r,5,key,Screen::Home,Action::Setting,value);}
static void numberRow(const char*s,int key,int value,int y){char n[24];snprintf(n,sizeof n,"%d",value);editRow(nullptr,s,key,n,y);}
static lv_obj_t*toggle(const lv_img_dsc_t*i,const char*s,bool on,int key,int y,const char*cameraVar=nullptr,bool enabled=true){(void)y;int left=i?44:12;auto*f=rowFont(s,212-left);int textHeight=wrappedHeight(s,212-left,f);int height=std::max(88,textHeight+54);auto*r=box(body,0,flowY(),224,height);if(i)icon(r,i,9,10);wrapped(r,s,left,10,212-left,f);auto*stateText=label(r,on?tr("Wł.","Ein","On"):tr("Wył.","Aus","Off"),left,height-36,108,&font_body18,MUTED);auto*sw=lv_switch_create(r);lv_obj_set_pos(sw,163,height-40);lv_obj_set_size(sw,52,32);lv_obj_set_ext_click_area(sw,8);lv_obj_clear_flag(sw,LV_OBJ_FLAG_EVENT_BUBBLE);lv_obj_set_style_bg_color(sw,col(TRACK),LV_PART_MAIN);lv_obj_set_style_bg_opa(sw,LV_OPA_COVER,LV_PART_MAIN);lv_obj_set_style_radius(sw,16,LV_PART_MAIN);lv_obj_set_style_bg_opa(sw,LV_OPA_COVER,LV_PART_INDICATOR);lv_obj_set_style_radius(sw,16,LV_PART_INDICATOR);lv_obj_set_style_bg_color(sw,col(KNOB),LV_PART_KNOB);lv_obj_set_style_bg_color(sw,col(ACCENT_INK),LV_PART_KNOB|LV_STATE_CHECKED);lv_obj_set_style_bg_opa(sw,LV_OPA_COVER,LV_PART_KNOB);lv_obj_set_style_radius(sw,LV_RADIUS_CIRCLE,LV_PART_KNOB);lv_obj_set_style_bg_color(sw,col(LIME),LV_PART_INDICATOR|LV_STATE_CHECKED);lv_obj_set_style_bg_color(sw,col(TRACK),LV_PART_INDICATOR|LV_STATE_CHECKED|LV_STATE_DISABLED);lv_obj_set_style_bg_color(sw,col(KNOB_DISABLED),LV_PART_KNOB|LV_STATE_DISABLED);lv_obj_set_style_pad_all(sw,-2,LV_PART_KNOB);if(on)lv_obj_add_state(sw,LV_STATE_CHECKED);if(!enabled){lv_obj_add_state(sw,LV_STATE_DISABLED);lv_obj_add_state(r,LV_STATE_DISABLED);}bind(sw,cameraVar?3:1,key,Screen::Home,Action::Setting,cameraVar?cameraVar:"");
 lv_obj_add_event_cb(sw,[](lv_event_t*e){auto*sw=lv_event_get_target(e);lv_label_set_text((lv_obj_t*)lv_event_get_user_data(e),lv_obj_has_state(sw,LV_STATE_CHECKED)?tr("Wł.","Ein","On"):tr("Wył.","Aus","Off"));},LV_EVENT_VALUE_CHANGED,stateText);
 lv_obj_add_event_cb(r,[](lv_event_t*e){auto*sw=(lv_obj_t*)lv_event_get_user_data(e);if(lv_obj_has_state(sw,LV_STATE_DISABLED))return;if(lv_obj_has_state(sw,LV_STATE_CHECKED))lv_obj_clear_state(sw,LV_STATE_CHECKED);else lv_obj_add_state(sw,LV_STATE_CHECKED);lv_event_send(sw,LV_EVENT_VALUE_CHANGED,nullptr);},LV_EVENT_CLICKED,sw);return sw;}
static void slider(const lv_img_dsc_t*i,const char*s,int key,int val,int y){(void)y;auto*r=box(body,0,flowY(),224,104);icon(r,i,8,10);label(r,s,44,10,169,&font_ui20);char text[12];snprintf(text,sizeof text,"%d%%",val);auto*v=label(r,text,153,37,61,&font_body18,LIME);lv_obj_set_style_text_align(v,LV_TEXT_ALIGN_RIGHT,0);lv_obj_t*sl=lv_slider_create(r);lv_obj_set_pos(sl,22,71);lv_obj_set_size(sl,180,10);lv_slider_set_range(sl,key==Brightness?5:0,100);lv_slider_set_value(sl,val,LV_ANIM_OFF);
 for(auto part:{LV_PART_MAIN,LV_PART_INDICATOR,LV_PART_KNOB}){lv_obj_set_style_bg_opa(sl,LV_OPA_COVER,part);lv_obj_set_style_radius(sl,LV_RADIUS_CIRCLE,part);}lv_obj_set_style_bg_color(sl,col(TRACK),LV_PART_MAIN);lv_obj_set_style_bg_color(sl,col(LIME),LV_PART_INDICATOR);lv_obj_set_style_bg_color(sl,col(KNOB),LV_PART_KNOB);lv_obj_set_style_pad_all(sl,8,LV_PART_KNOB);lv_obj_set_style_border_width(sl,data.theme==ThemeMode::Light?1:0,LV_PART_KNOB);lv_obj_set_style_border_color(sl,col(BORDER),LV_PART_KNOB);lv_obj_set_ext_click_area(sl,18);
 lv_obj_add_event_cb(sl,[](lv_event_t*e){auto*sl=lv_event_get_target(e);lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e),"%d%%",(int)lv_slider_get_value(sl));},LV_EVENT_VALUE_CHANGED,v);bind(sl,4,key);}
static void title(const char*s,Screen back=Screen::Home){auto*b=button(root,"",2,24,44,44);style(b,CARD,22,0);icon(b,&img_back,8,8,TEXT,256);bind(b,0,0,back);const lv_font_t*f=&font_ui22;if(wrappedHeight(s,184,f)>46)f=&font_ui20;if(wrappedHeight(s,184,f)>46)f=&font_body18;bool scroll=wrappedHeight(s,184,f)>46;if(scroll)f=&font_ui20;int height=scroll?f->line_height:wrappedHeight(s,184,f);auto*l=scroll?label(root,s,48,24+(46-height)/2,184,f):wrapped(root,s,48,24+(46-height)/2,184,f);if(scroll)lv_label_set_long_mode(l,LV_LABEL_LONG_SCROLL_CIRCULAR);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_LEFT,0);}
static void makeBody(int height=244){body=lv_obj_create(root);style(body,BG,0,0);lv_obj_set_pos(body,8,72);lv_obj_set_size(body,224,height);lv_obj_set_scrollbar_mode(body,LV_SCROLLBAR_MODE_AUTO);lv_obj_set_style_bg_color(body,col(BORDER),LV_PART_SCROLLBAR);lv_obj_set_style_bg_opa(body,LV_OPA_COVER,LV_PART_SCROLLBAR);lv_obj_set_style_width(body,2,LV_PART_SCROLLBAR);lv_obj_set_scroll_dir(body,LV_DIR_VER);}
static void footer(){auto*bar=box(root,0,268,240,52,BG);const lv_img_dsc_t*ic[]={&img_home,&img_camera,&img_bell,&img_settings};Screen dest[]={Screen::Home,Screen::Cameras,Screen::Alerts,Screen::Settings};for(int i=0;i<4;i++){auto*b=button(bar,"",i*60,0,60,52);style(b,page==dest[i]?SELECTED:BG,14,0);icon(b,ic[i],16,12,page==dest[i]?LIME:MUTED,256);bind(b,0,0,dest[i]);}}
static void metricCard(lv_obj_t*p,int m,int x,int y,int w,int h){auto*c=box(p,x,y,w,h);icon(c,metricIcon(m),7,8,m==0?LIME:TEXT,205);label(c,m==0?tr("Temp.","Temp.","Temp."):tr("Wilg.","Feuchte","Humidity"),35,10,w-41,&font_body18,MUTED);char n[32];bool valid=metricValid(data,m)&&isfinite(metricValue(data,m));if(valid)snprintf(n,sizeof n,m==0||m==4?"%.1f %s":"%.0f %s",metricValue(data,m),unit(m));else snprintf(n,sizeof n,"— %s",unit(m));auto*l=label(c,n,4,47,w-8,&font_ui20);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);bind(c,0,m,Screen::Metric);}
static void sensorHeader(lv_obj_t*p,int m,int y){auto*c=box(p,0,y,224,88);label(c,names(m),12,9,200,rowFont(names(m),200));icon(c,metricIcon(m),10,45,LIME);onlineLabel=wrapped(c,metricValid(data,m)?tr("● Połączony","● Verbunden","● Connected"):tr("○ Brak danych","○ Keine Werte","○ No data"),48,47,164,&font_body18,metricValid(data,m)?LIME:MUTED);}
static void home(){
 auto*mount=lv_img_create(root);lv_img_set_src(mount,data.theme==ThemeMode::Light?&img_mountains_light:&img_mountains);lv_obj_set_pos(mount,0,38);
 auto*l=label(root,"ESP32",0,45,240,&lv_font_montserrat_32);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);l=label(root,"R E M O T E",0,81,240,&font_ui14);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);
 const lv_img_dsc_t*ico[]={&img_camera,&img_sensor,&img_settings,&img_link};const char*txt[]={tr("Kamera","Kamera","Camera"),tr("Sensory","Sensoren","Sensors"),tr("Ustawienia","Einstellungen","Settings"),tr("Połączenie","Verbindung","Connect")};Screen dst[]={Screen::Cameras,Screen::Sensors,Screen::Settings,Screen::Network};
 for(int i=0;i<4;i++){auto*b=button(root,"",9+(i%2)*115,148+(i/2)*76,106,69);if(i==0){lv_obj_set_style_border_color(b,col(LIME),0);lv_obj_set_style_border_width(b,2,0);}icon(b,ico[i],38,7,i==0?LIME:TEXT,280);const lv_font_t*f=lv_txt_get_width(txt[i],strlen(txt[i]),&font_ui14,0,LV_TEXT_FLAG_NONE)<=102?&font_ui14:&font_ui12;auto*t=label(b,txt[i],2,47,102,f,i==0?LIME:TEXT);lv_obj_set_style_text_align(t,LV_TEXT_ALIGN_CENTER,0);bind(b,0,0,dst[i]);}
 auto*dot=box(root,107,306,8,8,LIME);lv_obj_set_style_radius(dot,4,0);dot=box(root,124,306,8,8,INACTIVE_DOT);lv_obj_set_style_radius(dot,4,0);
}
static lv_obj_t*mediaButton(lv_obj_t*p,const lv_img_dsc_t*src,int x,int y){
 auto*b=button(p,"",x,y,44,44);style(b,0x101820,22,0);lv_obj_set_style_bg_opa(b,LV_OPA_80,0);lv_obj_set_style_border_color(b,col(0xffffff),0);lv_obj_set_style_border_width(b,1,0);lv_obj_set_style_border_opa(b,LV_OPA_40,0);lv_obj_set_style_bg_color(b,col(0x35495b),LV_STATE_PRESSED);lv_obj_set_style_bg_color(b,col(0x101820),LV_STATE_DISABLED);lv_obj_set_style_opa(b,LV_OPA_40,LV_STATE_DISABLED);icon(b,src,8,8,0xffffff);return b;
}
static void closeVolume(){if(overlay)lv_obj_del(overlay);overlay=nullptr;volumeSlider=nullptr;volumeOpen=false;}
static void openVolume(){if(volumeOpen)return;volumeOpen=true;
 overlay=box(root,0,24,240,296,0x101820);style(overlay,0x101820,0,0);lv_obj_set_style_bg_opa(overlay,LV_OPA_60,0);lv_obj_move_foreground(overlay);
 lv_obj_add_event_cb(overlay,[](lv_event_t*e){if(lv_event_get_target(e)==overlay)closeVolume();},LV_EVENT_CLICKED,nullptr);
 auto*c=box(overlay,8,68,224,156);label(c,tr("Głośność","Lautstärke","Volume"),12,12,148,&font_ui20);
 auto*close=button(c,"",170,4,44,44);icon(close,&img_close,8,8);lv_obj_add_event_cb(close,[](lv_event_t*){closeVolume();},LV_EVENT_CLICKED,nullptr);
 auto*v=label(c,"",12,47,196,&font_ui22,LIME);lv_label_set_text_fmt(v,"%u%%",data.settings.volume);
 volumeSlider=lv_slider_create(c);lv_obj_set_pos(volumeSlider,20,95);lv_obj_set_size(volumeSlider,180,10);lv_slider_set_range(volumeSlider,0,100);lv_slider_set_value(volumeSlider,data.settings.volume,LV_ANIM_OFF);lv_obj_set_ext_click_area(volumeSlider,18);
 for(auto part:{LV_PART_MAIN,LV_PART_INDICATOR,LV_PART_KNOB}){lv_obj_set_style_bg_opa(volumeSlider,LV_OPA_COVER,part);lv_obj_set_style_radius(volumeSlider,LV_RADIUS_CIRCLE,part);}lv_obj_set_style_bg_color(volumeSlider,col(TRACK),LV_PART_MAIN);lv_obj_set_style_bg_color(volumeSlider,col(LIME),LV_PART_INDICATOR);lv_obj_set_style_bg_color(volumeSlider,col(KNOB),LV_PART_KNOB);lv_obj_set_style_pad_all(volumeSlider,8,LV_PART_KNOB);lv_obj_set_style_border_width(volumeSlider,data.theme==ThemeMode::Light?1:0,LV_PART_KNOB);lv_obj_set_style_border_color(volumeSlider,col(BORDER),LV_PART_KNOB);
 lv_obj_add_event_cb(volumeSlider,[](lv_event_t*e){lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e),"%d%%",int(lv_slider_get_value(lv_event_get_target(e))));},LV_EVENT_VALUE_CHANGED,v);bind(volumeSlider,4,Volume);
}
static void positionLive(){if(!image)return;auto view=Video::view(liveFit);lv_img_set_pivot(image,0,0);lv_img_set_zoom(image,view.zoom);lv_obj_set_pos(image,view.x,view.y);
 int top=liveFit?view.y+6:30,bottom=liveFit?view.y+view.height-50:268;
 if(liveHud){lv_img_set_src(lv_obj_get_child(lv_obj_get_child(liveHud,1),-1),liveFit?&img_expand:&img_minimize);lv_obj_set_pos(lv_obj_get_child(liveHud,0),8,top);lv_obj_set_pos(lv_obj_get_child(liveHud,1),66,top);lv_obj_set_pos(lv_obj_get_child(liveHud,2),188,top);lv_obj_set_pos(liveStatus,114,top+12);for(int i=0;i<4;i++)if(liveButtons[i])lv_obj_set_pos(liveButtons[i],8+i*60,bottom);}
 if(liveSettings)lv_obj_set_pos(liveSettings,188,top+50);
}
static void refreshLive(){if(!liveHud)return;
 if(hudVisible)lv_obj_clear_flag(liveHud,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(liveHud,LV_OBJ_FLAG_HIDDEN);
 bool ready=frameReady&&imageBuf&&(data.camera||data.playing);
 if(liveEmptyTitle)lv_label_set_text(liveEmptyTitle,!imageBuf?tr("Brak pamięci obrazu","Bildspeicher fehlt","No video memory"):data.camera||data.playing?tr("Ładowanie obrazu…","Bild wird geladen…","Loading video…"):tr("Brak obrazu","Kein Bild","No video"));
 if(liveEmptyHint){if(data.camera||data.playing)lv_obj_add_flag(liveEmptyHint,LV_OBJ_FLAG_HIDDEN);else lv_obj_clear_flag(liveEmptyHint,LV_OBJ_FLAG_HIDDEN);}
 if(liveEmpty){if(ready)lv_obj_add_flag(liveEmpty,LV_OBJ_FLAG_HIDDEN);else lv_obj_clear_flag(liveEmpty,LV_OBJ_FLAG_HIDDEN);}
 char text[20];if(data.recording)snprintf(text,sizeof text,"%02lu:%02lu",(unsigned long)(data.clipMs/60000),(unsigned long)(data.clipMs/1000%60));else snprintf(text,sizeof text,"%s",data.playing?tr("Plik","Datei","File"):data.camera?"LIVE":"—");lv_label_set_text(liveStatus,text);lv_obj_set_style_text_color(liveStatus,col(data.recording?0xff6878:0xffffff),0);
 for(int i=0;i<4;i++)if(liveButtons[i]){bool enabled=i==0||i==3?data.audioCap&&data.camera:i==1?data.camera&&data.sd:data.recording||(data.camera&&data.sd);if(enabled)lv_obj_clear_state(liveButtons[i],LV_STATE_DISABLED);else lv_obj_add_state(liveButtons[i],LV_STATE_DISABLED);}
 if(liveGlyphs[0])lv_obj_set_style_img_recolor(liveGlyphs[0],col(data.talking?0xa4f84e:0xffffff),0);
 if(liveGlyphs[2]){lv_img_set_src(liveGlyphs[2],data.recording?&img_stop:&img_record);lv_obj_set_style_img_recolor(liveGlyphs[2],col(data.recording?0xff6878:0xffffff),0);}
 if(liveGlyphs[3])lv_obj_set_style_img_recolor(liveGlyphs[3],col(data.listening?0xa4f84e:0xffffff),0);
}
static void live(){
 auto*frame=box(root,0,0,240,320,0x080b0f);style(frame,0x080b0f,0,0);lv_obj_add_event_cb(frame,[](lv_event_t*){if(hudVisible&&liveHud){lv_point_t p;auto*input=lv_indev_get_act();if(input){lv_indev_get_point(input,&p);for(uint32_t i=0;i<lv_obj_get_child_cnt(liveHud);i++){auto*c=lv_obj_get_child(liveHud,i);if(!lv_obj_check_type(c,&lv_btn_class))continue;lv_area_t a;lv_obj_get_coords(c,&a);if(p.x>=a.x1&&p.x<=a.x2&&p.y>=a.y1&&p.y<=a.y2)return;}}}hudVisible=!hudVisible;refreshLive();},LV_EVENT_CLICKED,nullptr);
 if(imageBuf){image=lv_img_create(frame);lv_img_set_src(image,&liveImage);lv_obj_clear_flag(image,LV_OBJ_FLAG_CLICKABLE);lv_img_set_antialias(image,true);}
 liveEmpty=box(frame,10,103,220,124,0x101820);style(liveEmpty,0x101820,14,0);lv_obj_clear_flag(liveEmpty,LV_OBJ_FLAG_CLICKABLE);icon(liveEmpty,&img_camera,96,8,0xffffff);auto*l=wrapped(liveEmpty,!imageBuf?tr("Brak pamięci obrazu","Bildspeicher fehlt","No video memory"):data.camera||data.playing?tr("Ładowanie obrazu…","Bild wird geladen…","Loading video…"):tr("Brak obrazu","Kein Bild","No video"),10,45,200,&font_ui20,0xffffff);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);liveEmptyTitle=l;l=wrapped(liveEmpty,tr("Sprawdź połączenie","Verbindung prüfen","Check connection"),10,83,200,&font_body18,0xcbd3da);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);liveEmptyHint=l;
 auto*status=box(frame,0,0,240,24,0x101820);style(status,0x101820,0,0);lv_obj_set_style_bg_opa(status,LV_OPA_80,0);lv_obj_clear_flag(status,LV_OBJ_FLAG_CLICKABLE);clockLabel=label(status,data.timeText,10,5,55,&font_ui14,0xffffff);wifiIcon=icon(status,&img_wifi,163,1,0xffffff,150);icon(status,&img_battery,183,0,0xffffff,130);batteryLabel=label(status,"—",201,6,37,&font_ui12,0xffffff);lv_obj_set_style_text_align(batteryLabel,LV_TEXT_ALIGN_RIGHT,0);
 liveHud=box(frame,0,0,240,320,0);style(liveHud,0,0,0);lv_obj_set_style_bg_opa(liveHud,LV_OPA_TRANSP,0);lv_obj_clear_flag(liveHud,LV_OBJ_FLAG_CLICKABLE);
 auto*back=mediaButton(liveHud,&img_back,8,30);lv_obj_add_event_cb(back,[](lv_event_t*){if(Ota::installing(data.ota.state))return;if(data.playing)send(Action::Play,0,"");next(Screen::Cameras);},LV_EVENT_CLICKED,nullptr);
 auto*fit=mediaButton(liveHud,&img_expand,66,30);bind(fit,16);auto*vol=mediaButton(liveHud,&img_volume,188,30);bind(vol,17);liveStatus=label(liveHud,"LIVE",114,42,66,&font_body18,0xffffff);
 if(!data.playing){liveSettings=mediaButton(liveHud,&img_settings,188,80);bind(liveSettings,0,0,Screen::CameraSettings);}
 if(!data.playing){const lv_img_dsc_t*icons[]={&img_mic,&img_camera,&img_record,&img_speaker};for(int i=0;i<4;i++){auto*b=mediaButton(liveHud,icons[i],8+i*60,268);liveButtons[i]=b;liveGlyphs[i]=lv_obj_get_child(b,-1);if(i==0)bind(b,6);if(i==1)bind(b,2,0,Screen::Home,Action::Snapshot);if(i==2)bind(b,7,0,Screen::Home,Action::Record);if(i==3)bind(b,7,0,Screen::Home,Action::Listen);}}
 positionLive();refreshLive();
}
static const char*shortMetric(int m){switch(m){case 0:return tr("Temp.","Temp.","Temp.");case 1:return tr("Wilgotn.","Feuchte","Humidity");case 2:return "CO₂";case 3:return "IAQ";case 4:return tr("Ciśn.","Druck","Pressure");default:return "eCO₂";}}
static void sensorReading(int m,char*text,size_t size){if(metricValid(data,m)&&isfinite(metricValue(data,m)))snprintf(text,size,m==0?"%.1f":"%.0f",metricValue(data,m));else snprintf(text,size,"—");}
static void sensors(){title(tr("Sensory","Sensoren","Sensors"));lv_obj_set_width(lv_obj_get_child(root,-1),132);auto*scanButton=button(root,"",188,24,44,44);icon(scanButton,&img_plus,8,8,LIME);bind(scanButton,0,0,Screen::Scan);makeBody();
 for(int m=0;m<6;m++){bool valid=metricValid(data,m)&&isfinite(metricValue(data,m));auto*r=box(body,(m%2)*116,(m/2)*124,108,116);icon(r,metricIcon(m),10,9,valid?LIME:MUTED,220);label(r,unit(m),42,11,44,&font_body18,MUTED);auto*dot=box(r,92,12,8,8,valid?LIME:MUTED);style(dot,valid?LIME:MUTED,4,0);lv_obj_clear_flag(dot,LV_OBJ_FLAG_CLICKABLE);sensorStates[m]=dot;label(r,shortMetric(m),10,42,88,&font_body18,TEXT);char reading[32];sensorReading(m,reading,sizeof reading);sensorValues[m]=label(r,reading,10,72,88,lv_txt_get_width(reading,strlen(reading),&lv_font_montserrat_28,0,LV_TEXT_FLAG_NONE)>88?&font_ui22:&lv_font_montserrat_28);bind(r,0,m,Screen::Metric);}
 flowText(tr("Dotknij pomiaru, aby otworzyć wykres.","Messwert antippen für den Verlauf.","Tap a reading to open its chart."));auto*b=button(body,tr("Podłącz sensor","Sensor koppeln","Connect sensor"),0,0,224,64);bind(b,0,0,Screen::Scan);navRow(&img_settings,tr("Ustawienia sensorów","Sensor-Einstellungen","Sensor settings"),Screen::SensorSettings,0);
}
static void metricPage(){title(names(metric),Screen::Sensors);makeBody();sensorHeader(body,metric,0);
 char n[40];if(metricValid(data,metric)&&isfinite(metricValue(data,metric)))snprintf(n,sizeof n,metric==0||metric==4?"%.1f %s":"%.0f %s",metricValue(data,metric),unit(metric));else snprintf(n,sizeof n,"— %s",unit(metric));valueLabel=label(body,n,0,100,224,&lv_font_montserrat_32);lv_obj_set_style_text_align(valueLabel,LV_TEXT_ALIGN_CENTER,0);
 chart=lv_chart_create(body);style(chart,CARD,0,0);lv_obj_set_pos(chart,13,148);lv_obj_set_size(chart,202,110);lv_chart_set_type(chart,LV_CHART_TYPE_LINE);lv_chart_set_point_count(chart,HISTORY_COUNT);lv_chart_set_div_line_count(chart,4,5);lv_obj_set_style_line_color(chart,col(BORDER),LV_PART_MAIN);lv_obj_set_style_line_width(chart,1,LV_PART_MAIN);lv_obj_set_style_size(chart,4,LV_PART_INDICATOR);lv_obj_set_style_line_width(chart,3,LV_PART_ITEMS);series=lv_chart_add_series(chart,col(metric==1?BLUE:LIME),LV_CHART_AXIS_PRIMARY_Y);
 flowText(tr("Historia · ostatnie 60 próbek","Verlauf · letzte 60 Werte","History · last 60 samples"));
 minLabel=flowText("Min —");maxLabel=flowText("Max —");
 auto*b=button(body,tr("Ustawienia sensora","Sensor-Einstellungen","Sensor settings"),0,0,224,64);bind(b,0,metric,Screen::SensorSettings);
}
static void settings(){title(tr("Ustawienia","Einstellungen","Settings"));makeBody();
 navRow(&img_settings,tr("Ogólne","Allgemein","General"),Screen::General,0);navRow(&img_sun,tr("Wygląd","Design","Appearance"),Screen::Appearance,0,themeName());navRow(&img_camera,tr("Kamera","Kamera","Camera"),Screen::CameraSettings,45);navRow(&img_volume,tr("Audio","Audio","Audio"),Screen::AudioSettings,0);navRow(&img_sensor,tr("Sensory","Sensoren","Sensors"),Screen::SensorSettings,90);navRow(&img_bell,tr("Powiadomienia","Meldungen","Alerts"),Screen::Alerts,135);navRow(&img_wifi,tr("Połączenie","Verbindung","Connection"),Screen::Network,180);navRow(&img_info,tr("System","System","System"),Screen::System,225);
}
static const char*linkText(){switch(data.wifiLink){
 case WifiLinkState::Connecting:return tr("Łączenie…","Verbinden…","Connecting…");
 case WifiLinkState::Connected:return tr("Połączono","Verbunden","Connected");
 case WifiLinkState::Failed:return tr("Błąd połączenia","Verbindung fehlgeschlagen","Connection failed");
 default:return tr("Rozłączono","Nicht verbunden","Disconnected");
}}
static void network(){title(tr("Połączenie","Verbindung","Connection"),Screen::Settings);makeBody();
 const char*info=data.wifi?data.ip:data.wifiLink==WifiLinkState::Failed?tr("Sprawdź hasło i zasięg","Passwort / Empfang prüfen","Check password / signal"):"2.4 GHz";
 int statusHeight=wrappedHeight(linkText(),168,&font_ui20);auto*r=box(body,0,0,224,std::max(104,statusHeight+font_body18.line_height+wrappedHeight(info,168,&font_body18)+28),data.wifi?SELECTED:CARD);
 icon(r,&img_wifi,8,18,data.wifi?LIME:MUTED);wrapped(r,linkText(),44,8,168,&font_ui20,data.wifi?LIME:data.wifiLink==WifiLinkState::Failed?RED:TEXT);
 auto*ssid=label(r,data.wifi?data.connectedSsid:data.settings.ssid[0]?data.settings.ssid:tr("Wybierz sieć","Netz wählen","Choose a network"),44,12+statusHeight,168,&font_body18,TEXT);lv_label_set_long_mode(ssid,LV_LABEL_LONG_SCROLL_CIRCULAR);
 wrapped(r,info,44,16+statusHeight+font_body18.line_height,168,&font_body18,MUTED);
 auto*b=button(body,tr("Wyszukaj Wi-Fi","WLAN suchen","Find Wi-Fi"),0,0,224,56,true);bind(b,9);
 flowText(tr("Sieć","Netzwerk","Network"));
 r=row(&img_link,tr("Połączenie przez","Verbindung über","Connect via"),data.settings.networkMode?tr("Router","Router","Router"):tr("Bezpośrednio","Direkt","Direct"),0,64,18);chevron(r);bind(r,12);
 editRow(nullptr,tr("Nazwa sieci","Netzname","Network name"),WifiSsid,data.settings.ssid,0);editRow(nullptr,tr("Hasło Wi-Fi","WLAN-Passwort","Wi-Fi password"),WifiPassword,data.settings.password,0,true);
 b=button(body,data.wifi?tr("Połącz ponownie","Neu verbinden","Reconnect"):tr("Połącz","Verbinden","Connect"),0,0,224,56);bind(b,2,0,Screen::Home,Action::Connect);
 flowText(tr("Kamera","Kamera","Camera"));
 editRow(&img_camera,tr("Adres kamery","Kamera-Adresse","Camera address"),CameraHost,data.settings.cameraHost,0);editRow(&img_link,tr("Klucz parowania","Kopplungsschlüssel","Pairing key"),PairToken,data.settings.pairToken,0,true);
 navRow(&img_link,tr("Parowanie kamery","Kamera koppeln","Pair camera"),Screen::Pair,0);
}
static const char*securityText(WifiSecurity s){switch(s){case WifiSecurity::Open:return tr("Otwarta","Offen","Open");case WifiSecurity::Wpa:return "WPA";case WifiSecurity::Wpa2:return "WPA2";case WifiSecurity::Wpa3:return "WPA3";case WifiSecurity::WpaWpa2:return "WPA / WPA2";case WifiSecurity::Wpa2Wpa3:return "WPA2 / WPA3";default:return tr("Nieobsługiwana","Nicht unterstützt","Unsupported");}}
static void wifiNetworks(){title(tr("Sieci Wi-Fi","WLAN-Netze","Wi-Fi networks"),Screen::Network);makeBody(188);
 const char*status=data.wifiScan==WifiScanState::Scanning?tr("Skanowanie…","Suche läuft…","Scanning…"):data.wifiScan==WifiScanState::Failed?tr("Błąd skanu. Ponów próbę.","Suche fehlgeschlagen. Erneut.","Scan failed. Try again."):data.wifiScan==WifiScanState::Done&&!data.networkCount?tr("Brak sieci w zasięgu.","Keine Netze in Reichweite.","No networks in range."):tr("Wybierz sieć · 2.4 GHz","Netz wählen · 2.4 GHz","Select a network · 2.4 GHz");
 flowText(status,&font_body18,data.wifiScan==WifiScanState::Failed?RED:MUTED);
 for(int i=0;i<data.networkCount;i++){auto&n=data.networks[i];bool active=data.wifi&&!strcmp(n.ssid,data.connectedSsid);char info[64];snprintf(info,sizeof info,"%s · %d dBm",securityText(n.security),n.rssi);auto*r=box(body,0,flowY(),224,std::max(80,40+wrappedHeight(info,168,&font_body18)+12));if(active){lv_obj_set_style_bg_color(r,col(SELECTED),0);}icon(r,&img_wifi,8,18,active?LIME:TEXT,256);auto*l=label(r,n.ssid,44,8,168,&font_ui20);lv_label_set_long_mode(l,LV_LABEL_LONG_SCROLL_CIRCULAR);wrapped(r,info,44,40,168,&font_body18,MUTED);bind(r,11,i);if(n.security==WifiSecurity::Unsupported||data.wifiScan==WifiScanState::Scanning)lv_obj_add_state(r,LV_STATE_DISABLED);}
 auto*b=button(root,data.wifiScan==WifiScanState::Scanning?tr("Skanowanie…","Suche läuft…","Scanning…"):tr("Skanuj ponownie","Erneut suchen","Scan again"),8,268,224,52,true);bind(b,9);if(data.wifiScan==WifiScanState::Scanning)lv_obj_add_state(b,LV_STATE_DISABLED);
}
static const char*languageName(int code){return code==1?"Deutsch":code==2?"English":"Polski";}
static void languages(){title(tr("Język","Sprache","Language"),Screen::General);makeBody();const int codes[]={1,0,2};
 for(int i=0;i<3;i++){int code=codes[i];bool active=code==data.settings.language;auto*r=box(body,0,i*80,224,72);if(active){lv_obj_set_style_bg_color(r,col(SELECTED),0);}label(r,languageName(code),16,23,166,&font_ui20,active?LIME:TEXT);if(active)icon(r,&img_check,184,22,LIME);bind(r,10,code);}
 flowText(tr("Język zapisany automatycznie.","Sprache automatisch gespeichert.","Language saved automatically."));
}
static void appearance(){title(tr("Wygląd","Design","Appearance"),appearanceBack);makeBody();
 const ThemeMode modes[]={ThemeMode::Light,ThemeMode::Dark};
 for(int i=0;i<2;i++){bool active=data.theme==modes[i];auto sample=UiColors::get(modes[i],false);auto*r=box(body,0,i*100,224,92,active?SELECTED:CARD);
  auto*preview=box(r,12,12,56,68,sample.background);lv_obj_clear_flag(preview,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_border_width(preview,1,0);lv_obj_set_style_border_color(preview,col(sample.border),0);
  auto*bar=box(preview,8,9,28,4,sample.text);lv_obj_clear_flag(bar,LV_OBJ_FLAG_CLICKABLE);
  for(int j=0;j<2;j++){auto*c=box(preview,6,22+j*19,44,14,j?sample.accent:sample.card);lv_obj_clear_flag(c,LV_OBJ_FLAG_CLICKABLE);}
  wrapped(r,modes[i]==ThemeMode::Light?tr("Jasny","Hell","Light"):tr("Ciemny","Dunkel","Dark"),84,33,96,&font_ui20,active?LIME:TEXT);if(active)icon(r,&img_check,185,32,LIME);bind(r,15,int(modes[i]));
 }
 flowText(tr("Wybór zapisuje się automatycznie.","Wird automatisch gespeichert.","Your choice is saved automatically."));
}
static void scan(){title(tr("Podłącz sensor","Sensor koppeln","Connect sensor"),Screen::Sensors);makeBody(188);auto*r=box(body,0,0,224,95);auto*i=lv_img_create(r);lv_img_set_src(i,&img_module);lv_img_set_zoom(i,500);lv_obj_set_pos(i,60,9);label(r,"P O G O  ·  I²C",10,68,205,&font_body18,LIME);
 flowText(tr("1  Podłącz moduł pogo\n2  Automatyczne wykrycie\n3  Otwórz pomiary","1  Modul anstecken\n2  Automatische Erkennung\n3  Messwerte öffnen","1  Attach the pogo module\n2  Automatic discovery\n3  Open measurements"),&font_ui20,TEXT);
 sensorInfo=flowText("");auto*b=button(root,tr("Skanuj I²C","I²C scannen","Scan I²C"),8,268,224,52,true);bind(b,2,0,Screen::Home,Action::Scan);
}
static void gallery(){title(tr("Nagrania","Aufnahmen","Recordings"),Screen::Cameras);makeBody();if(!data.fileCount)flowText(tr("Brak plików na karcie SD","Keine Dateien auf SD","No recordings on SD"),&font_ui20);
 for(int i=0;i<data.fileCount;i++){const char*p=strrchr(data.files[i],'/');auto*r=row(&img_play,p?p+1:data.files[i],"›",i*50,46);bind(r,2,1,Screen::Home,Action::Play,data.files[i]);lv_obj_add_event_cb(r,[](lv_event_t*){next(Screen::Live);},LV_EVENT_CLICKED,nullptr);}}
static const char*otaText(){using Ota::State;using Ota::Error;
 switch(data.ota.state){
 case State::Idle:return tr("Sprawdź dostępne aktualizacje.","Nach Updates suchen.","Check for updates.");
 case State::Checking:return tr("Sprawdzanie…","Wird geprüft…","Checking…");
 case State::Current:return tr("Masz aktualną wersję.","Version ist aktuell.","You are up to date.");
 case State::Available:return tr("Nowa wersja dostępna.","Neue Version verfügbar.","New version available.");
 case State::Downloading:return tr("Pobieranie aktualizacji…","Update wird geladen…","Downloading update…");
 case State::Verifying:return tr("Sprawdzanie pliku…","Datei wird geprüft…","Verifying image…");
 case State::Restarting:return tr("Gotowe. Restart…","Fertig. Neustart…","Done. Restarting…");
 case State::Error:break;
 }
 switch(data.ota.error){
 case Error::Network:return tr("Połącz Wi-Fi z internetem.","WLAN mit Internet verbinden.","Connect Wi-Fi with internet.");
 case Error::Clock:return tr("Synchronizacja czasu… Spróbuj za chwilę.","Zeit wird synchronisiert… Gleich erneut.","Syncing time… Try again shortly.");
 case Error::Http:return tr("Brak wydania lub błąd HTTPS.","Kein Release / HTTPS-Fehler.","No release or HTTPS error.");
 case Error::Dns:return tr("Błąd DNS. Sprawdź router.","DNS-Fehler. Router prüfen.","DNS failed. Check router.");
 case Error::Connection:return tr("GitHub nie odpowiada. Sprawdź internet.","GitHub antwortet nicht. Internet prüfen.","GitHub unreachable. Check internet.");
 case Error::Tls:{static char text[100];snprintf(text,sizeof text,tr("Błąd HTTPS/TLS. Kod: %ld","HTTPS/TLS-Fehler. Code: %ld","HTTPS/TLS error. Code: %ld"),long(data.ota.detail));return text;}
 case Error::NotFound:return tr("Brak pliku wydania (HTTP 404).","Release-Datei fehlt (HTTP 404).","Release file missing (HTTP 404).");
 case Error::RateLimit:return tr("GitHub odrzuca żądanie. Spróbuj później.","GitHub lehnt Anfrage ab. Später erneut.","GitHub rejected request. Retry later.");
 case Error::Redirect:return tr("Błąd przekierowania GitHub.","GitHub-Weiterleitung fehlgeschlagen.","GitHub redirect failed.");
 case Error::Manifest:return tr("Nieprawidłowy opis wydania.","Ungültige Release-Daten.","Invalid release metadata.");
 case Error::Memory:{if(data.ota.detail){static char text[100];snprintf(text,sizeof text,tr("Brak pamięci dla HTTPS.\nKod: %ld","HTTPS: Speicher fehlt.\nCode: %ld","HTTPS: not enough RAM.\nCode: %ld"),long(data.ota.detail));return text;}return tr("Brak pamięci RAM.","Zu wenig RAM.","Not enough RAM.");}
 case Error::Partition:return tr("Błąd partycji OTA.","OTA-Partitionsfehler.","OTA partition error.");
 case Error::Expired:return tr("Sprawdź aktualizacje ponownie.","Updates erneut prüfen.","Check updates again.");
 case Error::Download:return tr("Przerwany transfer. Ponów próbę.","Transfer abgebrochen. Erneut.","Download interrupted. Retry.");
 case Error::Checksum:return tr("Błędna suma SHA-256.","Falsche SHA-256-Prüfsumme.","SHA-256 mismatch.");
 case Error::Image:return tr("Plik nie pasuje do pilota.","Datei passt nicht zum Gerät.","Invalid remote image.");
 case Error::Write:return tr("Nie udało się zapisać pamięci.","Flash-Schreiben fehlgeschlagen.","Flash write failed.");
 case Error::Busy:return tr("Zatrzymaj audio, nagranie i skan.","Audio, Aufnahme, Suche stoppen.","Stop audio, recording and scan.");
 case Error::Battery:return tr("Naładuj baterię przed OTA.","Akku vor dem Update laden.","Charge battery before OTA.");
 default:return tr("Błąd aktualizacji.","Update fehlgeschlagen.","Update failed.");
 }
}
static void updatePage(){title(tr("Aktualizacja","Software","Firmware"),Screen::System);makeBody(100);
 auto*r=box(body,0,0,224,96);label(r,tr("Zainstalowana wersja","Installierte Version","Installed version"),12,10,200,&font_body18,MUTED);label(r,FIRMWARE_VERSION,12,36,200,&font_ui22);
 if(data.ota.version[0]&&data.ota.state!=Ota::State::Current){char version[64];snprintf(version,sizeof version,tr("Dostępna: %s","Verfügbar: %s","Available: %s"),data.ota.version);label(r,version,12,68,200,&font_body18,LIME);}
 flowText(data.ota.state==Ota::State::Available?data.ota.notes[data.settings.language]:tr("Aktualizacje przez Wi-Fi z internetem.","Updates über WLAN mit Internet.","Updates need Wi-Fi with internet."));
 onlineLabel=wrapped(root,otaText(),12,180,216,&font_ui18,data.ota.state==Ota::State::Error?RED:data.ota.state==Ota::State::Current?LIME:TEXT);lv_obj_set_height(onlineLabel,Ota::installing(data.ota.state)?52:76);
 lv_label_set_long_mode(onlineLabel,LV_LABEL_LONG_WRAP);
 otaProgress=lv_bar_create(root);lv_obj_set_pos(otaProgress,12,251);lv_obj_set_size(otaProgress,156,6);lv_obj_set_style_bg_color(otaProgress,col(BORDER),LV_PART_MAIN);lv_obj_set_style_bg_color(otaProgress,col(LIME),LV_PART_INDICATOR);for(auto part:{LV_PART_MAIN,LV_PART_INDICATOR}){lv_obj_set_style_bg_opa(otaProgress,LV_OPA_COVER,part);lv_obj_set_style_radius(otaProgress,3,part);}otaPercent=label(root,"0%",173,235,57,&font_body18,LIME);
 otaCheckButton=button(root,tr("Sprawdź","Prüfen","Check"),8,264,108,52);bind(otaCheckButton,13);
 otaInstallButton=button(root,tr("Aktualizuj","Update","Update"),124,264,108,52,true);bind(otaInstallButton,14);
}
void show(Screen s,int m){if(s==Screen::Live&&page!=Screen::Live)hudVisible=true;page=s;pending=s;palette();metric=std::max(0,std::min(5,m));rebuild=false;bindCount=0;image=valueLabel=onlineLabel=minLabel=maxLabel=chart=recordLabel=sensorInfo=wave=nullptr;overlay=editor=keyboard=editorHint=revealLabel=nullptr;joiningWifi=false;
 otaCheckButton=otaInstallButton=otaProgress=otaPercent=nullptr;otaConfirm=false;
 for(auto&v:sensorValues)v=nullptr;for(auto&v:sensorStates)v=nullptr;
 clockLabel=batteryLabel=wifiIcon=liveSettings=nullptr;liveHud=liveEmpty=liveEmptyTitle=liveEmptyHint=liveStatus=volumeSlider=nullptr;volumeOpen=false;for(auto&v:liveButtons)v=nullptr;for(auto&v:liveGlyphs)v=nullptr;
 if(!root)root=lv_obj_create(nullptr);else lv_obj_clean(root);body=nullptr;style(root,BG,0,0);lv_obj_set_style_text_font(root,&font_ui14,0);lv_obj_clear_flag(root,LV_OBJ_FLAG_SCROLLABLE);lv_scr_load(root);
 if(s!=Screen::Live){clockLabel=label(root,data.timeText,10,5,55,&font_ui14);wifiIcon=icon(root,&img_wifi,163,1,data.wifi?TEXT:MUTED,150);icon(root,&img_battery,183,0,TEXT,130);batteryLabel=label(root,"—",201,6,37,&font_ui12);lv_obj_set_style_text_align(batteryLabel,LV_TEXT_ALIGN_RIGHT,0);}
 switch(s){
 case Screen::Home:home();break;case Screen::Live:live();break;case Screen::Sensors:sensors();break;case Screen::Metric:metricPage();break;case Screen::Settings:settings();break;case Screen::Network:network();break;case Screen::Scan:scan();break;case Screen::Recordings:gallery();break;case Screen::WifiNetworks:wifiNetworks();break;case Screen::Languages:languages();break;case Screen::Appearance:appearance();break;case Screen::Update:updatePage();break;
 case Screen::Cameras:title(tr("Kamery","Kameras","Cameras"));makeBody(188);{const char*cameraTitle=tr("Podgląd","Live-Bild","Live view");auto*f=rowFont(cameraTitle,164);int h=wrappedHeight(cameraTitle,164,f);auto*r=box(body,0,0,224,std::max(88,h+font_ui18.line_height+24));lv_obj_set_style_border_color(r,col(LIME),0);icon(r,&img_camera,10,22,LIME);wrapped(r,cameraTitle,48,10,164,f);label(r,data.camera?tr("● Połączona","● Verbunden","● Online"):tr("○ Offline","○ Offline","○ Offline"),48,16+h,164,&font_body18,data.camera?LIME:MUTED);bind(r,0,0,Screen::Live);}navRow(&img_folder,tr("Nagrania i zdjęcia","Aufnahmen & Fotos","Recordings & photos"),Screen::Recordings,83);navRow(&img_settings,tr("Ustawienia kamery","Kamera-Einstellungen","Camera settings"),Screen::CameraSettings,132);navRow(&img_volume,tr("Ustawienia audio","Audio-Einstellungen","Audio settings"),Screen::AudioSettings,181);send(Action::LoadFiles,0,"");footer();break;
 case Screen::General:title(tr("Ogólne","Allgemein","General"),Screen::Settings);makeBody();navRow(&img_globe,tr("Język","Sprache","Language"),Screen::Languages,0,languageName(data.settings.language));navRow(&img_sun,tr("Wyświetlacz","Display","Display"),Screen::Display,45);navRow(&img_battery,tr("Bateria","Batterie","Battery"),Screen::Battery,90);row(&img_clock,tr("Czas automatyczny","Automatische Zeit","Automatic time"),"NTP",204);break;
 case Screen::Display:title(tr("Wyświetlacz","Display","Display"),Screen::General);makeBody();slider(&img_sun,tr("Jasność","Helligkeit","Brightness"),Brightness,data.settings.brightness,0);numberRow(tr("Przygaś po [s]","Dimmen nach [s]","Dim after [s]"),Timeout,data.settings.timeoutSec,70);navRow(data.theme==ThemeMode::Light?&img_sun:&img_moon,tr("Wygląd","Design","Appearance"),Screen::Appearance,118,themeName());navRow(&img_sun,tr("Test kolorów","Farbtest","Color test"),Screen::Colors,211);break;
 case Screen::Colors:title(tr("Test kolorów","Farbtest","Color test"),Screen::Display);makeBody();{
  const uint32_t colors[]={0xff0000,0x00ff00,0x0000ff,0x000000,0x808080,0xffffff};const char*names[]={"R","G","B","0","50","100"};
  for(int i=0;i<6;i++){auto*c=box(body,(i%3)*75,(i/3)*74,74,68,colors[i]);auto*l=label(c,names[i],0,23,74,&font_body18,i==1||i>=4?0x000000:0xffffff);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);}
  auto*c=box(body,0,154,224,49,LIME);auto*l=label(c,tr("Aktywny","Aktiv","Active"),0,12,224,&font_ui20,ACCENT_INK);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);
  flowText(tr("Biel powinna być biała,\na czerń możliwie ciemna.","Weiß sollte weiß sein,\nSchwarz möglichst dunkel.","White should look white;\nblack as dark as possible."),&font_body18,TEXT);
 }break;
 case Screen::SensorSettings:title(tr("Ustawienia sensorów","Sensor-Einstellungen","Sensor settings"),Screen::Sensors);makeBody();sensorHeader(body,metric,0);numberRow(tr("Zapis co [s]","Speichern alle [s]","Save every [s]"),LogInterval,data.settings.logIntervalSec,60);toggle(&img_folder,tr("Zapis na SD","Auf SD speichern","Save to SD"),data.settings.logging,Logging,109);navRow(&img_bell,tr("Progi alarmów","Alarmgrenzen","Alert thresholds"),Screen::Limits,158);navRow(&img_scan,tr("Skanowanie I²C","I²C scannen","I²C scan"),Screen::Scan,207);break;
 case Screen::Limits:title(tr("Progi alarmów","Alarmgrenzen","Alert thresholds"),Screen::SensorSettings);makeBody();numberRow(tr("Temp. min [°C]","Temp. min [°C]","Temp. min [°C]"),TempMin,data.settings.tempMin,0);numberRow(tr("Temp. max [°C]","Temp. max [°C]","Temp. max [°C]"),TempMax,data.settings.tempMax,45);numberRow(tr("Wilg. min [%]","Feuchte min [%]","Humidity min [%]"),HumidityMin,data.settings.humidityMin,90);numberRow(tr("Wilg. max [%]","Feuchte max [%]","Humidity max [%]"),HumidityMax,data.settings.humidityMax,135);numberRow("CO₂ max [ppm]",Co2Max,data.settings.co2Max,180);numberRow("IAQ max",IaqMax,data.settings.iaqMax,225);break;
 case Screen::CameraSettings:title(tr("Kamera","Kamera","Camera"),cameraSettingsBack);makeBody();row(&img_video,tr("Obraz","Bild","Video"),"QVGA · ~8 fps",0);toggle(&img_moon,tr("Tryb nocny","Nachtmodus","Night mode"),data.night,0,45,"night",data.camera);toggle(&img_motion,tr("Detekcja ruchu","Bewegung","Motion detection"),data.motionEnabled,0,90,"motion",data.motionCap);navRow(&img_folder,tr("Nagrania","Aufnahmen","Recordings"),Screen::Recordings,0);flowText(tr("Płynność zależy od Wi-Fi. Tryb nocny: obraz czarno-biały. Podczerwień wymaga diod IR.","Bildrate je nach WLAN. Nachtmodus: Schwarz-Weiß. Infrarot braucht IR-LEDs.","Frame rate depends on Wi-Fi. Night mode: monochrome. Infrared needs IR LEDs."));break;
 case Screen::AudioSettings:title(tr("Audio","Audio","Audio"),Screen::Settings);makeBody();slider(&img_volume,tr("Głośność","Lautstärke","Volume"),Volume,data.settings.volume,0);{auto*b=button(body,data.listening?tr("Wyłącz odsłuch","Hören beenden","Stop listening"):tr("Włącz odsłuch","Hören starten","Start listening"),0,0,224,64,true);bind(b,7,0,Screen::Home,Action::Listen);if(!data.audioCap||!data.camera)lv_obj_add_state(b,LV_STATE_DISABLED);}toggle(&img_mic,tr("Alarm dźwięku","Geräuschalarm","Sound alert"),data.noiseEnabled,0,0,"noise",data.audioCap&&data.camera);navRow(&img_mic,tr("Mówienie","Sprechen","Talk"),Screen::Audio,0);flowText(data.audioCap?tr("Alarm reaguje na głośny dźwięk.","Alarm bei lauten Geräuschen.","Alerts react to loud sounds."):tr("Kamera nie ma modułu audio.","Kamera ohne Audiomodul.","Camera has no audio module."));break;
 case Screen::Audio:title(tr("Mówienie","Sprechen","Talk"),Screen::AudioSettings);makeBody();{auto*b=button(body,"",36,0,152,152);lv_obj_set_style_radius(b,76,0);lv_obj_set_style_border_color(b,col(LIME),0);lv_obj_set_style_border_width(b,2,0);icon(b,&img_mic,58,25,LIME,400);auto*l=wrapped(b,tr("Mów","Sprechen","Talk"),10,107,132,&font_body18,LIME);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);bind(b,6);if(!data.audioCap)lv_obj_add_state(b,LV_STATE_DISABLED);}
 {auto*c=flowText(data.audioCap?tr("Przytrzymaj, aby mówić","Zum Sprechen halten","Hold to speak"):tr("Kamera nie ma modułu audio","Kamera ohne Audiomodul","Camera has no audio module"));lv_obj_set_style_text_align(c,LV_TEXT_ALIGN_CENTER,0);}
 {auto*b=button(body,data.listening?tr("Wyłącz odsłuch","Hören aus","Stop listening"):tr("Włącz odsłuch","Hören ein","Start listening"),0,214,224,42,true);bind(b,7,0,Screen::Home,Action::Listen);if(!data.audioCap)lv_obj_add_state(b,LV_STATE_DISABLED);}break;
 case Screen::Alerts:title(tr("Powiadomienia","Meldungen","Alerts"));makeBody(188);toggle(&img_bell,tr("Alarmy lokalne","Lokale Alarme","Local alerts"),data.settings.notifications,Notifications,0);for(int i=0;i<data.alertCount;i++){auto*r=row(&img_bell,data.alerts[i],"",50+i*55,64);lv_obj_set_style_border_color(r,col(RED),0);}if(!data.alertCount)flowText(tr("Brak zdarzeń","Keine Ereignisse","No events"));{auto*b=button(body,tr("Wyczyść listę","Liste löschen","Clear list"),0,0,224,64);bind(b,2,0,Screen::Home,Action::ClearAlerts);}footer();break;
 case Screen::Battery:title(tr("Bateria","Batterie","Battery"),Screen::General);makeBody();icon(body,&img_battery,90,11,LIME,450);{char b[32];snprintf(b,sizeof b,data.batteryPct>=0?"%d %%":"—",data.batteryPct);valueLabel=label(body,b,0,68,224,&lv_font_montserrat_32);lv_obj_set_style_text_align(valueLabel,LV_TEXT_ALIGN_CENTER,0);}sensorInfo=label(body,"",12,118,202,&font_ui22);lv_obj_set_style_text_align(sensorInfo,LV_TEXT_ALIGN_CENTER,0);wrapped(body,tr("Szacowany poziom na podstawie napięcia. Stan ładowania jest niedostępny.","Schätzung anhand der Spannung. Ladestatus nicht verfügbar.","Level estimated from voltage. Charging status is unavailable."),9,160,206,&font_body18,MUTED);break;
 case Screen::System:title(tr("System","System","System"),Screen::Settings);makeBody();row(&img_info,tr("Urządzenie","Gerät","Device"),"ESP32-S3 · ES3C28P",0);row(&img_info,tr("Oprogramowanie","Software","Firmware"),FIRMWARE_VERSION,45);{char b[32];snprintf(b,sizeof b,"%llu MB",(unsigned long long)(data.sdFree/1024/1024));row(&img_folder,tr("Wolne miejsce na SD","Freier SD-Speicher","Free SD storage"),data.sd?b:tr("Brak karty","Keine Karte","No card"),90);}navRow(&img_battery,tr("Bateria","Batterie","Battery"),Screen::Battery,135);navRow(&img_info,tr("Aktualizacja","Update","Update"),Screen::Update,180);{auto*b=button(body,tr("Reset ustawień","Zurücksetzen","Reset settings"),0,231,224,42);lv_obj_set_style_text_color(b,col(RED),0);bind(b,8);}break;
 case Screen::Pair:title(tr("Parowanie kamery","Kamera koppeln","Pair camera"),Screen::Network);makeBody();flowText(tr("1  Wgraj firmware ESP-CAM.\n2  Ustaw token i hasło.\n3  Direct: sieć kamery.\n    IP: 192.168.4.1\n4  Router: wspólne Wi-Fi\n    i adres IP kamery.\n5  Naciśnij Połącz.","1  Kamera-Firmware laden.\n2  Token/Passwort abgleichen.\n3  Direkt: Kamera-WLAN.\n    IP: 192.168.4.1\n4  Router: gemeinsames WLAN\n    und Kamera-IP.\n5  Verbinden drücken.","1  Flash camera firmware.\n2  Match token and password.\n3  Direct: camera Wi-Fi.\n    IP: 192.168.4.1\n4  Router: same Wi-Fi\n    and camera IP address.\n5  Tap Connect."),&font_ui20,TEXT);break;
 }
 notice=label(root,"",7,294,226,&font_ui12,RED);lv_obj_add_flag(notice,LV_OBJ_FLAG_HIDDEN);
}
void begin(Dispatch d){send=d;imageBuf=(lv_color_t*)lv_mem_alloc(Video::bytes);if(imageBuf){memset(imageBuf,0,Video::bytes);liveImage.data=(const uint8_t*)imageBuf;}for(auto&h:data.history)for(float&v:h)v=NAN;show(Screen::Home);}
Screen current(){return page;}
void frame(const uint16_t*p){if(!imageBuf||!p)return;memcpy(imageBuf,p,Video::bytes);frameReady=true;if(image)lv_obj_invalidate(image);if(page==Screen::Live)refreshLive();}
void update(const AppState&s){
 bool liveModeChanged=data.playing!=s.playing;bool wasVolume=volumeOpen;int volumeDraft=volumeSlider?lv_slider_get_value(volumeSlider):s.settings.volume;
 bool changed=data.theme!=s.theme||data.camera!=s.camera||data.audioCap!=s.audioCap||data.recording!=s.recording||data.listening!=s.listening||data.fileCount!=s.fileCount||data.settings.language!=s.settings.language||data.alertCount!=s.alertCount||data.bmeFresh!=s.bmeFresh||data.scdFresh!=s.scdFresh||data.night!=s.night||data.motionEnabled!=s.motionEnabled||data.noiseEnabled!=s.noiseEnabled;
 if(page==Screen::Network||page==Screen::WifiNetworks)changed=changed||data.wifiLink!=s.wifiLink||strcmp(data.connectedSsid,s.connectedSsid)||(page==Screen::WifiNetworks&&data.wifiScanGeneration!=s.wifiScanGeneration);
 if(page==Screen::Update)changed=changed||data.ota.state!=s.ota.state||data.ota.error!=s.ota.error||data.ota.detail!=s.ota.detail||strcmp(data.ota.version,s.ota.version);
 bool themeChanged=data.theme!=s.theme;
 bool editing=themeChanged&&editor;char draft[65]={},hint[175]={};int key=editorKey,kbPage=keyPage;bool kbUpper=keyUpper,wasJoining=joiningWifi,wasMasked=editor&&lv_textarea_get_password_mode(editor);char selectedSsid[33]={};
 if(editing){snprintf(draft,sizeof draft,"%s",lv_textarea_get_text(editor));snprintf(hint,sizeof hint,"%s",lv_label_get_text(editorHint));snprintf(selectedSsid,sizeof selectedSsid,"%s",joinSsid);}
 data=s;if(rebuild)show(pending,pendingMetric);else if(themeChanged||(page==Screen::Live&&liveModeChanged)||(changed&&page!=Screen::Metric&&page!=Screen::Scan&&page!=Screen::Live&&!editor))show(page,metric);
 if(editing){edit(key,draft,editorTitle(key));keyPage=kbPage;keyUpper=kbUpper;keyboardMap();lv_label_set_text(editorHint,hint);lv_textarea_set_password_mode(editor,wasMasked);if(revealLabel)lv_img_set_src(revealLabel,wasMasked?&img_eye:&img_eye_off);joiningWifi=wasJoining;if(wasJoining){snprintf(joinSsid,sizeof joinSsid,"%s",selectedSsid);lv_label_set_long_mode(editorHint,LV_LABEL_LONG_SCROLL_CIRCULAR);}}

 if(themeChanged&&wasVolume&&page==Screen::Live){openVolume();lv_slider_set_value(volumeSlider,volumeDraft,LV_ANIM_OFF);lv_event_send(volumeSlider,LV_EVENT_VALUE_CHANGED,nullptr);}
 if(!data.camera&&!data.playing)frameReady=false;
 if(page==Screen::Live)refreshLive();

 if(clockLabel)lv_label_set_text(clockLabel,data.timeText);if(batteryLabel){char b[12];if(data.batteryPct>=0)snprintf(b,sizeof b,"%d%%",data.batteryPct);else snprintf(b,sizeof b,"—");lv_label_set_text(batteryLabel,b);}if(wifiIcon)lv_obj_set_style_img_recolor(wifiIcon,col(page==Screen::Live?0xffffff:data.wifi?TEXT:MUTED),0);
 if(page==Screen::Sensors){for(int m=0;m<6;m++){bool valid=metricValid(data,m)&&isfinite(metricValue(data,m));char reading[32];sensorReading(m,reading,sizeof reading);lv_label_set_text(sensorValues[m],reading);lv_obj_set_style_text_font(sensorValues[m],lv_txt_get_width(reading,strlen(reading),&lv_font_montserrat_28,0,LV_TEXT_FLAG_NONE)>88?&font_ui22:&lv_font_montserrat_28,0);lv_obj_set_style_bg_color(sensorStates[m],col(valid?LIME:MUTED),0);}}

 if(page==Screen::Metric){char n[40];float v=metricValue(data,metric);if(metricValid(data,metric)&&isfinite(v))snprintf(n,sizeof n,metric==0||metric==4?"%.1f %s":"%.0f %s",v,unit(metric));else snprintf(n,sizeof n,"— %s",unit(metric));lv_label_set_text(valueLabel,n);lv_label_set_text(onlineLabel,metricValid(data,metric)?tr("● Połączony","● Verbunden","● Connected"):tr("○ Brak danych","○ Keine Werte","○ No data"));
 float lo=INFINITY,hi=-INFINITY;for(int i=0;i<HISTORY_COUNT;i++){float x=data.history[metric][i];series->y_points[i]=isfinite(x)?(lv_coord_t)(x*10):LV_CHART_POINT_NONE;if(isfinite(x)){lo=std::min(lo,x);hi=std::max(hi,x);}}
 if(isfinite(lo)){float margin=std::max(1.0f,(hi-lo)*.15f);lv_chart_set_range(chart,LV_CHART_AXIS_PRIMARY_Y,(lo-margin)*10,(hi+margin)*10);snprintf(n,sizeof n,"Min %.1f %s",lo,unit(metric));lv_label_set_text(minLabel,n);snprintf(n,sizeof n,"Max %.1f %s",hi,unit(metric));lv_label_set_text(maxLabel,n);}lv_chart_refresh(chart);}
 if(page==Screen::Scan&&sensorInfo){char line[220]={};snprintf(line,sizeof line,"%s\n",data.scanRunning?tr("Skanowanie...","Scannen...","Scanning..."):"I²C:");for(int a=8;a<120;a++)if(data.found[a]){char entry[48];snprintf(entry,sizeof entry,"0x%02X %s\n",a,a==0x38?"FT6336G":a==0x18?"ES8311":a==0x62?"SCD4x":a==0x76||a==0x77?"BME68x":"?");if(strlen(line)+strlen(entry)<sizeof line)strcat(line,entry);}lv_label_set_text(sensorInfo,line);}
 if(page==Screen::Battery){char b[40];snprintf(b,sizeof b,"%.3f V",data.batteryV);lv_label_set_text(sensorInfo,b);}
 if(page==Screen::Update&&otaProgress){bool working=Ota::installing(data.ota.state);unsigned p=std::min(100u,unsigned(data.ota.progress));lv_bar_set_value(otaProgress,p,LV_ANIM_OFF);lv_label_set_text_fmt(otaPercent,"%u%%",p);
  for(auto*o:{otaProgress,otaPercent})if(working)lv_obj_clear_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
  if(working||data.ota.state==Ota::State::Checking)lv_obj_add_state(otaCheckButton,LV_STATE_DISABLED);else lv_obj_clear_state(otaCheckButton,LV_STATE_DISABLED);
  if(data.ota.state==Ota::State::Available)lv_obj_clear_state(otaInstallButton,LV_STATE_DISABLED);else lv_obj_add_state(otaInstallButton,LV_STATE_DISABLED);
 }
 if(notice&&strcmp(lastMessage,data.message)){snprintf(lastMessage,sizeof lastMessage,"%s",data.message);lv_label_set_text(notice,data.message);lv_obj_set_style_text_color(notice,col(MUTED),0);lv_obj_clear_flag(notice,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(notice);noticeUntil=lv_tick_get()+4000;}if(notice&&noticeUntil&&(int32_t)(lv_tick_get()-noticeUntil)>0)lv_obj_add_flag(notice,LV_OBJ_FLAG_HIDDEN);lastFrames=data.frames;(void)lastFrames;
}
}
