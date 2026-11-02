"""Pure command51/page21 decoder. No MIDI/device access on import."""
import zlib
from fm1_slow_block_read import HEADER, DiagnosticReadError, number
FIELDS=('schema_magic','publication_sequence','boot_session','half_number',
        'patterns_and_playing','transport_epoch','profiler_clock_hz','frame_delta',
        '909','808_or_core0_wait','303A','303B','break','FX','master','sequencer')
def request(source,token):
 if type(source)is not int or type(token)is not int or not 0<=source<3 or not 0<=token<128:raise ValueError('Invalid source/token')
 return [*HEADER,1,token,source,21,0]
def decode(data,source,token):
 request(source,token);d=tuple(data)
 if d[:4]!=HEADER:return None
 if len(d)<15 or any(type(v)is not int or not 0<=v<128 for v in d):raise DiagnosticReadError('Malformed frame')
 if d[4]!=1:raise DiagnosticReadError('Unsupported ABI')
 if d[5:9]!=(token,source,21,0):return None
 crc=number(d[-5:])
 if crc>0xffffffff or zlib.crc32(bytes(d[:-5]))!=crc:raise DiagnosticReadError('CRC mismatch')
 if d[9]:
  if len(d)!=15 or d[9] not in (1,2,3):raise DiagnosticReadError('Malformed refusal')
  raise DiagnosticReadError({1:'Invalid request',2:'Busy snapshot; no automatic retry',3:'Unsupported/unavailable diagnostic'}[d[9]])
 if len(d)!=96 or d[10]!=16:raise DiagnosticReadError('Unexpected field count')
 values=[number(d[11+5*i:16+5*i]) for i in range(16)]
 if any(v>0xffffffff for v in values):raise DiagnosticReadError('Oversize integer')
 r=dict(zip(FIELDS,values))
 if r['schema_magic']!=0x53545001 or r['publication_sequence']&1:raise DiagnosticReadError('Unsupported schema/incomplete publication')
 r.update(read_only=True,command=51,page=21)
 r['scope']='Existing core0 profiler deltas; partial coverage, nested time may overlap. 808 is core0 wait under offload. A zero clock rate forbids conversion to seconds.'
 return r
def pair(base,stages):
 if not base.get('valid'):raise DiagnosticReadError('No retained event')
 for key in ('publication_sequence','boot_session','half_number','patterns_and_playing','transport_epoch'):
  if base[key]!=stages[key]:raise DiagnosticReadError('Unmatched base/stage '+key)
 if stages['frame_delta']!=512:raise DiagnosticReadError('Unexpected frame coverage')
 return {'base':base,'stages':stages,'matched':True}
