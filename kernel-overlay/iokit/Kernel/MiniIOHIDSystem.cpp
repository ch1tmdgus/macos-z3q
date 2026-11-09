/* S20 RAM macOS bring-up: software HID event system, no physical HID device.
 * ABI: Apple's IOHIDFamily 1915.120.2, IOHIDShared.h and IOHIDUserClient.cpp.
 * Shared memory, bounds, cursor state and the legacy event queue are real.
 * Unsupported calls fail explicitly. Hardware touch/keyboard are not supplied.
 */
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOSharedDataQueue.h>
#include <IOKit/IOLocks.h>
#include <libkern/c++/OSDictionary.h>
#include <libkern/c++/OSBoolean.h>
#include <libkern/c++/OSNumber.h>
#include <libkern/c++/OSArray.h>
#include <kern/clock.h>
#include <pexpert/pexpert.h>
#include "MiniHIDABI/IOHIDShared.h"
#include <libkern/OSAtomic.h>
#include "s20_hid_coordinates.h"
extern "C" void s20_display_pixels(uint32_t *,uint32_t *);
extern "C" void redwood_tl(const char *,...) __printflike(1,2);
extern "C" {
#include <console/video_console.h>
extern struct vc_info vinfo;
}
class IOHIDSystem;
class S20HIDUserClient: public IOUserClient {
 OSDeclareDefaultStructors(S20HIDUserClient);
 IOHIDSystem *owner;uint32_t kind;IOSharedDataQueue *queue;
public:
 virtual bool initWithTask(task_t t,void*s,UInt32 k,OSDictionary*p) APPLE_KEXT_OVERRIDE;
 virtual bool start(IOService *p) APPLE_KEXT_OVERRIDE;
 virtual IOService *getService(void) APPLE_KEXT_OVERRIDE;
 virtual IOReturn clientClose(void) APPLE_KEXT_OVERRIDE;
 virtual void free(void) APPLE_KEXT_OVERRIDE;
 virtual IOReturn clientMemoryForType(UInt32 t,UInt32*f,IOMemoryDescriptor**m) APPLE_KEXT_OVERRIDE;
 virtual IOReturn registerNotificationPort(mach_port_t p,UInt32 t,io_user_reference_t r) APPLE_KEXT_OVERRIDE;
 virtual IOReturn externalMethod(uint32_t s,IOExternalMethodArguments*a,IOExternalMethodDispatch*d,OSObject*t,void*r) APPLE_KEXT_OVERRIDE;
};
class IOHIDSystem: public IOService {
 OSDeclareDefaultStructors(IOHIDSystem);
public:
 IOBufferMemoryDescriptor *memory;EvGlobals *g;IOLock *lock;bool events,cursor;uint32_t displayMask;
 virtual bool start(IOService*p) APPLE_KEXT_OVERRIDE;
 virtual void free(void) APPLE_KEXT_OVERRIDE;
 virtual IOReturn newUserClient(task_t t,void*s,UInt32 k,OSDictionary*p,IOUserClient**out) APPLE_KEXT_OVERRIDE;
 void reset(void);
 IOReturn call(uint32_t selector,IOExternalMethodArguments*a,uint32_t kind);
};
OSDefineMetaClassAndStructors(IOHIDSystem,IOService);
OSDefineMetaClassAndStructors(S20HIDUserClient,IOUserClient);
void IOHIDSystem::reset(void){
 bzero(memory->getBytesNoCopy(),memory->getCapacity());EvOffsets*o=(EvOffsets*)memory->getBytesNoCopy();o->evGlobalsOffset=sizeof(EvOffsets);o->evShmemOffset=sizeof(EvOffsets)+sizeof(EvGlobals);
 g=(EvGlobals*)((char*)memory->getBytesNoCopy()+sizeof(EvOffsets));g->version=kIOHIDCurrentShmemVersion;g->structSize=sizeof(EvGlobals);g->eNum=13;g->waitThreshold=72;
 g->workBounds.minx=0;g->workBounds.miny=0;g->workBounds.maxx=(SInt16)vinfo.v_width;g->workBounds.maxy=(SInt16)vinfo.v_height;
 g->cursorLoc.x=g->workBounds.maxx/2;g->cursorLoc.y=g->workBounds.maxy/2;g->desktopCursorFixed.x=g->cursorLoc.x<<8;g->desktopCursorFixed.y=g->cursorLoc.y<<8;g->screenCursorFixed.x=g->desktopCursorFixed.x;g->screenCursorFixed.y=g->desktopCursorFixed.y;
 for(unsigned i=0;i<LLEQSIZE;i++)g->lleq[i].next=(i+1)%LLEQSIZE;g->LLEHead=1;g->LLETail=1;g->LLELast=0;
}
bool IOHIDSystem::start(IOService*p){
 if(!IOService::start(p))return false;lock=IOLockAlloc();if(!lock)return false;
 memory=IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task,kIODirectionInOut|kIOMemoryKernelUserShared,((sizeof(EvOffsets)+sizeof(EvGlobals)+0x3fff)&~0x3fff),0x4000);if(!memory)return false;
 reset();setName("IOHIDSystem");
 /* NXClickTime consults this dictionary while WindowServer is still
  * starting its HID Mach service. Missing values send it into a synchronous
  * request to that same not-yet-serving process. Publish Apple's ordinary
  * nanosecond defaults (ev_private.h and IOHIKeyboard.h), as OSNumbers. */
 OSDictionary*params=OSDictionary::withCapacity(5);if(!params)return false;
 const char*keys[]={"HIDClickTime","HIDKeyRepeat","HIDInitialKeyRepeat"};
 const uint64_t values[]={500000000ULL,83333333ULL,500000000ULL};
 for(unsigned i=0;i<3;i++){OSNumber*n=OSNumber::withNumber(values[i],64);if(!n){params->release();return false;}bool ok=params->setObject(keys[i],n);n->release();if(!ok){params->release();return false;}}
 /* NXGetClickSpace expects an array of two 32-bit numbers. */
 OSArray*space=OSArray::withCapacity(2);if(!space){params->release();return false;}
 for(unsigned i=0;i<2;i++){OSNumber*n=OSNumber::withNumber(3,32);if(!n){space->release();params->release();return false;}bool ok=space->setObject(n);n->release();if(!ok){space->release();params->release();return false;}}
 bool haveSpace=params->setObject("HIDClickSpace",space);space->release();if(!haveSpace){params->release();return false;}
 params->setObject("HIDDefaultParameters",kOSBooleanTrue);
 setProperty("HIDParameters",params);params->release();setProperty("S20SoftwareEventSystem",kOSBooleanTrue);
 registerService();IOLog("S20HID: registered software IOHIDSystem, shared memory %u bytes, EvGlobals %u, version %u\n",(unsigned)memory->getCapacity(),(unsigned)sizeof(EvGlobals),kIOHIDCurrentShmemVersion);return true;
}
void IOHIDSystem::free(void){if(memory)memory->release();if(lock)IOLockFree(lock);IOService::free();}
IOReturn IOHIDSystem::newUserClient(task_t t,void*s,UInt32 k,OSDictionary*p,IOUserClient**out){
 if(k!=0&&k!=1&&k!=3)return kIOReturnUnsupported;
 S20HIDUserClient*c=new S20HIDUserClient;if(!c)return kIOReturnNoMemory;
 if(!c->initWithTask(t,s,k,p)||!c->attach(this)||!c->start(this)){c->detach(this);c->release();return kIOReturnError;}*out=c;IOLog("S20HID: user client type %u opened\n",k);return kIOReturnSuccess;
}
bool S20HIDUserClient::initWithTask(task_t t,void*s,UInt32 k,OSDictionary*p){kind=k;return IOUserClient::initWithTask(t,s,k,p);}
bool S20HIDUserClient::start(IOService*p){owner=OSDynamicCast(IOHIDSystem,p);return owner&&IOUserClient::start(p);}
IOService *S20HIDUserClient::getService(void){return owner;}
IOReturn S20HIDUserClient::clientClose(void){terminate();return kIOReturnSuccess;}
void S20HIDUserClient::free(void){if(queue)queue->release();IOUserClient::free();}
IOReturn S20HIDUserClient::clientMemoryForType(UInt32 t,UInt32*f,IOMemoryDescriptor**m){
 IOMemoryDescriptor*md=kind==3?(t==100&&queue?queue->getMemoryDescriptor():NULL):(t==0&&owner?owner->memory:NULL);if(!md)return kIOReturnBadArgument;md->retain();*f=0;*m=md;IOLog("S20HID: shared memory mapped type %u\n",t);return kIOReturnSuccess;
}
IOReturn S20HIDUserClient::registerNotificationPort(mach_port_t p,UInt32 t,io_user_reference_t){
 if(kind==3&&t==100&&queue){queue->setNotificationPort(p);return kIOReturnSuccess;}
 if(kind!=3&&t==0){releaseNotificationPort(p);return kIOReturnSuccess;}return kIOReturnUnsupported;
}
IOReturn S20HIDUserClient::externalMethod(uint32_t s,IOExternalMethodArguments*a,IOExternalMethodDispatch*,OSObject*,void*){
 static unsigned trace[64];if(s<64&&trace[s]++<8)IOLog("S20HID: client %u selector %u scalar %u/%u struct %u/%u\n",kind,s,a->scalarInputCount,a->scalarOutputCount,(unsigned)a->structureInputSize,(unsigned)a->structureOutputSize);
 if(kind==3){
  if(s==0&&a->scalarInputCount==2&&a->scalarOutputCount==1){uint64_t size=a->scalarInput[1];if(size<64||size>(1<<20))return kIOReturnBadArgument;if(!queue)queue=IOSharedDataQueue::withCapacity((UInt32)size);if(!queue)return kIOReturnNoMemory;a->scalarOutput[0]=100;return kIOReturnSuccess;}
  if(s==1&&a->scalarInputCount==2){if(queue){queue->release();queue=NULL;}return kIOReturnSuccess;}
  if(s==2&&a->scalarInputCount==1)return kIOReturnSuccess;
  return kIOReturnUnsupported;
 }
 IOLockLock(owner->lock);IOReturn result=owner->call(s,a,kind);IOLockUnlock(owner->lock);return result;
}
IOReturn IOHIDSystem::call(uint32_t s,IOExternalMethodArguments*a,uint32_t kind){
 if(s<=2&&kind==0&&a->scalarInputCount==1){if(s==0)reset();else if(s==1)events=!!a->scalarInput[0];else cursor=!!a->scalarInput[0];return kIOReturnSuccess;}
 if(s==3&&a->structureInput&&a->structureInputSize>=sizeof(evioLLEvent)&&a->structureInputSize<=sizeof(evioLLEvent)+4){
  evioLLEvent e;bcopy(a->structureInput,&e,sizeof(e));int tail=g->LLETail;if(tail<0||tail>=LLEQSIZE)return kIOReturnBadArgument;int next=g->lleq[tail].next;if(next<0||next>=LLEQSIZE)return kIOReturnBadArgument;if(next==g->LLEHead)return kIOReturnNoResources;
  NXEvent n;bzero(&n,sizeof(n));n.type=e.type;n.location.x=e.location.x;n.location.y=e.location.y;n.flags=e.setFlags?e.flags:g->eventFlags;clock_get_uptime(&n.time);bcopy(&e.data,&n.data,sizeof(n.data));bcopy(&n,(void*)&g->lleq[tail].event,sizeof(n));g->LLELast=tail;OSMemoryBarrier();g->LLETail=next;return kIOReturnSuccess;
 }
 if(s==4&&a->structureInput){
  int32_t fixed[2];
  if(a->structureInputSize==28){
   /* Apple's packed SetFixedMouseLocData: two timestamps, 24.8 x/y,
    * then pid. The leading bytes are not an IOGPoint. */
   bcopy((const char*)a->structureInput+16,fixed,sizeof(fixed));
  }else if(a->structureInputSize==sizeof(IOGPoint)){
   IOGPoint p;bcopy(a->structureInput,&p,sizeof(p));fixed[0]=(int32_t)p.x*256;fixed[1]=(int32_t)p.y*256;
  }else return kIOReturnBadArgument;
  /* The virtual display advertises pixel desktop bounds, while CGWarp's
   * caller uses CoreGraphics points. Explicit scale selects this measured
   * recovery-display mode; normal HID clients keep the default identity. */
  uint32_t input_scale=1;PE_parse_boot_argn("s20u-hid-input-scale",&input_scale,sizeof(input_scale));
  if(input_scale<1||input_scale>4)input_scale=1;
  fixed[0]=s20_hid_input_fixed(fixed[0],input_scale);fixed[1]=s20_hid_input_fixed(fixed[1],input_scale);
  /* Match Apple's cursorSema protocol: the WindowServer shares these fields.
   * Its software cursor must not see a partly updated desktop/screen pair. */
  if(!OSCompareAndSwap(0,1,(volatile UInt32 *)&g->cursorSema))return kIOReturnBusy;
  int32_t x=fixed[0]/256,y=fixed[1]/256;uint32_t pw=0,ph=0;s20_display_pixels(&pw,&ph);
  g->cursorLoc.x=(SInt16)(x<-32768?-32768:x>32767?32767:x);g->cursorLoc.y=(SInt16)(y<-32768?-32768:y>32767?32767:y);
  g->desktopCursorFixed.x=fixed[0];g->desktopCursorFixed.y=fixed[1];
  g->screenCursorFixed.x=s20_screen_fixed(fixed[0],g->workBounds.minx,g->workBounds.maxx-g->workBounds.minx,pw);
  g->screenCursorFixed.y=s20_screen_fixed(fixed[1],g->workBounds.miny,g->workBounds.maxy-g->workBounds.miny,ph);
  static unsigned positions;if(positions++<12)redwood_tl("S20_HID_COORD desktop=%d,%d screen=%d,%d desktop_bounds=%d,%d..%d,%d pixels=%u,%u\n",fixed[0],fixed[1],g->screenCursorFixed.x,g->screenCursorFixed.y,g->workBounds.minx,g->workBounds.miny,g->workBounds.maxx,g->workBounds.maxy,pw,ph);
  OSMemoryBarrier();*(volatile UInt32 *)&g->cursorSema=0;return kIOReturnSuccess;
 }
 if(s==5&&a->scalarInputCount==1&&a->scalarOutputCount==1){a->scalarOutput[0]=kind==0?g->eNum:g->eventFlags;return kIOReturnSuccess;}
 if(s==6&&kind==0&&a->structureInput&&a->structureInputSize==sizeof(IOGBounds)){IOGBounds b;bcopy(a->structureInput,&b,sizeof(b));if(b.minx>=b.maxx||b.miny>=b.maxy)return kIOReturnBadArgument;bcopy(&b,(void*)&g->workBounds,sizeof(b));return kIOReturnSuccess;}
 if(s==6&&kind==1&&a->scalarInputCount==2){g->eventFlags=(int)a->scalarInput[1];return kIOReturnSuccess;}
 if(s==7&&a->scalarInputCount==0&&a->scalarOutputCount==1){for(unsigned i=0;i<16;i++)if(!(displayMask&(1U<<i))){displayMask|=1U<<i;a->scalarOutput[0]=i+1;return kIOReturnSuccess;}return kIOReturnNoResources;}
 if(s==8&&a->scalarInputCount==1){unsigned n=(unsigned)a->scalarInput[0];if(!n||n>16||!(displayMask&(1U<<(n-1))))return kIOReturnBadArgument;displayMask&=~(1U<<(n-1));return kIOReturnSuccess;}
 if(s==9&&a->scalarInputCount==5){unsigned n=(unsigned)a->scalarInput[0];if(!n||n>16||!(displayMask&(1U<<(n-1))))return kIOReturnBadArgument;g->workBounds.minx=(SInt16)a->scalarInput[1];g->workBounds.maxx=(SInt16)a->scalarInput[2];g->workBounds.miny=(SInt16)a->scalarInput[3];g->workBounds.maxy=(SInt16)a->scalarInput[4];return kIOReturnSuccess;}
 if(s==10&&a->scalarOutputCount==1){a->scalarOutput[0]=0;return kIOReturnSuccess;}
 if(s==11&&a->scalarInputCount==1){cursor=!!a->scalarInput[0];return kIOReturnSuccess;}
 return kIOReturnUnsupported;
}
void MiniIOHIDSystemStart(IOService*){
 int enabled=0;PE_parse_boot_argn("s20u-macos-hid",&enabled,sizeof(enabled));if(!enabled)return;
 /* NXOpenEventStatus uses the absolute IOService:/IOResources/IOHIDSystem
  * path, not class matching. Publish under its actual expected provider. */
 IOService*p=IOService::getResourceService();if(!p){IOLog("S20HID: IOResources unavailable\n");return;}
 IOHIDSystem*s=new IOHIDSystem;if(s){if(!s->init()||!s->attach(p)||!s->start(p)){s->detach(p);IOLog("S20HID: startup failed\n");}s->release();}
}
