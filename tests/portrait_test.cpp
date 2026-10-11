#include "portrait_geometry.h"
#include <assert.h>
#include <stdio.h>
extern "C" int __lsan_is_turned_off(){return 1;}
int main(){int16_t x=-1,y=-1;
 assert(Portrait::touch(0,0,x,y,0)&&x==0&&y==0);
 assert(Portrait::touch(239,319,x,y,0)&&x==239&&y==319);
 assert(Portrait::touch(50,70,x,y,0)&&x==50&&y==70);
 assert(Portrait::touch(0,0,x,y,2)&&x==239&&y==319);
 assert(Portrait::touch(239,319,x,y,2)&&x==0&&y==0);
 assert(Portrait::touch(50,70,x,y,2)&&x==189&&y==249);
 assert(!Portrait::touch(240,20,x,y,0)&&!Portrait::touch(20,320,x,y,0));
 assert(!Portrait::touch(20,20,x,y,1)&&!Portrait::touch(20,20,x,y,3));
 int32_t a=-5,b=-7,w=20,h=30;assert(Portrait::clip(a,b,w,h)&&a==0&&b==0&&w==15&&h==23);
 a=235;b=310;w=30;h=30;assert(Portrait::clip(a,b,w,h)&&w==5&&h==10);
 a=240;b=0;w=1;h=1;assert(!Portrait::clip(a,b,w,h));
 a=-20;b=0;w=10;h=1;assert(!Portrait::clip(a,b,w,h));
 a=0;b=0;w=0;h=10;assert(!Portrait::clip(a,b,w,h));
 printf("Portrait: coordinates, 0/180 rotation, bounds and clipping OK.\n");
}
