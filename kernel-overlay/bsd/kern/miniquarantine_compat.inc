/* 22F66 libquarantine process-metadata ABI, only for the explicit RAM macOS
 * profile. Like the other minimac policies, this is NOT security enforcement.
 * It stores bounded self-process metadata, not filesystem quarantine rules.
 * Unknown operations, other PIDs and exclusion patterns fail explicitly.
 */
#define MINIQ_SLOTS 64
struct miniq_data {
 uint64_t uniqueid;
 uint32_t flags, metadata_len, tracking_len;
 char identifier[257], metadata[65];
 uint8_t tracking[64];
};
static struct miniq_data miniq_table[MINIQ_SLOTS];
LCK_GRP_DECLARE(miniq_group,"miniquarantine");
LCK_MTX_DECLARE(miniq_lock,&miniq_group);
static void miniq_exit(struct proc*p){
 uint64_t id=proc_uniqueid(p);lck_mtx_lock(&miniq_lock);
 for(unsigned i=0;i<MINIQ_SLOTS;i++)if(miniq_table[i].uniqueid==id)bzero(&miniq_table[i],sizeof(miniq_table[i]));
 lck_mtx_unlock(&miniq_lock);
}
static int miniq_syscall(struct proc*p,int call,user_addr_t arg){
 uint64_t a[9]={0},id=proc_uniqueid(p);struct miniq_data data;int error;
 bzero(&data,sizeof(data));
 if(call!=84&&call!=85&&call!=87)return EOPNOTSUPP;
 error=copyin(arg,a,(call==87?9:8)*sizeof(uint64_t));if(error)return error;
 if(call==87){
  /* {pid, identifier, metadata length, metadata, tracking, tracking length,
   *  path exclusion, path exclusion length, flags}. */
  if(a[0]!=0&&a[0]!=(uint64_t)proc_getpid(p))return EPERM;
  if(a[2]>64||a[5]>64||a[8]>0x3ff)return EINVAL;
  if(a[6]||a[7])return EOPNOTSUPP;
  size_t copied=0;error=copyinstr(a[1],data.identifier,sizeof(data.identifier),&copied);if(error)return error;
  if(a[2]&&(error=copyin(a[3],data.metadata,(size_t)a[2])))return error;
  if(a[5]&&(error=copyin(a[4],data.tracking,(size_t)a[5])))return error;
  data.uniqueid=id;data.flags=(uint32_t)a[8];data.metadata_len=(uint32_t)a[2];data.tracking_len=(uint32_t)a[5];
  int slot=-1;lck_mtx_lock(&miniq_lock);
  for(unsigned i=0;i<MINIQ_SLOTS;i++)if(miniq_table[i].uniqueid==id){slot=(int)i;break;}
  if(slot<0)for(unsigned i=0;i<MINIQ_SLOTS;i++)if(!miniq_table[i].uniqueid){slot=(int)i;break;}
  if(slot>=0)miniq_table[slot]=data;
  lck_mtx_unlock(&miniq_lock);return slot>=0?0:ENOSPC;
 }
 /* Get-self / get-PID outputs {id length, id pointer, metadata length,
  * metadata pointer, tracking length, tracking pointer, flags, pid}. */
 if(call==85&&a[7]!=0&&a[7]!=(uint64_t)proc_getpid(p))return EPERM;
 lck_mtx_lock(&miniq_lock);
 for(unsigned i=0;i<MINIQ_SLOTS;i++)if(miniq_table[i].uniqueid==id){data=miniq_table[i];break;}
 lck_mtx_unlock(&miniq_lock);if(!data.uniqueid)return ENOATTR;
 a[0]=strlen(data.identifier)+1;a[2]=data.metadata_len;a[4]=data.tracking_len;a[6]=data.flags;
 error=copyout(data.identifier,a[1],(size_t)a[0]);if(error)return error;
 if(a[2]&&(error=copyout(data.metadata,a[3],(size_t)a[2])))return error;
 if(a[4]&&(error=copyout(data.tracking,a[5],(size_t)a[4])))return error;
 return copyout(a,arg,8*sizeof(uint64_t));
}
#ifndef MINIQ_TEST
static const struct mac_policy_ops miniq_ops={.mpo_policy_syscall=miniq_syscall,.mpo_proc_notify_exit=miniq_exit};
SECURITY_READ_ONLY_LATE(static struct mac_policy_conf) miniq_conf={.mpc_name="Quarantine",.mpc_fullname="RAM macOS process-metadata compatibility (not enforcement)",.mpc_ops=&miniq_ops};
SECURITY_READ_ONLY_LATE(static mac_policy_handle_t) miniq_handle;
#endif
