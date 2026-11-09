/* SM8250 EPSS inspection, no register writes. Register layout verified against
 * Linux sm8250.dtsi and qcom-cpufreq-hw.c (epss_soc_data). */
static void
s20_cpu_probe(const ml_topology_info_t *topology)
{
 uint32_t enabled=0, domains[EPSS_DOMAINS]={0};
 if (!PE_parse_boot_argn("s20u-cpu-probe",&enabled,sizeof(enabled)) || !enabled || !redwood_platform_is("y2q")) return;
 for (unsigned i=0;i<topology->num_cpus;i++) {unsigned d=domain_of(topology->cpus[i].phys_id);if(d<EPSS_DOMAINS)domains[d]++;}
 for(unsigned d=0;d<EPSS_DOMAINS;d++) {
  if(!domains[d])continue;
  vm_offset_t base=ml_io_map(EPSS_PHYS+d*EPSS_DOMAIN_SIZE,EPSS_DOMAIN_SIZE);
  if(!base){tell("S20_CPU_DOMAIN %u map failed\n",d);continue;}
  uint32_t on=*(volatile uint32_t*)(base+0), state=*(volatile uint32_t*)(base+0x20), row=*(volatile uint32_t*)(base+EPSS_PERF_STATE), max=0, requested=0,last=0;
  if(!(on&1)){tell("S20_CPU_DOMAIN %u disabled enable=%#x\n",d,on);continue;}
  for(unsigned i=0;i<EPSS_TABLE_ROWS;i++) {
   uint32_t value=*(volatile uint32_t*)(base+EPSS_FREQ_TABLE+4*i), khz=(value>>30)?XO_KHZ*(value&255):ALTERNATE_KHZ;
   if(i==row)requested=khz;
   if(i&&khz==last)break;
   if(khz>max&&khz<=4000000)max=khz;
   last=khz;
  }
  tell("S20_CPU_DOMAIN %u cores=%u requested_row=%u requested_mhz=%u current_vote_mhz=%u factory_max_mhz=%u enable=%#x\n",d,domains[d],row,requested/1000,(state&255)*XO_KHZ/1000,max/1000,on);
 }
}
