#pragma once
#include <stdint.h>
#include <stddef.h>
#include <algorithm>
namespace Video {
constexpr int width=320,height=240,pixels=width*height;
constexpr size_t bytes=pixels*sizeof(uint16_t);
struct View {int zoom,x,y,width,height;};
inline View view(bool wholeFrame,int viewportWidth=240,int viewportHeight=320){
 int zx=viewportWidth*256/width,zy=viewportHeight*256/height;
 int zoom=wholeFrame?std::min(zx,zy):std::max((viewportWidth*256+width-1)/width,(viewportHeight*256+height-1)/height);
 int w=width*zoom/256,h=height*zoom/256;
 return {zoom,(viewportWidth-w)/2,(viewportHeight-h)/2,w,h};
}
// Decoder callback uses this same mapping for QVGA/VGA input, with complete
// frame bounds checked before indexing the PSRAM buffer.
inline bool decodedPixel(int x,int y,int sourceWidth,int sourceHeight,int&dx,int&dy){
 if(sourceWidth<=0||sourceHeight<=0||x<0||y<0||x>=sourceWidth||y>=sourceHeight)return false;
 dx=x*width/sourceWidth;dy=y*height/sourceHeight;return dx>=0&&dy>=0&&dx<width&&dy<height;
}
}
