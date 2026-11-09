#include <stdint.h>
struct touch_sample {uint64_t time;uint32_t type,finger,x,y;};
struct pointer_state {int down;uint32_t finger;double x,y,width,height;};
struct pointer_event {int kind;double x,y;};
enum {POINTER_MOVE=1,POINTER_DOWN,POINTER_DRAG,POINTER_UP};
static unsigned map_touch(struct pointer_state*s,const struct touch_sample*t,struct pointer_event out[2]){
 if(t->x>65535||t->y>65535||t->finger>=32||s->width<1||s->height<1)return 0;
 double x=s->x+(double)t->x*(s->width-1)/65535.,y=s->y+(double)t->y*(s->height-1)/65535.;
 if(t->type==1){
  if(s->down){if(s->finger!=t->finger)return 0;out[0]=(struct pointer_event){POINTER_DRAG,x,y};return 1;}
  s->down=1;s->finger=t->finger;out[0]=(struct pointer_event){POINTER_MOVE,x,y};out[1]=(struct pointer_event){POINTER_DOWN,x,y};return 2;
 }
 if(!s->down||s->finger!=t->finger)return 0;
 if(t->type==2){out[0]=(struct pointer_event){POINTER_DRAG,x,y};return 1;}
 if(t->type==3){s->down=0;out[0]=(struct pointer_event){POINTER_UP,x,y};return 1;}
 return 0;
}
