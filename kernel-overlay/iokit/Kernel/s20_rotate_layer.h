/* Logical landscape -> portrait scanout, clockwise. All strides are bytes. */
#include <stdint.h>
static void s20_rotate_layer(uint8_t *out,uint32_t stride,uint32_t pw,uint32_t ph,
 uint32_t lw,uint32_t lh,const uint8_t *in,uint32_t in_stride,
 int32_t sx,int32_t sy,int32_t sw,int32_t sh,int32_t dx,int32_t dy,int32_t dw,int32_t dh)
{
 if(!pw||!ph||!lw||!lh||sw<=0||sh<=0||dw<=0||dh<=0)return;
 int direct=pw==lh&&ph==lw&&sw==dw&&sh==dh;
 for(uint32_t by=0;by<ph;by+=16)for(uint32_t bx=0;bx<pw;bx+=16)
  for(uint32_t py=by;py<ph&&py<by+16;py++){
   int64_t lx=direct?py:(uint64_t)py*lw/ph;
   if(lx<dx||lx>=(int64_t)dx+dw)continue;
   int64_t ix=direct?sx+lx-dx:sx+(lx-dx)*sw/dw;
   for(uint32_t px=bx;px<pw&&px<bx+16;px++){
    int64_t ly=direct?pw-1-px:(uint64_t)(pw-1-px)*lh/pw;
    if(ly<dy||ly>=(int64_t)dy+dh)continue;
    int64_t iy=direct?sy+ly-dy:sy+(ly-dy)*sh/dh;
    ((uint32_t*)(out+(uint64_t)py*stride))[px]=((const uint32_t*)(in+(uint64_t)iy*in_stride))[ix];
   }
  }
}
