"use strict";
const items=Array.from(document.querySelectorAll('[data-search-item]'));
const q=document.getElementById('q'), family=document.getElementById('family');
const text=items.map(item=>item.textContent.toLocaleLowerCase());
function filter(){const words=q.value.toLocaleLowerCase().trim().split(/\s+/).filter(Boolean);let n=0;items.forEach((item,i)=>{const show=(!family.value||item.dataset.family===family.value)&&words.every(w=>text[i].includes(w));item.hidden=!show;if(show)n++});document.getElementById('count').textContent=n+' of '+items.length+' '+(document.querySelector('.map')?'controls':'topics');document.getElementById('empty').hidden=n!==0;}
q.addEventListener('input',filter);family.addEventListener('change',filter);document.getElementById('clear').addEventListener('click',()=>{q.value='';family.value='';filter();q.focus()});filter();
