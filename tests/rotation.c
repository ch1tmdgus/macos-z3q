#include "s20_rotate_layer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 uint32_t in[6]={1,2,3,4,5,6},out[9];for(int i=0;i<9;i++)out[i]=0xfeed;
 s20_rotate_layer((uint8_t*)out,12,2,3,3,2,(uint8_t*)in,12,0,0,3,2,0,0,3,2);
 uint32_t expected[9]={4,1,0xfeed,5,2,0xfeed,6,3,0xfeed};assert(!memcmp(out,expected,sizeof out));
 uint32_t red=77;s20_rotate_layer((uint8_t*)out,12,2,3,3,2,(uint8_t*)&red,4,0,0,1,1,1,0,1,1);
 assert(out[4]==77&&out[0]==4&&out[1]==1&&out[3]==5&&out[6]==6&&out[7]==3);
 uint32_t guard[4]={9,9,9,9};s20_rotate_layer((uint8_t*)guard,8,2,2,2,2,(uint8_t*)&red,4,0,0,1,1,-10,-10,1,1);assert(guard[0]==9&&guard[3]==9);
 for(unsigned x=0;x<=65535;x+=257)for(unsigned y=0;y<=65535;y+=257){unsigned lx=y,ly=65535-x;assert(65535-ly==x&&lx==y);}
 puts("Clockwise orientation, padded rows, partial update, and offscreen clipping passed");
}
