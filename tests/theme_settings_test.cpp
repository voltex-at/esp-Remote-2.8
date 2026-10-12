#include "app_state.h"
#include "ui_palette.h"
#include <assert.h>
#include <initializer_list>
#include <math.h>
#include <stdio.h>
#include <string.h>

struct Store {
    bool present=false, failWrite=false;
    uint8_t value=0;
    unsigned writes=0;
    unsigned char legacy[sizeof(Settings)];
    Store(){memset(legacy,0x5a,sizeof legacy);}
    uint8_t getUChar(const char* key,uint8_t fallback){assert(!strcmp(key,"theme"));return present?value:fallback;}
    size_t putUChar(const char* key,uint8_t next){assert(!strcmp(key,"theme"));writes++;if(failWrite)return 0;present=true;value=next;return 1;}
};
// Check colors after the same RGB565 quantization used by the LCD.
static double channel(unsigned c){double n=c/255.0;return n<=0.04045?n/12.92:pow((n+0.055)/1.055,2.4);}
static double luminance(uint32_t c){unsigned r=(c>>16)&255,g=(c>>8)&255,b=c&255;r=(r>>3)*255/31;g=(g>>2)*255/63;b=(b>>3)*255/31;return .2126*channel(r)+.7152*channel(g)+.0722*channel(b);}
static double contrast(uint32_t a,uint32_t b){double x=luminance(a),y=luminance(b);return x>y?(x+.05)/(y+.05):(y+.05)/(x+.05);}
int main(){
    Store store;unsigned char original[sizeof(Settings)];memcpy(original,store.legacy,sizeof original);
    assert(ThemeSettings::load(store)==ThemeMode::Dark); // Upgrade without the new key.
    assert(ThemeSettings::save(store,ThemeMode::Light));
    AppState reboot;reboot.theme=ThemeSettings::load(store);assert(reboot.theme==ThemeMode::Light);
    assert(!memcmp(original,store.legacy,sizeof original));
    assert(ThemeSettings::save(store,ThemeMode::Dark));assert(ThemeSettings::load(store)==ThemeMode::Dark);
    for(auto invalid:{2,3,127,255}){store.value=invalid;assert(ThemeSettings::load(store)==ThemeMode::Dark);}
    assert(ThemeSettings::save(store,ThemeMode(255))&&store.value==0);
    store.failWrite=true;assert(!ThemeSettings::save(store,ThemeMode::Light));assert(store.value==0);
    for(bool home:{false,true}){
        auto p=UiColors::get(ThemeMode::Light,home);
        for(auto background:{p.background,p.card,p.selected})for(auto ink:{p.text,p.muted,p.accent,p.error,p.blue}){
            double ratio=contrast(ink,background);if(ratio<4.5)fprintf(stderr,"Contrast %06x / %06x = %.2f\n",ink,background,ratio);assert(ratio>=4.5);
        }
        assert(contrast(p.accentInk,p.accent)>=4.5&&contrast(p.errorInk,p.error)>=4.5);
    }
    auto home=UiColors::get(ThemeMode::Dark,true);assert(home.background==0&&home.accent==0x9aff32&&home.text==0xffffff);
    puts("Theme settings: default, upgrade, reboot persistence, corrupt values, failed write, unchanged schema1 blob and light RGB565 contrast OK.");
}
