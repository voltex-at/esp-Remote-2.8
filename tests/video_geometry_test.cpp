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
 const auto portrait=Video::cameraLayout(false),landscape=Video::cameraLayout(true);
 assert(portrait.width==240&&portrait.height==320&&portrait.image.zoom==192&&portrait.image.x==0&&portrait.image.y==84&&portrait.image.height==180);
 assert(portrait.toolbarY+44<portrait.image.y&&portrait.image.y+portrait.image.height<portrait.mediaY);
 assert(portrait.emptyY>portrait.toolbarY+44&&portrait.emptyY+portrait.emptyHeight<portrait.mediaY);
 assert(landscape.width==320&&landscape.height==240&&landscape.image.zoom==256&&landscape.image.x==0&&landscape.image.y==0);
 assert(landscape.emptyY>landscape.toolbarY+44&&landscape.emptyY+landscape.emptyHeight<landscape.mediaY);
 int x,y;
 assert(!Video::decodedPixel(-1,0,320,240,x,y));
 assert(!Video::decodedPixel(0,-1,320,240,x,y));
 assert(!Video::decodedPixel(320,0,320,240,x,y));
 assert(!Video::decodedPixel(0,240,320,240,x,y));
 assert(!Video::decodedPixel(0,0,0,240,x,y));
}
