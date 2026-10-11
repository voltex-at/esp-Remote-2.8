#pragma once
#include <stdint.h>
namespace Portrait {
constexpr int16_t width=240,height=320;
inline bool touch(uint16_t rawX,uint16_t rawY,int16_t&x,int16_t&y,uint8_t rotation){
 if(rawX>=width||rawY>=height||(rotation!=0&&rotation!=2))return false;
 x=rotation==2?width-1-rawX:rawX;y=rotation==2?height-1-rawY:rawY;return true;
}
inline bool clip(int32_t&x,int32_t&y,int32_t&w,int32_t&h){
 if(w<=0||h<=0||x>=width||y>=height)return false;
 if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}
 if(w<=0||h<=0)return false;
 if(w>width-x)w=width-x;if(h>height-y)h=height-y;return true;
}
}
