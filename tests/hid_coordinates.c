#include "s20_hid_coordinates.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(s20_screen_fixed(609*256,0,1218,2436)==1218*256);
 assert(s20_screen_fixed(0,0,1218,2436)==0);
 assert(s20_screen_fixed(1218*256,0,1218,2436)==2436*256);
 assert(s20_screen_fixed(300*256,100,400,800)==400*256);
 assert(s20_screen_fixed(50*256,100,400,800)==-100*256);
 assert(s20_screen_fixed(44,0,0,800)==44);
 assert(s20_screen_fixed(563*256,0,563,1125)==1125*256);
 assert(s20_hid_input_fixed(609*256,2)==1218*256);assert(s20_hid_input_fixed(-400,2)==-800);assert(s20_hid_input_fixed(INT32_MAX,2)==INT32_MAX);
 puts("Retina scale, translated bounds, negative positions, invalid bounds, and noninteger scale passed");
}
