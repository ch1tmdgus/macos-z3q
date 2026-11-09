/* Guest-only bridge from real redwood-touch samples to native macOS mouse events.
 * No multitouch gesture implementation. The kernel touch driver remains opt-in.
 */
#include <ApplicationServices/ApplicationServices.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "map.h"
_Static_assert(sizeof(struct touch_sample)==24,"redwood touch ABI");
int main(void){
 setbuf(stdout,0);char args[4096]={0};size_t len=sizeof(args);
 if(sysctlbyname("kern.bootargs",args,&len,0,0)||!strstr(args,"s20u-macos-hid=1")){fputs("Guest macOS HID profile required\n",stderr);return 2;}
 CGRect bounds=CGRectZero;
 for(unsigned i=0;i<120;i++){bounds=CGDisplayBounds(CGMainDisplayID());if(bounds.size.width>=1&&bounds.size.height>=1)break;sleep(1);}
 if(bounds.size.width<1||bounds.size.height<1)return 3;
 struct pointer_state state={0,0,bounds.origin.x,bounds.origin.y,bounds.size.width,bounds.size.height};
 int fd=open("/dev/redwood-touch",O_RDONLY|O_CLOEXEC);if(fd<0){perror("redwood-touch");return 4;}
 printf("S20_TOUCH_BRIDGE_READY bounds=%.0f,%.0f %.0fx%.0f\n",state.x,state.y,state.width,state.height);
 const CGEventType types[]={kCGEventNull,kCGEventMouseMoved,kCGEventLeftMouseDown,kCGEventLeftMouseDragged,kCGEventLeftMouseUp};
 unsigned long count=0;
 for(;;){struct touch_sample samples[16];ssize_t n=read(fd,samples,sizeof(samples));if(n<0&&errno==EINTR)continue;if(n<=0||n%sizeof(samples[0]))break;
  for(unsigned i=0;i<(unsigned)n/sizeof(samples[0]);i++){struct pointer_event events[2];unsigned total=map_touch(&state,&samples[i],events);
   for(unsigned j=0;j<total;j++){
    CGEventRef e=CGEventCreateMouseEvent(NULL,types[events[j].kind],CGPointMake(events[j].x,events[j].y),kCGMouseButtonLeft);
    if(!e){close(fd);return 5;}
    if(events[j].kind==POINTER_MOVE||events[j].kind==POINTER_DRAG){CGError err=CGWarpMouseCursorPosition(CGPointMake(events[j].x,events[j].y));if(err&&count<12)printf("S20_TOUCH_WARP_ERROR %d\n",err);}
    CGEventSetIntegerValueField(e,kCGMouseEventClickState,1);CGEventPost(kCGSessionEventTap,e);CFRelease(e);
    if(++count<=12||count%100==0)printf("S20_TOUCH_POST %lu kind=%d at=%.0f,%.0f\n",count,events[j].kind,events[j].x,events[j].y);
   }
  }
 }
 close(fd);return 0;
}
