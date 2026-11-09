/* y2q/z3q only. EPSS selects firmware-calibrated rates/voltages. TSENS v2
 * status is signed 12-bit tenths C, valid bit 21 (Linux tsens-v2.c).
 * No writes to thermal configuration, voltage registers, or memory clocks. */
static vm_offset_t s20_epss[EPSS_DOMAINS], s20_tsens[2];
static unsigned s20_domains[EPSS_DOMAINS];
static unsigned
s20_row_for_cap(vm_offset_t base, unsigned cap_mhz)
{
 unsigned best=0,last=0;
 for(unsigned i=0;i<EPSS_TABLE_ROWS;i++){
  uint32_t v=*(volatile uint32_t*)(base+EPSS_FREQ_TABLE+4*i);
  unsigned khz=(v>>30)?XO_KHZ*(v&255):ALTERNATE_KHZ;
  if(i&&khz==last)break;
  if(khz<=cap_mhz*1000)best=i;
  last=khz;
 }
 return best;
}
static void
s20_cpu_control_thread(void *arg __unused,wait_result_t wr __unused)
{
 const ml_topology_info_t *topology=ml_get_topology_info();
 uint64_t idle_before[MAX_CPUS]={0},then=mach_absolute_time();unsigned thermal=0,iteration=0,boost_ticks=0;
 for(;;){
  delay_for_interval(100,NSEC_PER_MSEC);
  uint64_t now=mach_absolute_time(),total=0,busy=0;int peak=-2048;unsigned valid=0;
  for(unsigned i=0;i<topology->num_cpus&&i<MAX_CPUS;i++){
   uint64_t idle=0;if(!ml_cpu_idle_time(i,&idle))continue;
   uint64_t span=now-then,delta=idle-idle_before[i];idle_before[i]=idle;total+=span;if(delta<span)busy+=span-delta;
  }
  then=now;
  for(unsigned block=0;block<2;block++)for(unsigned i=0;i<(block?9u:16u);i++){
   uint32_t status=*(volatile uint32_t*)(s20_tsens[block]+0xa0+4*i);
   if(!(status&(1u<<21)))continue;
   int temp=(int32_t)(status<<20)>>20;
   if(temp < -400 || temp > 1500)continue;
   valid++;if(temp>peak)peak=temp;
  }
  if(!valid)thermal=3;
  else if(peak>=850)thermal=2;
  else if(peak>=750){if(thermal!=2||peak<800)thermal=1;}
  else if(peak<700)thermal=0;
  else if(thermal==2)thermal=1;
  unsigned load=total?(unsigned)(busy*100/total):0;
  if(load>=5)boost_ticks=10;else if(boost_ticks)boost_ticks--;
  unsigned cap=thermal==2?900:thermal==1?1800:thermal==3?1400:boost_ticks?4000:1400;
  for(unsigned d=1;d<EPSS_DOMAINS;d++)if(s20_domains[d]){
   unsigned row=s20_row_for_cap(s20_epss[d],cap);
   if(*(volatile uint32_t*)(s20_epss[d]+EPSS_PERF_STATE)!=row){
    *(volatile uint32_t*)(s20_epss[d]+EPSS_PERF_STATE)=row;
    tell("S20_CPU_CONTROL domain=%u row=%u cap_mhz=%u peak_tenths_c=%d sensors=%u load=%u\n",d,row,cap,peak,valid,load);
   }
  }
  if(!(iteration++%100))tell("S20_CPU_THERMAL peak_tenths_c=%d sensors=%u level=%u load=%u\n",peak,valid,thermal,load);
 }
}
static void
s20_cpu_control_start(const ml_topology_info_t *topology)
{
 uint32_t enabled=0;thread_t thread;
 if(!redwood_platform_is("y2q")||!PE_parse_boot_argn("s20u-cpu-control",&enabled,sizeof(enabled))||!enabled)return;
 for(unsigned i=0;i<topology->num_cpus;i++){unsigned d=domain_of(topology->cpus[i].phys_id);if(d<EPSS_DOMAINS)s20_domains[d]++;}
 for(unsigned d=1;d<EPSS_DOMAINS;d++)if(s20_domains[d]){s20_epss[d]=ml_io_map(EPSS_PHYS+d*EPSS_DOMAIN_SIZE,EPSS_DOMAIN_SIZE);if(!s20_epss[d])return;}
 s20_tsens[0]=ml_io_map(0x0c263000,0x1000);s20_tsens[1]=ml_io_map(0x0c265000,0x1000);if(!s20_tsens[0]||!s20_tsens[1])return;
 if(kernel_thread_start(s20_cpu_control_thread,NULL,&thread)==KERN_SUCCESS){thread_deallocate(thread);tell("S20_CPU_CONTROL started: load-responsive factory LUT, thermal caps 75/85C\n");}
}
