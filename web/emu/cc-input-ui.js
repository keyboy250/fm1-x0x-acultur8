// SPDX-License-Identifier: GPL-3.0-only
import {CcInputRouter} from './cc-input-router.mjs';
const $=id=>document.getElementById(id);
let selected='';
const router=new CcInputRouter({send:m=>{if(typeof window.fm1SliceSend!=='function')throw Error('Engine is unavailable.');window.fm1SliceSend(m);},change:render,browserApply:(packet,ctx)=>window.fm1SliceControl?.applyPacket(packet,ctx)||Promise.resolve({status:'rejected',reason:'slice-controller-unavailable'})});
function render(){
  const ports=router.ports(),options=[document.createElement('option')];options[0].value='';options[0].textContent='Choose an input';
  for(const port of ports){const o=document.createElement('option');o.value=port.id;o.textContent=`${port.name||port.id}${port.state==='connected'?'':' (disconnected)'}`;o.disabled=port.state!=='connected';options.push(o);}
  $('cc-input-port').replaceChildren(...options);$('cc-input-port').value=selected;
  const busy=!!router.operation,ready=router.supported&&router.fresh;
  $('cc-input-connect').disabled=router.connecting;
  $('cc-input-port').disabled=!router.access||busy;
  $('cc-input-enable').disabled=busy||!ready||!router.current(router.access?.inputs.get(selected));
  $('cc-input-retry').disabled=busy||!ready||router.active||!router.current(router.access?.inputs.get(router.selected));
  $('cc-input-disable').disabled=!router.input&&!busy;
  $('cc-input-status').textContent=router.status;
  $('cc-input-availability').textContent=!router.fresh?'Awaiting a fresh engine frame.':router.supported?'CC input protocol 1.1 is present. Native coverage remains partial; browser slice controls require slice ABI 1.1.':'CC automation input is unavailable in this engine.';
  $('cc-input-counts').textContent=`Received ${router.received} CC messages · ingress receipts ${router.accepted} · queued ${router.queue.length}/64 · browser applied ${router.browserApplied} / rejected ${router.browserRejected} / no-op ${router.browserNoop} · ignored ${router.ignored} · discarded or unconfirmed ${router.discarded}`;
  const counters=router.stats.length?` Engine counters across all sources: ${['accepted','rejected','unsupported','busy','coalesced','overflow'].map((name,i)=>`${name} ${router.stats[i]}`).join(' · ')}.`:'';
  $('cc-input-engine').textContent=`Engine pending: ${router.pending??'unavailable'}.${counters} Receipts report ingress acceptance; they do not confirm canonical parameter application.`;
}
$('cc-input-connect').addEventListener('click',()=>router.connect());
$('cc-input-port').addEventListener('change',()=>{selected=$('cc-input-port').value;render();});
$('cc-input-enable').addEventListener('click',()=>router.activate(selected));
$('cc-input-retry').addEventListener('click',()=>router.retry());
$('cc-input-disable').addEventListener('click',()=>router.disconnect());
window.addEventListener('fm1-ready',()=>router.engineReset());
window.addEventListener('fm1-marked',event=>{const m=event.detail;if(m?.type==='cc-input-reset')router.engineReset();else if(m?.type==='frame')router.engineState(m.ccInput);else router.message(m);});
window.addEventListener('pagehide',()=>router.pagehide());render();
