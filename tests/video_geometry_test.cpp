#include "video_geometry.h"
#include <cassert>
#include <vector>

int main(){
 const auto whole=Video::view(true),cover=Video::view(false);
 assert(whole.width==240&&whole.height==180&&whole.x==0&&whole.y==70);
 assert(cover.width>=240&&cover.height>=320&&cover.x<=0&&cover.y<=0);
 for(int scale:{1,2}){
  std::vector<bool> visited(Video::pixels);
  for(int y=0;y<240*scale;y++)for(int x=0;x<320*scale;x++){
   int dx,dy;assert(Video::decodedPixel(x,y,320*scale,240*scale,dx,dy));
   assert(dx==x/scale&&dy==y/scale);visited[dy*Video::width+dx]=true;
  }
  for(bool covered:visited)assert(covered);
 }
 int x,y;
 assert(!Video::decodedPixel(-1,0,320,240,x,y));
 assert(!Video::decodedPixel(0,-1,320,240,x,y));
 assert(!Video::decodedPixel(320,0,320,240,x,y));
 assert(!Video::decodedPixel(0,240,320,240,x,y));
 assert(!Video::decodedPixel(0,0,0,240,x,y));
}
