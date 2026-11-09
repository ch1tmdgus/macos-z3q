#include <stdint.h>
/* Apple IOHIDSystemCursorHelper desktop->screen affine transform, 24.8 units. */
static int32_t s20_screen_fixed(int32_t desktop_fixed,int32_t origin,int32_t extent,uint32_t pixels)
{
 if(extent<=0||!pixels)return desktop_fixed;
 int64_t result=((int64_t)desktop_fixed-(int64_t)origin*256)*pixels/extent;
 return result<INT32_MIN?INT32_MIN:result>INT32_MAX?INT32_MAX:(int32_t)result;
}

static int32_t s20_hid_input_fixed(int32_t input,uint32_t scale){
 int64_t v=(int64_t)input*scale;return v<INT32_MIN?INT32_MIN:v>INT32_MAX?INT32_MAX:(int32_t)v;
}
