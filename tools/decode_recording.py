"""Decode native MiniIOMobileFramebuffer RWREC003 RGB frames, without edits."""
from pathlib import Path
import argparse,struct,zlib,binascii,json,hashlib
p=argparse.ArgumentParser();p.add_argument('recording',type=Path);p.add_argument('directory',type=Path);a=p.parse_args();b=a.recording.read_bytes()
magic,w,h,n,oldest,began=struct.unpack_from('<8sIIIIQ',b);assert magic==b'RWREC003';assert 0<w<=8192 and 0<h<=8192 and 0<n<=4096;assert oldest<n
frame_size=8+w*h*3;assert len(b)==32+n*frame_size;a.directory.mkdir(exist_ok=True);selected=sorted(set([0,n//2,n-1]));frames=[]
def chunk(t,d):return struct.pack('>I',len(d))+t+d+struct.pack('>I',binascii.crc32(t+d))
for i in selected:
 slot=(oldest+i)%n;offset=32+slot*frame_size;ns=struct.unpack_from('<Q',b,offset)[0];pixels=b[offset+8:offset+frame_size]
 raw=b''.join(b'\0'+pixels[y*w*3:(y+1)*w*3]for y in range(h));out=a.directory/f'os-frame-{i:03d}.png';out.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''));frames.append(dict(index=i,offset_ns=ns,file=str(out.resolve()),rgb_sha256=hashlib.sha256(pixels).hexdigest()))
report=dict(source=str(a.recording.resolve()),source_sha256=hashlib.sha256(b).hexdigest(),format='RWREC003',scope='Native OS surfaces captured on the physical phone immediately before framebuffer presentation; not a photograph of the panel',width=w,height=h,frames=n,began_kernel_ns=began,decoded=frames)
(a.directory/'recording.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
