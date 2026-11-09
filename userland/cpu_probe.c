#include <mach/mach.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <unistd.h>
static uint64_t ns(void){mach_timebase_info_data_t t;mach_timebase_info(&t);return mach_absolute_time()*t.numer/t.denom;}
static int number(const char*n){int v=-1;size_t s=sizeof(v);sysctlbyname(n,&v,&s,0,0);return v;}
static double used(void){struct rusage r;getrusage(RUSAGE_SELF,&r);return r.ru_utime.tv_sec+r.ru_stime.tv_sec+(r.ru_utime.tv_usec+r.ru_stime.tv_usec)/1e6;}
static void* work(void*x){volatile uint64_t s=0;for(uint64_t i=0;i<60000000;i++)s+=i*i;*(uint64_t*)x=s;return 0;}
int main(int argc,char**argv){setbuf(stdout,0);char args[4096]={0};size_t size=sizeof(args);if(sysctlbyname("kern.bootargs",args,&size,0,0)||!strstr(args,"s20u-ram-recovery=1"))return 2;
 unsigned delay=argc>1?(unsigned)atoi(argv[1]):90;if(delay>180)return 3;sleep(delay);
 int active=number("hw.activecpu"),count=active>0?active:1;if(count>8)count=8;
 printf("S20_CPU_TOPOLOGY ncpu=%d active=%d physical=%d logical=%d\n",number("hw.ncpu"),active,number("hw.physicalcpu"),number("hw.logicalcpu"));
 for(unsigned round=0;round<3;round++) {pthread_t ts[8];uint64_t sums[8]={0},start=ns();work(sums);double single=(ns()-start)/1e6,begin=used();start=ns();int launched=0;
  for(int i=0;i<count;i++){if(pthread_create(&ts[i],0,work,&sums[i]))break;launched++;}
  for(int i=0;i<launched;i++)pthread_join(ts[i],0);
  double wall=(ns()-start)/1e9,cpu=used()-begin;int valid=launched==count;for(int i=1;i<launched;i++)if(sums[i]!=sums[0])valid=0;
  printf("S20_CPU_BENCH round=%u threads=%d single_ms=%.2f parallel_ms=%.2f cpu_ms=%.2f parallel_cpu=%.2f throughput_gain=%.2f checksum_valid=%d\n",round,launched,single,wall*1000,cpu*1000,wall?cpu/wall:0,wall?count*single/(wall*1000):0,valid);
 }
 return 0;}
