/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit-project edit primitives: canonical bytes only, no UI selection,
 * transport, audio publication, history or storage. Callers own those gates. */
static POLY_COLD void edit_copy_notes(project_t *candidate,int dst,int src,int part)
{
    pattern_t *d = &candidate->pat[dst];
    const pattern_t *a = &candidate->pat[src];
    if (part < NKIT) {
        if(candidate->variant.arrangement.flags&POLY_FLAG){int v,w;for(v=0;v<NDRUM;v++){for(w=0;w<NSW;w++)d->drum[part].hit[v][w]=a->drum[part].hit[v][w];poly_set(candidate->pat,(uint8_t *)&candidate->sound,dst,part*11+v,poly_length(candidate->pat,(const uint8_t *)&candidate->sound,src,part*11+v),poly_rate(candidate->pat,(const uint8_t *)&candidate->sound,src,part*11+v));}for(w=0;w<NSW;w++)d->drum[part].accent[w]=a->drum[part].accent[w];}
        else d->drum[part] = a->drum[part];
        if (part == PART_909)
            d->swing = a->swing;                      /* the 909 is the bar: its groove goes with it */
    } else if (part < PART_BRK)
        d->bass[part - NKIT] = a->bass[part - NKIT];
    else if(candidate->variant.arrangement.flags&POLY_FLAG){int st;for(st=0;st<BRK_NSET;st++)d->brk.set[st]=a->brk.set[st];d->brk.slot_a=a->brk.slot_a;d->brk.slot_b=a->brk.slot_b;poly_set(candidate->pat,(uint8_t *)&candidate->sound,dst,24,poly_length(candidate->pat,(const uint8_t *)&candidate->sound,src,24),poly_rate(candidate->pat,(const uint8_t *)&candidate->sound,src,24));for(st=0;st<64;st++)poly_break_set(candidate->pat,(uint8_t *)&candidate->sound,dst,st,poly_break_gate(candidate->pat,(const uint8_t *)&candidate->sound,src,st),poly_break_slice(candidate->pat,(const uint8_t *)&candidate->sound,src,st));}
    else d->brk = a->brk;
}
static POLY_COLD int edit_copy_motion(project_t *candidate,int dst,int src,int part)
{
    int k,rc=0;
    for (k = 0; k < NLANE; k++) {                     /* a copy lands on pattern dst: never copied again */
        const lane_t *l = &candidate->arr.lane[k];
        int n, i;
        if (!l->used || l->pat != src || l->part != part)
            continue;
        if ((n = motion_alloc(candidate->arr.lane, dst, part, l->t, l->v, l->i)) < 0) {
            rc = -1;
            break;
        }
        for (i = 0; i < NSTEPS; i++)
            candidate->arr.lane[n].val[i] = l->val[i];
    }
    return rc;
}
static POLY_COLD void edit_clear_part(project_t *candidate,pattern_t *p,int part)
{
    int i;
    if (part < NKIT) {
        int w;
        for (w = 0; w < NSW; w++) {
            for (i = 0; i < NDRUM; i++)
                p->drum[part].hit[i][w] = 0;
            p->drum[part].accent[w] = 0;
        }
    } else if (part < PART_BRK) {
        for (i = 0; i < NSTEPS; i++)
            p->bass[part - NKIT].step[i].flags = G_REST;
    } else {
        if(candidate->variant.arrangement.flags&POLY_FLAG)for(i=0;i<64;i++)poly_break_set(candidate->pat,(uint8_t *)&candidate->sound,(int)(p-candidate->pat),i,0,0);
        else {p->brk.steps = 0;for (i = 0; i < 16; i++)p->brk.slice[i] = 0;}
    }
}
static POLY_COLD void edit_song_shift(song_t *song,uint8_t *degree,int block,int insert)
{
    int i;
    if(insert){
        if(song->len>=NSONG)return;
        for(i=song->len;i>block;i--){song->bar[i]=song->bar[i-1];if(degree)degree[i]=degree[i-1];}
        song->len++;
    }else{
        for(i=block;i+1<song->len;i++){song->bar[i]=song->bar[i+1];if(degree)degree[i]=degree[i+1];}
        song->len--;
    }
}
