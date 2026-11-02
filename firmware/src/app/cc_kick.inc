/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit source-owned kick preset transactions use existing72-byte source records. */
static int kp_apply(cc_source_t *s,int field,uint32_t value){
 cc_generation_t *g=&s->generation;
 if(cg_generation==UINT32_MAX)return cg_refuse(s,CG_STALE);
 if(g->mode!=7){cf_cancel_pending(s);memset(g,0,sizeof *g);g->mode=7;s->armed=0;}
 if(field>=4){unsigned shift=(unsigned)(field-4)*7;uint16_t request=(uint16_t)((g->request&~(127u<<shift))|(value<<shift));if(request!=g->request){s->armed=0;g->flags&=(uint8_t)~CG_ARM;}g->request=request;g->flags|=(uint8_t)(field==4?CG_TOKEN_LO:CG_TOKEN_HI);return CC_APPLIED;}
 int load=field>=2,execute=field&1;
 if(!value){if(execute)g->flags&=(uint8_t)~CG_HIGH;else {g->flags&=(uint8_t)~CG_ARM;s->armed=0;g->state=CG_CANCELLED;g->reason=CG_CANCEL;}return CC_APPLIED;}
 if(!s->ready || !(s->destination_ready&2u) || s->destination_applied>=16 || s->applied.part!=1 || s->applied.kit!=1 || s->applied.drum!=0 || (g->flags&(CG_TOKEN_LO|CG_TOKEN_HI))!=(CG_TOKEN_LO|CG_TOKEN_HI))return cg_refuse(s,CG_CONTEXT);
 unsigned signature=s->destination_applied|((unsigned)load<<8);
 if(!execute){
  if((g->flags&CG_LAST)&&g->request<=g->last_request){if(g->request==g->last_request&&g->seed==signature&&(g->state==CG_APPLIED||g->state==CG_NO_CHANGE))return CC_APPLIED;return cg_refuse(s,CG_STALE);}
  if(!ui_kick_edit_ready()||ui_cc_action_ready(ACT_SAVE))return cg_refuse(s,CG_OWNER);
  s->guard.revision=variant_current_revision();s->guard.transport_epoch=seq.transport_epoch;s->guard.fingerprint=project_fingerprint();s->armed=(uint8_t)(240+load);g->seed=signature;g->flags|=CG_ARM;g->state=CG_ACCEPTED;g->reason=CG_OK;g->result_request=g->request;return CC_APPLIED;
 }
 if((g->flags&CG_LAST)&&!(g->flags&CG_ARM)&&g->request==g->last_request&&g->seed==signature&&(g->state==CG_APPLIED||g->state==CG_NO_CHANGE))return CC_APPLIED;
 if(g->flags&CG_HIGH)return CC_APPLIED;
 if(!(g->flags&CG_ARM)||s->armed!=240+load||g->seed!=signature)return cg_refuse(s,CG_CONTEXT);
 if(s->guard.revision!=variant_current_revision()||s->guard.transport_epoch!=seq.transport_epoch||s->guard.fingerprint!=project_fingerprint()){s->armed=0;g->flags&=(uint8_t)~CG_ARM;return cg_refuse(s,CG_STALE);}
 kick_preset_t before,after;int prior=load?VR_MISSING:project_kick_preset_get(s->destination_applied,&before);
 int rc=ui_kick_preset_action(s->destination_applied,load);int same=rc==VR_OK&&(load?project_fingerprint()==s->guard.fingerprint:prior==VR_OK&&project_kick_preset_get(s->destination_applied,&after)==VR_OK&&!memcmp(&before,&after,sizeof before));g->flags=(uint8_t)((g->flags&~CG_ARM)|CG_LAST|CG_HIGH);s->armed=0;g->last_request=g->result_request=g->request;
 g->state=rc==VR_OK?(same?CG_NO_CHANGE:CG_APPLIED):CG_REJECTED;g->reason=rc==VR_OK?CG_OK:rc==VR_MISSING?CG_MISSING:rc==VR_IO?CG_VALUE:CG_OWNER;if(cg_generation!=UINT32_MAX)cg_generation++;
 return rc==VR_OK?CC_APPLIED:CC_REJECTED;
}
