#!/usr/bin/env python3
"""Pure request/decoder for proposed command51/page20. No device access on import.
Only the isolated diagnostic candidate implements this page; not installed .38r2.
"""
import zlib
HEADER=(125,70,76,51)
FIELDS=('schema_magic','publication_sequence','boot_session','half_number','patterns_and_playing',
 'render_ticks','wait_sum_ticks','wait_max_ticks','nested_timer_ticks','subrender_count',
 'guard_before','guard_after','dropouts_before','dropouts_after','core_and_validity_flags',
 'start_tick','end_tick','transport_epoch','sequencer_timing_rate','capture_reason_flags')
class DiagnosticReadError(RuntimeError):pass
def request(source,token):
 if type(source)is not int or type(token)is not int or not 0<=source<3 or not 0<=token<128:raise ValueError('Invalid source/token')
 return [*HEADER,1,token,source,20,0]
def number(values):return sum(v<<(7*i) for i,v in enumerate(values))
def decode(data,source,token):
 request(source,token);d=tuple(data)
 if d[:4]!=HEADER:return None
 if len(d)<15 or any(type(v)is not int or not 0<=v<128 for v in d):raise DiagnosticReadError('Malformed frame')
 if d[4]!=1:raise DiagnosticReadError('Unsupported ABI')
 if d[5:9]!=(token,source,20,0):return None
 crc=number(d[-5:])
 if crc>0xffffffff or zlib.crc32(bytes(d[:-5]))!=crc:raise DiagnosticReadError('CRC mismatch')
 if d[9]:
  if len(d)!=15 or d[9] not in (1,2,3):raise DiagnosticReadError('Malformed refusal')
  raise DiagnosticReadError({1:'Invalid request',2:'Busy snapshot; no automatic retry',3:'Unsupported/unavailable diagnostic'}[d[9]])
 if len(d)!=116 or d[10]!=20:raise DiagnosticReadError('Unexpected field count')
 values=[number(d[11+5*i:16+5*i]) for i in range(20)]
 if any(v>0xffffffff for v in values):raise DiagnosticReadError('Oversize integer')
 r=dict(zip(FIELDS,values))
 if r['schema_magic'] not in (0x53423102,0x53423103) or r['publication_sequence']&1:raise DiagnosticReadError('Unsupported schema/incomplete publication')
 if r['capture_reason_flags']&~7:raise DiagnosticReadError('Unknown capture reason')
 r.update(read_only=True,command=51,page=20,valid=bool(r['core_and_validity_flags']&0x80000000))
 if r['valid']:
  if not r['capture_reason_flags'] or ((r['end_tick']-r['start_tick'])&0xffffffff)!=r['render_ticks'] or r['wait_max_ticks']>r['wait_sum_ticks']:raise DiagnosticReadError('Inconsistent capture')
  r['patterns_one_based']=[((r['patterns_and_playing']>>(5*j))&31)+1 for j in range(5)]
 r['capture_policy_revision']=r['schema_magic']&255
 if r['valid']:
  r['playing_at_block_start']=bool(r['patterns_and_playing']&(1<<31))
  r['start_or_continue_pending_at_block_start']=bool(r['patterns_and_playing']&(1<<30))
  if r['capture_policy_revision']==3 and not (r['playing_at_block_start'] or r['start_or_continue_pending_at_block_start']) and not (r['capture_reason_flags']&4):raise DiagnosticReadError('Stopped schema3 record requires a dropout')
 r['retention_policy']='latest_dropout_else_latest_qualifying_since_boot'
 r['scope']=('Schema3: latest dropout in playing,Start-pending or stopped half; otherwise latest playing/Start-pending qualifying half. Transport flags describe block start. Nested times overlap; no retrospective data.' if r['capture_policy_revision']==3 else 'Schema2: playing/Start-pending captures only; stopped dropout halves not retained. Latest dropout precedes later slow/guard-only records. Nested times overlap.')
 return r
