// UI decoration only. Font Awesome Free 5.15.4 SVGs are bundled locally.
// SVG license: CC BY 4.0. Attribution and original files: icons/LICENSE.txt.
const NS='http://www.w3.org/2000/svg';
function icon(name){const svg=document.createElementNS(NS,'svg'),use=document.createElementNS(NS,'use');svg.classList.add('ui-icon');svg.setAttribute('aria-hidden','true');svg.setAttribute('focusable','false');use.setAttribute('href','./icons/sprite.svg#'+name);svg.append(use);return svg;}
const buttons={demo:'music',stop:'stop','save-profile':'save','export-profile':'file-export',detect:'search',inspect:'wave-square','hear-band':'headphones','hear-band-loop':'headphones',apply:'check',discard:'undo',previous:'chevron-left',next:'chevron-right',audition:'volume-up',play:'play',bounds:'cut','set-length':'cut','apply-sound':'sliders-h','copy-settings':'copy','paste-settings':'paste','reset-settings':'undo','export-markers':'file-export','export-project':'save'};
for(const [id,name]of Object.entries(buttons)){
  const button=document.getElementById(id);if(!button)continue;
  if(id==='previous'||id==='next')button.textContent=button.textContent.replace(/[←→]/g,'').trim();
  button.classList.add('icon-button');button.prepend(icon(name));
}
for(const [id,name]of Object.entries({'load-title':'folder-open','profile-title':'sliders-h','analysis-title':'wave-square','review-title':'cut'}))document.getElementById(id)?.prepend(icon(name));
for(const [id,name]of Object.entries({wav:'folder-open','import-profile':'file-import','import-project':'file-import'})){
  const input=document.getElementById(id),label=input?.closest('label'),text=label?.firstChild;
  if(text?.nodeType===Node.TEXT_NODE){const title=document.createElement('span');title.className='field-title';title.append(icon(name),document.createTextNode(text.textContent.trim()));label.replaceChild(title,text);}
}
