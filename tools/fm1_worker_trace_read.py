"""Pure command51/page22 trace decoder; no hardware access or automatic retry."""
import zlib
from fm1_slow_block_read import HEADER,DiagnosticReadError,number
HEADER_FIELDS=('schema_magic','publication_sequence','boot_session','half_number','patterns_and_playing','transport_epoch','tick_hz','stored_entries','total_entries','overflow_entries','flags','total_frames')
ENTRY_FIELDS=('publication_sequence','boot_session','half_number','entry_index','job','arg','dispatch_tick','worker_start_tick','worker_end_tick','wait_enter_tick','completion_seen_tick','flags')
def request(source,token,index=0):
 if type(source)is not int or type(token)is not int or type(index)is not int or not 0<=source<3 or not 0<=token<128 or not 0<=index<=16:raise ValueError('Invalid source/token/index')
 return [*HEADER,1,token,source,22,index]
def decode(data,source,token,index=0):
 request(source,token,index);d=tuple(data)
 if d[:4]!=HEADER:return None
 if len(d)<15 or any(type(v)is not int or not 0<=v<128 for v in d):raise DiagnosticReadError('Malformed frame')
 if d[4]!=1:raise DiagnosticReadError('Unsupported ABI')
 if d[5:9]!=(token,source,22,index):return None
 crc=number(d[-5:])
 if crc>0xffffffff or zlib.crc32(bytes(d[:-5]))!=crc:raise DiagnosticReadError('CRC mismatch')
 if d[9]:
  if len(d)!=15 or d[9] not in (1,2,3):raise DiagnosticReadError('Malformed refusal')
  raise DiagnosticReadError({1:'Invalid request',2:'Busy snapshot; no automatic retry',3:'Unsupported/unavailable trace/index'}[d[9]])
 if len(d)!=76 or d[10]!=12:raise DiagnosticReadError('Unexpected field count')
 values=[number(d[11+5*i:16+5*i]) for i in range(12)]
 if any(v>0xffffffff for v in values):raise DiagnosticReadError('Oversize integer')
 r=dict(zip(HEADER_FIELDS if index==0 else ENTRY_FIELDS,values))
 if r['publication_sequence']&1:raise DiagnosticReadError('Incomplete publication')
 if r['flags']&~63:raise DiagnosticReadError('Unknown trace flags')
 if index==0:
  if r['schema_magic']!=0x57545201 or r['tick_hz']!=24000000:raise DiagnosticReadError('Unsupported trace schema/clock')
  if r['stored_entries']!=min(r['total_entries'],16) or r['overflow_entries']!=r['total_entries']-r['stored_entries']:raise DiagnosticReadError('Inconsistent trace capacity')
 else:
  if r['entry_index']!=index or r['arg']&~511 or (r['flags']&31) not in (1,2,4,8,16):raise DiagnosticReadError('Invalid trace entry')
 r.update(read_only=True,command=51,page=22,index=index)
 return r
def entry_timing(r):
 if r['flags']&31!=1 or not r['arg']&256:raise DiagnosticReadError('Worker execution unavailable: fallback,failed or mismatched job')
 dispatch=r['dispatch_tick'];offset=lambda k:(r[k]-dispatch)&0xffffffff
 start,end,enter,seen=[offset(k) for k in ('worker_start_tick','worker_end_tick','wait_enter_tick','completion_seen_tick')]
 if not 0<=start<=end<=seen<0x80000000 or not 0<=enter<=seen:raise DiagnosticReadError('Inconsistent timestamp order')
 return {'frames':r['arg']&255 or 256,'dispatch_to_entry_ticks':start,'worker_execution_ticks':end-start,
  'wait_ticks':seen-enter,'worker_exit_to_observation_ticks':seen-end,
  'core0_arrival_after_worker_exit_ticks':max(enter-end,0),
  'observation_after_ready_and_wait_ticks':seen-max(end,enter),
  'worker_finished_before_wait_entry':end<=enter,
  'host_synchronous_model':bool(r['flags']&32),
  'scope':'Worker wall execution includes flash/bus stalls. Exit-to-observation includes intentional core0 work before wait. Observation-after-ready-and-wait includes polling,nested IRQs and probes; it is not pure mailbox propagation latency.'}
def pair(base,header,entries):
 if not base.get('valid'):raise DiagnosticReadError('No retained event')
 for k in ('publication_sequence','boot_session','half_number','patterns_and_playing','transport_epoch'):
  if base[k]!=header[k]:raise DiagnosticReadError('Unmatched base/trace '+k)
 if header['overflow_entries'] or header['flags']&~33 or not header['flags']&1:raise DiagnosticReadError('Incomplete/unavailable worker trace')
 if len(entries)!=header['stored_entries'] or header['total_frames']!=512:raise DiagnosticReadError('Incomplete frame/entry coverage')
 times=[];flags=0;prior_seen=0
 for i,r in enumerate(entries,1):
  if r['entry_index']!=i or any(r[k]!=header[k] for k in ('publication_sequence','boot_session','half_number')):raise DiagnosticReadError('Unmatched trace row')
  if r['flags']&32!=header['flags']&32:raise DiagnosticReadError('Mixed host/target trace')
  if i>1 and r['job']!=(entries[i-2]['job']+1)&0xffffffff:raise DiagnosticReadError('Nonconsecutive worker jobs')
  timing=entry_timing(r)
  offsets={k:(r[k]-base['start_tick'])&0xffffffff for k in ('dispatch_tick','worker_start_tick','worker_end_tick','wait_enter_tick','completion_seen_tick')}
  if any(v>base['render_ticks'] for v in offsets.values()) or offsets['dispatch_tick']<prior_seen:raise DiagnosticReadError('Trace outside retained render/order')
  timing['prior_seen_or_render_start_to_dispatch_ticks']=offsets['dispatch_tick']-prior_seen
  prior_seen=offsets['completion_seen_tick'];times.append(timing);flags|=r['flags']
 if flags!=header['flags'] or sum(t['frames'] for t in times)!=512:raise DiagnosticReadError('Inconsistent trace flags/frame total')
 return {'matched':True,'complete':True,'header':header,'entries':entries,'timing_ticks':times,
  'last_completion_observation_to_render_end_ticks':base['render_ticks']-prior_seen,
  'scope':'Correlated stretches within one retained half; worker times overlap core0 work and must not be added to render elapsed. Host records do not prove target timing.'}
