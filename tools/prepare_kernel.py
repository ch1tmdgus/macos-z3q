#!/usr/bin/env python3
"""Reconstruct the kernel source on the pinned Apple XNU base; never writes a phone."""
from pathlib import Path
import argparse,hashlib,json,subprocess,shutil
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--tree',type=Path,required=True)
p.add_argument('--existing',action='store_true',help='Use an existing clean pinned Apple source tree')
p.add_argument('--focaltech-header',type=Path,required=True,help='Matching external generated firmware header; see external-inputs.json')
a=p.parse_args();repo=Path(__file__).resolve().parents[1];tree=a.tree.resolve()
m=json.loads((repo/'manifests/kernel-source.json').read_text());external=json.loads((repo/'manifests/external-inputs.json').read_text())['excluded_firmware_header']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
if sha(a.focaltech_header)!=external['sha256']:raise SystemExit('External firmware header hash mismatch; nothing changed')
if not a.existing:
 if tree.exists():raise SystemExit('Destination exists; refusing to replace it')
 subprocess.run(['git','clone','--depth','1','--branch','xnu-8796.121.2','https://github.com/apple-oss-distributions/xnu.git',str(tree)],check=True)
 head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=tree,text=True).strip()
 if head!=m['base_commit']:raise SystemExit('Unexpected Apple base commit')
for n,h in m['base_sha256'].items():
 if sha(tree/n)!=h:raise SystemExit('Not the expected clean Apple source: '+n)
patch=repo/'patches/xnu-8796.121.2.patch'
subprocess.run(['git','apply','--check',str(patch)],cwd=tree,check=True)
subprocess.run(['git','apply',str(patch)],cwd=tree,check=True)
for p in (repo/'kernel-overlay').rglob('*'):
 if p.is_file():
  dst=tree/p.relative_to(repo/'kernel-overlay');dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dst)
shutil.copyfile(a.focaltech_header,tree/external['path'])
for n,h in m['result_sha256'].items():
 if sha(tree/n)!=h:raise SystemExit('Reconstructed source mismatch: '+n)
print('Verified kernel source prepared:',tree)
