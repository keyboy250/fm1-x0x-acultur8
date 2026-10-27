/* Read-only feedback from the running engine. No device connections. */
window.addEventListener('fm1-marked',e=>{const p=e.detail?.preview;if(!p?.bd||!p?.sd)return;
const rates=["1/16", "1/16T", "1/32", "1/8T", "1/4", "1/8", "1/4T", "1/32T", "1/1", "1/2", "1/64", "1/1D", "1/2D", "1/4D", "1/8D", "1/16D", "1/32D", "1/64D", "1/1T", "1/2T", "1/64T", "2/1", "3/1", "4/1", "6/1", "8/1", "12/1", "16/1", "32/1"];
const modes=['GLOBAL','STRAIGHT','STR+TRIPLET','ALL'];document.querySelector('#preview-state').textContent=`Engine readback Â· Global ${p.bd[13]?'STRAIGHT':'ALL'} Â· P1 909 BD ${rates[p.bd[2]]??p.bd[2]} / ${modes[p.bd[14]??0]} Â· P1 909 SD ${rates[p.sd[2]]??p.sd[2]} / ${modes[p.sd[14]??0]}`;});
