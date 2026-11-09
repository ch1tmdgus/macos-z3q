"""RAM-only retry from an already connected idle xnuloader; never flashes partitions."""
from pathlib import Path
import argparse,subprocess,time,json,hashlib,struct,datetime
p=argparse.ArgumentParser();p.add_argument('--trial',type=int,required=True);p.add_argument('--root',type=Path,required=True);p.add_argument('--seconds',type=int,default=600);p.add_argument('--touch-steps',type=int,choices=[0,2,7],default=0);p.add_argument('--reuse-from-trial',type=int);p.add_argument('--graphics-fast',action='store_true');p.add_argument('--kernel',type=Path,required=True);p.add_argument('--observe-seconds',type=int,default=240);p.add_argument('--capture-frame',action='store_true');p.add_argument('--cpus',type=int,choices=[1,2,4,7],default=1);p.add_argument('--cpu-inspect',action='store_true');p.add_argument('--cpuload-ms',type=int,choices=[0,1000,2000],default=0);p.add_argument('--adreno-steps',type=int,choices=[0,3],default=0);p.add_argument('--gpu',type=int,choices=[0,1],default=0);p.add_argument('--factory-cpu-frequency',action='store_true');p.add_argument('--cpu-control',action='store_true');p.add_argument('--landscape',action='store_true');p.add_argument('--hid-input-scale',type=int,choices=[1,2],default=1);p.add_argument('--expected-kernel-sha256',required=True);a=p.parse_args()
if a.cpu_control and not a.factory_cpu_frequency:raise SystemExit('CPU control requires factory frequency table initialization')
w=Path(__file__).resolve().parent;k=a.kernel.resolve() if a.kernel else w/'s20-ventura-desktop-kernel';root=a.root.resolve()
if a.seconds!=0 and not 120<=a.seconds<=900:raise SystemExit('Invalid trial length')
if not 120<=a.observe_seconds<=900:raise SystemExit('Invalid observation length')
probe=subprocess.run(['fastboot','devices'],capture_output=True,text=True,timeout=5)
if 'xnuloader'not in probe.stdout:raise SystemExit('Phone USB loader is not enumerated; no phone action taken')
out=w.parent/'runs'/f'trial-{a.trial}';out.mkdir(parents=True,exist_ok=False)
r=dict(scope='Physical native macOS RAM desktop trial, no partition writes or Android reboot',commands=[],files={},started=datetime.datetime.now().isoformat());record=out/'trial.json'
def save():record.write_text(json.dumps(r,indent=2)+'\n')
def call(*args,timeout=30):
 c=['fastboot','-s','xnuloader',*map(str,args)];z=subprocess.run(c,capture_output=True,text=True,timeout=timeout);s=z.stdout+z.stderr;r['commands'].append(dict(command=c,returncode=z.returncode,output=s));save();print(s,flush=True);z.check_returncode();return s
def digest(path):
 h=hashlib.sha256()
 with path.open('rb')as f:
  while b:=f.read(4<<20):h.update(b)
 return dict(path=str(path),bytes=path.stat().st_size,sha256=h.hexdigest())
assert k.is_file()and root.is_file()
r['files']['kernel']=digest(k);r['files']['root']=digest(root)
if not __import__('re').fullmatch(r'[0-9a-fA-F]{64}',a.expected_kernel_sha256) or r['files']['kernel']['sha256']!=a.expected_kernel_sha256.lower():raise SystemExit('Kernel SHA-256 mismatch; nothing staged')
touch_args=f' touchscreen=1 touch_steps={a.touch_steps}' if a.touch_steps else ''
perf_args=(f' s20u-hid-input-scale={a.hid_input_scale}' if a.hid_input_scale!=1 else '')+(' s20u-landscape=1' if a.landscape else '')+(' s20u-cpu-control=1' if a.cpu_control else '')+(' cpufreq=1 ddr=0' if a.factory_cpu_frequency else '')+(' s20u-cpu-probe=1' if a.cpu_inspect else '')+(f' cpuload={a.cpuload_ms}' if a.cpuload_ms else '')+(f' adreno={a.adreno_steps} adreno_pause=0 adreno_watch=0 adreno_return=1' if a.adreno_steps else '')
record_args=' rec_secs=2 rec_start=60 rec_mb=24 rec_step=2' if a.capture_frame else ''
visual_args='fb_flip=3 fb_hwscale=1 fb_threads=1 fb_filter=0 fbtrace=0' if a.graphics_fast else 'fb_flip=0'
verbose='' if a.graphics_fast else ' -v'
cmd=f'cpus={a.cpus} bootcpu=7 loader_return_after={a.seconds}{verbose} s20u-osenvironment=recoveryos s20u-ram-recovery=1 s20u-user-exception-log=1 s20u-macos-hid=1 {visual_args} gpu={a.gpu} wifi=0 battery=0{touch_args}{record_args}{perf_args}\0'.encode()
b=k.read_bytes();entries=[(b'kernel',b),(b'cmdline',cmd)];total=sum(24+len(v)+(-len(v)%16)for n,v in entries)+24
payload=out/'kernel-ram.bin'
with payload.open('wb')as f:
 f.write(struct.pack('<II',0x50554e58,total))
 for n,v in entries:f.write(struct.pack('<16sII',n,len(v),0));f.write(v);f.write(bytes(-len(v)%16))
 f.write(bytes(24))
r['files']['payload']=digest(payload);r['bootargs']=cmd[:-1].decode();save()
state=call('getvar','result')
if 'running'in state.lower():raise SystemExit('An OS is already running; refused to replace it')
call('getvar','product')
if a.reuse_from_trial is not None:
 previous=json.loads((out.parent/f'trial-{a.reuse_from_trial}'/'trial.json').read_text())
 assert previous.get('returned') is True and 'returned' in state
 assert previous['files']['root']==r['files']['root']
 r['resident_root_reused_from_trial']=a.reuse_from_trial;save()
else:
 call('oem','ramdisk');call('getvar','max-download-size');call('stage',root,timeout=600)
call('stage',payload,timeout=60);call('oem','run');r['kernel_started']=datetime.datetime.now().isoformat();save()
start=time.monotonic();i=0
limit=a.seconds+100 if a.seconds else a.observe_seconds
while time.monotonic()-start<limit:
 time.sleep(5);state=call('getvar','result')
 if 'returned'in state:
  call('get_staged',out/'final.log',timeout=60);r['returned']=True;save();break
 if i%3==0:call('oem','log');call('get_staged',out/f'live-{i}.log')
 i+=1
else:
 r['returned']=False
 if a.seconds:save();raise SystemExit('Trial bound reached; loader state requires observation')
 r['left_running']=True;r['observation_completed']=datetime.datetime.now().isoformat();save()
if a.capture_frame and r.get('returned'):
 call('oem','recording');call('get_staged',out/'recording.bin',timeout=120)
save()
