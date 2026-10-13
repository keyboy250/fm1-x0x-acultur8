'use strict';
let lab, token, current = null, activeView = 'overview', pendingAction = null;
let buildPoll = null;
const $ = s => document.querySelector(s);
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };
function notify(message, error = false) { $('#notice').textContent = message; $('#notice').className = error ? 'error' : ''; }
async function api(path, body) {
    const response = await fetch(path, body === undefined ? {} : { method: 'POST', headers: { 'Content-Type': 'application/json', 'X-Lab-Token': token }, body: JSON.stringify(body) });
    const data = await response.json();
    if (!response.ok) throw new Error(data.error || 'Request failed');
    return data;
}
function unsaved() { return current && $('#code').value !== current.content; }
function guard(action) { if (unsaved()) { pendingAction = action; $('#confirm-dialog').showModal(); } else action(); }
function view(name) {
    if (name !== 'emulator') stopEmulator();
    activeView = name;
    document.querySelectorAll('.view').forEach(v => { v.hidden = v.id !== name; });
    document.querySelectorAll('nav button').forEach(b => b.classList.toggle('active', b.dataset.view === name));
    $('#view-title').textContent = name === 'source' ? 'SOURCE EDITOR' : name.toUpperCase();
    window.scrollTo(0, 0);
    if (name === 'emulator') refreshEmulator();
}
async function loadState() {
    lab = await api('/api/state'); render();
}
function render() {
    $('#stats').replaceChildren(...[['Source files',lab.stats.files],['Sound parts',lab.stats.parts],['Steps per pattern',lab.stats.steps],['Modified files',lab.stats.changed]].map(([name,count]) => { const s = el('div','stat'); s.append(el('strong','',String(count)),el('span','',name)); return s; }));
    $('#modules').replaceChildren(...lab.modules.map((m,i) => { const b = el('button','module '+m.color); b.append(el('span','module-icon',String(i+1).padStart(2,'0')+' / SOURCE'),el('strong','',m.name),el('p','',m.description),el('span','arrow','↗')); b.onclick = () => guard(() => openFile(m.path)); return b; }));
    renderFiles(); renderTargets(); renderChecks(); renderEmulator(lab.emulator);
    $('#readiness').replaceChildren(...lab.readiness.map(r => { const d = el('div','ready-item'); d.append(el('span',r.ready?'tag ready':'tag missing',r.ready?'● DETECTED':'○ SETUP NEEDED'),el('strong','',r.name),el('p','',r.detail)); return d; }));
    $('#source-root').textContent = lab.source_root; $('#archive-sha').textContent = lab.archive_sha;
    $('#history').replaceChildren(...(lab.history.length ? lab.history.map(h => { const d = el('div','activity'); d.append(el('small','',h.at),el('span','',h.event+' · '+(h.path || h.detail || ''))); return d; }) : [el('p','','No edits yet. Saves, restores, checks and exports will appear here.')]));
}
function renderFiles() {
    if (!lab) return;
    const term = $('#file-search').value.toLowerCase();
    const list = lab.files.filter(f => f.editable && f.path.toLowerCase().includes(term) && (!$('#changed-only').checked || f.changed));
    $('#file-list').replaceChildren(...list.map(f => { const b = el('button','file-row'+(f.changed?' changed':'')+(current?.path === f.path?' active':''),f.path); b.onclick = () => guard(() => openFile(f.path)); return b; }));
    if (!list.length) $('#file-list').append(el('p','muted','No matching source files.'));
}
function fileUI() {
    $('#file-name').textContent = current.path;
    $('#code').disabled = false; $('#code').value = current.content;
    $('#save').disabled = false; $('#show-diff').disabled = false;
    $('#diff').textContent = current.diff || 'No changes from the archive baseline.';
    $('#diff-state').textContent = '(saved file)';
    $('#backups').replaceChildren(new Option('Previous checkpoints',''),...current.backups.map(b => new Option(b.id,b.id)));
    $('#restore').disabled = true;
    updateDirty(); renderFiles();
}
async function openFile(path) {
    try { current = await api('/api/file?path='+encodeURIComponent(path)); fileUI(); $('#diff-panel').hidden = true; view('source'); }
    catch(e) { notify(e.message,true); }
}
function updateDirty() {
    if (!current) return;
    $('#dirty').textContent = unsaved() ? 'UNSAVED' : current.changed ? 'MODIFIED' : 'BASELINE';
    $('#line-count').textContent = $('#code').value.split('\n').length+' lines · UTF-8 · working copy';
    if (unsaved()) $('#diff-state').textContent = '(saved file; save to include current edits)';
}
async function save() {
    if (!current) return false;
    const selection = [$('#code').selectionStart,$('#code').selectionEnd,$('#code').scrollTop];
    try { current = await api('/api/save',{ path:current.path,content:$('#code').value,sha:current.sha }); fileUI(); $('#code').setSelectionRange(selection[0],selection[1]); $('#code').scrollTop = selection[2]; await loadState(); notify('Saved. The previous version is available under checkpoints.'); return true; }
    catch(e) { notify(e.message,true); return false; }
}
function renderTargets() {
    $('#target-list').replaceChildren(...lab.targets.map(t => {
        const row = el('article','target'); const detail = el('div'); detail.append(el('span','area',t.area),el('h3','',t.title),el('p','',t.acceptance));
        if (t.milestone) detail.append(el('p','target-milestone',(t.priority ? 'Priority '+t.priority+' · ' : '')+t.milestone));
        if (t.depends_on?.length) detail.append(el('p','target-dependencies','Depends on: '+t.depends_on.join(', ')));
        if (t.next_check) detail.append(el('p','target-next','Next check: '+t.next_check));
        const controls = el('div','target-controls'); const status = el('select'); status.setAttribute('aria-label',t.id+' status');
        ['todo','doing','review','blocked','done'].forEach(s => status.add(new Option(s,s,false,s===t.status)));
        const notes = el('textarea'); notes.value=t.notes; notes.placeholder='Acceptance evidence / session notes'; notes.setAttribute('aria-label',t.id+' acceptance notes');
        const button = el('button','','Save target'); button.onclick = async () => { button.disabled=true; try { await api('/api/targets',{id:t.id,status:status.value,notes:notes.value}); t.status=status.value; t.notes=notes.value; notify(t.id+' saved.'); } catch(e) { notify(e.message,true); } finally { button.disabled=false; } };
        controls.append(status,notes,button); row.append(el('div','target-index',t.id),detail,controls); return row;
    }));
}
function renderChecks() {
    $('#check-time').textContent = lab.checks?.at || 'Not run yet';
    $('#check-results').replaceChildren(...(lab.checks ? lab.checks.results.map(r => { const row = el('div','result'), text=el('div'); text.append(el('strong','',r.name),el('p','',r.detail)); row.append(el('span','tag '+(r.status==='pass'?'ready':'missing'),r.status.toUpperCase()),text); return row; }) : [el('p','','Run offline checks to parse Python source and record archive compatibility findings. Firmware and simulator tests are separate gates.')]));
}
document.querySelectorAll('nav button').forEach(b => b.onclick = () => guard(() => view(b.dataset.view)));
document.querySelectorAll('[data-goto]').forEach(b => b.onclick = () => guard(() => view(b.dataset.goto)));
document.querySelectorAll('[data-file]').forEach(b => b.onclick = () => guard(() => openFile(b.dataset.file)));
$('#start-edit').onclick = () => guard(() => openFile('firmware/src/app/ui.c'));
$('.brand').onclick = e => { e.preventDefault(); guard(() => view('overview')); };
$('#file-search').oninput = renderFiles; $('#changed-only').onchange = renderFiles;
$('#code').oninput = updateDirty;
$('#code').onkeydown = e => { if (e.key === 'Tab') { e.preventDefault(); const a=e.target.selectionStart,b=e.target.selectionEnd; e.target.setRangeText('    ',a,b,'end'); updateDirty(); } };
$('#save').onclick = save;
$('#show-diff').onclick = () => { $('#diff-panel').hidden = !$('#diff-panel').hidden; };
$('#backups').onchange = () => { $('#restore').disabled = !$('#backups').value; };
$('#restore').onclick = () => guard(async () => { const backup=$('#backups').value; if (!backup) return; try { current = await api('/api/restore',{path:current.path,sha:current.sha,backup}); fileUI(); await loadState(); notify('Checkpoint restored. The version you replaced was also backed up.'); } catch(e) { notify(e.message,true); } });
$('#run-checks').onclick = async () => { const b=$('#run-checks'); b.disabled=true; b.textContent='Checking…'; try { await api('/api/checks',{}); await loadState(); notify('Offline checks recorded. See the remaining build gates below.'); } catch(e) { notify(e.message,true); } finally { b.disabled=false; b.textContent='Run offline checks ↗'; } };
$('#export').onclick = () => guard(async () => { try { const result=await api('/api/export',{}); const a=el('a'); a.href=result.url; a.download=result.name; document.body.append(a); a.click(); a.remove(); notify('Complete source exported with licences and change inventory.'); } catch(e) { notify(e.message,true); } });
function stopEmulator() { const frame=$('#emu-frame'); if (!frame.hidden) { frame.src='about:blank'; frame.hidden=true; } }
function renderEmulator(e) {
    if (!e) return;
    $('#emu-status').textContent=e.running?'Compiling working firmware…':e.error?e.error:!e.ready?'No compiled emulator yet':e.stale?'Source changed — rebuild before testing':'Ready — build matches the working source';
    $('#emu-detail').textContent=e.manifest?`Built ${e.manifest.built_at} · ${Math.round(e.manifest.wasm_bytes/1024)} KiB WebAssembly · ${e.manifest.compiler}`:'The first build needs the prepared Docker compiler image.';
    $('#emu-launch').disabled=!e.ready||e.stale||e.running||Boolean(e.error);
    $('#emu-rebuild').disabled=e.running;
    $('#emu-build-log').textContent=e.log||'No dashboard rebuild has run yet.';
    if(e.running && !buildPoll) buildPoll=setInterval(refreshEmulator,1800);
    if(!e.running && buildPoll) { clearInterval(buildPoll); buildPoll=null; }
}
let emuRefreshing=false;
let rexResult=null;
function renderRexResult(job){
    rexResult=job;$('#rex-result').hidden=false;
    $('#rex-info').textContent=`${job.name} · ${job.sliceCount} slices · ${job.bpm} BPM · ${job.loopDuration.toFixed(3)} s loop`;
    $('#rex-downloads').replaceChildren(...[['WAV slice bank',job.audioUrl],['Exact marker JSON',job.markersUrl],['Marker CSV',job.csvUrl],['SDK license',job.licenseUrl],['Attribution',job.attributionUrl]].map(([label,url])=>{const a=el('a','small',label+' ↗');a.href=url;a.download=url.split('/').pop();return a;}));
}
async function pollRex(id){
    try{const job=await api('/api/rex/status?id='+encodeURIComponent(id));
      if(job.status==='running'){setTimeout(()=>pollRex(id),1000);return;}
      $('#rex-convert').disabled=false;
      if(job.status==='failed')throw Error(job.error);
      $('#rex-status').textContent='Converted with original REX onset positions. Ready to audition.';renderRexResult(job);
    }catch(e){$('#rex-convert').disabled=false;$('#rex-status').textContent=e.message;notify(e.message,true);}
}
$('#rex-convert').onclick=async()=>{
    try{const file=$('#rex-file').files[0];if(!file)throw Error('Choose a REX loop first.');if(file.size>16*1024*1024)throw Error('Maximum input size is 16 MiB.');
      $('#rex-convert').disabled=true;$('#rex-result').hidden=true;$('#rex-status').textContent='Reading and converting the original slices…';
      const bytes=new Uint8Array(await file.arrayBuffer());let raw='';for(let i=0;i<bytes.length;i+=32768)raw+=String.fromCharCode(...bytes.subarray(i,i+32768));
      const job=await api('/api/rex/convert',{name:file.name,data:btoa(raw)});pollRex(job.id);
    }catch(e){$('#rex-convert').disabled=false;$('#rex-status').textContent=e.message;}
};
$('#rex-open').onclick=()=>guard(async()=>{view('emulator');await refreshEmulator();if(!lab.emulator.ready||lab.emulator.stale||lab.emulator.running){notify('Rebuild the emulator first, then return to the converted loop.',true);return;}$('#emu-frame').src='/emu/?rex='+encodeURIComponent(rexResult.id);$('#emu-frame').hidden=false;});
async function refreshEmulator() { if(emuRefreshing) return; emuRefreshing=true; try { const e=await api('/api/emulator'); lab.emulator=e; renderEmulator(e); } catch(e) { notify(e.message,true); } finally { emuRefreshing=false; } }
$('#emu-launch').onclick=async()=>{ try { const e=await api('/api/emulator'); renderEmulator(e); if(!e.ready||e.stale||e.running||e.error) return; $('#emu-frame').src='/emu/?build='+encodeURIComponent(e.manifest.wasm_sha256); $('#emu-frame').hidden=false; } catch(e) { notify(e.message,true); } };
$('#emu-stop').onclick=stopEmulator;
$('#emu-rebuild').onclick=()=>guard(async()=>{ stopEmulator(); try { await api('/api/emulator/build',{}); await refreshEmulator(); } catch(e) { notify(e.message,true); } });
$('#stay').onclick = () => { pendingAction=null; $('#confirm-dialog').close(); };
$('#discard').onclick = () => { if(current) fileUI(); const a=pendingAction; pendingAction=null; $('#confirm-dialog').close(); a?.(); };
$('#save-leave').onclick = async () => { if(await save()) { const a=pendingAction; pendingAction=null; $('#confirm-dialog').close(); a?.(); } };
$('#confirm-dialog').addEventListener('cancel',() => { pendingAction=null; });
window.addEventListener('beforeunload',e => { if(unsaved()) { e.preventDefault(); e.returnValue=''; } });
document.addEventListener('keydown',e => { if((e.ctrlKey||e.metaKey) && e.key.toLowerCase()==='s' && activeView==='source') { e.preventDefault(); save(); } });
(async () => { try { token=(await api('/api/session')).token; await loadState(); $('#rex-status').textContent=lab.rex_ready?'Official Windows decoder is available. Choose your REX loop.':'Windows REX SDK is missing. See the illustrated FAQ.';const initialView=location.hash.slice(1);if(document.getElementById(initialView)?.classList.contains('view'))view(initialView); } catch(e) { notify('Cannot load the local workspace: '+e.message,true); } })();
