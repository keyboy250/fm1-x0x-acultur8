"""Pure command51/page23 indexed909 workload decoder;no device I/O or retry."""
import zlib
from fm1_slow_block_read import HEADER,DiagnosticReadError,number
HEADER_FIELDS=('schema_magic','publication_sequence','boot_session','half_number','patterns_and_playing','transport_epoch','engine_frames','native_batch_frames','native_batch_calls','coverage_flags','voice_count','row_columns')
ROW_FIELDS=('publication_sequence','boot_session','half_number','voice_index','synthesized_frames','strip_frames','tail_input_frames','zero_handled_frames','potential_filterzero_frames','mixed_frames','packed_calls','entry_flags_or')
VOICES=('BD','SD','LT','MT','HT','RS','CP','CH','OH','CR','RD')
def request(source,token,index=0):
 if any(type(v)is not int for v in (source,token,index)) or not 0<=source<3 or not 0<=token<128 or not 0<=index<=11:raise ValueError('Invalid source/token/index')
 return [*HEADER,1,token,source,23,index]
def decode(data,source,token,index=0):
 request(source,token,index);d=tuple(data)
 if d[:4]!=HEADER:return None
 if len(d)<15 or any(type(v)is not int or not 0<=v<128 for v in d):raise DiagnosticReadError('Malformed frame')
 if d[4]!=1:raise DiagnosticReadError('Unsupported ABI')
 if d[5:9]!=(token,source,23,index):return None
 crc=number(d[-5:])
 if crc>0xffffffff or zlib.crc32(bytes(d[:-5]))!=crc:raise DiagnosticReadError('CRC mismatch')
 if d[9]:
  if len(d)!=15 or d[9] not in (1,2,3):raise DiagnosticReadError('Malformed refusal')
  raise DiagnosticReadError({1:'Invalid request',2:'Busy snapshot;no automatic retry',3:'Unsupported/unavailable workload/index'}[d[9]])
 if len(d)!=76 or d[10]!=12:raise DiagnosticReadError('Unexpected field count')
 words=[number(d[11+5*i:16+5*i]) for i in range(12)]
 if any(w>0xffffffff for w in words):raise DiagnosticReadError('Oversize integer')
 r=dict(zip(HEADER_FIELDS if index==0 else ROW_FIELDS,words))
 if r['publication_sequence']&1:raise DiagnosticReadError('Incomplete publication')
 if index==0:
  if r['schema_magic']!=0x44395701 or r['voice_count']!=11 or r['row_columns']!=8:raise DiagnosticReadError('Unsupported workload schema')
  if r['coverage_flags']&~3 or r['coverage_flags']&2 or r['native_batch_frames']>r['engine_frames']:raise DiagnosticReadError('Invalid workload coverage')
  if r['coverage_flags']&1 and (r['engine_frames']!=512 or r['native_batch_frames']!=512 or not 0<r['native_batch_calls']<=512):raise DiagnosticReadError('Inconsistent complete coverage')
  if r['engine_frames']==0 and (r['native_batch_frames'] or r['native_batch_calls'] or r['coverage_flags']):raise DiagnosticReadError('Inconsistent empty header')
 else:
  if r['voice_index']!=index or r['entry_flags_or']&~0x7fef:raise DiagnosticReadError('Invalid workload row')
  r.update(voice=VOICES[index-1],synthesis_calls=r['packed_calls']&65535,strip_calls=r['packed_calls']>>16)
 r.update(read_only=True,command=51,page=23,index=index)
 return r
def pair(base,stage,header,rows):
 if not base.get('valid'):raise DiagnosticReadError('No retained event')
 for k in ('publication_sequence','boot_session','half_number','patterns_and_playing','transport_epoch'):
  if base[k]!=stage[k] or base[k]!=header[k]:raise DiagnosticReadError('Unmatched workload metadata '+k)
 if stage['frame_delta']!=512 or header['engine_frames']!=512 or header['native_batch_frames']!=512 or header['coverage_flags']!=1:raise DiagnosticReadError('Partial/unavailable native batch workload coverage')
 if len(rows)!=11:raise DiagnosticReadError('Incomplete voice rows')
 for index,r in enumerate(rows,1):
  if r['voice_index']!=index or any(r[k]!=header[k] for k in ('publication_sequence','boot_session','half_number')):raise DiagnosticReadError('Unmatched voice row')
  counts=[r[k] for k in ROW_FIELDS[4:10]]
  if any(v>512 for v in counts) or not r['zero_handled_frames']<=r['potential_filterzero_frames']<=r['tail_input_frames']<=r['strip_frames']<=r['mixed_frames']<=512:raise DiagnosticReadError('Inconsistent voice frame counts')
  if r['synthesized_frames']>r['mixed_frames'] or r['synthesis_calls']>header['native_batch_calls'] or r['strip_calls']>header['native_batch_calls']:raise DiagnosticReadError('Inconsistent voice call counts')
  if r['packed_calls']!=r['synthesis_calls']+(r['strip_calls']<<16):raise DiagnosticReadError('Inconsistent packed calls')
  if bool(r['synthesis_calls'])!=bool(r['synthesized_frames']) or bool(r['strip_calls'])!=bool(r['strip_frames']):raise DiagnosticReadError('Inconsistent voice activity')
 return {'matched':True,'complete':True,'header':header,'voices':rows,
 'scope':'Counts from the exact retained half,not pervoice elapsed time. Entry flags are ORed across its stretches,not one simultaneous state. Native batch coverage required;event timing still comes from paired base/stage/worker records. Diagnostic counts add observer work.'}
