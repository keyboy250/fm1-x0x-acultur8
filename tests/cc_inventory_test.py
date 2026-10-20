# SPDX-License-Identifier: GPL-3.0-only
"""Coverage, source mutation and stable wire-map design checks; no firmware build."""
import copy
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('cc_inventory',ROOT/'tools/cc_inventory.py')
cc=importlib.util.module_from_spec(spec);spec.loader.exec_module(cc)


class InventoryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.data=cc.build(ROOT)

    def clone(self,root):
        for path in cc.FILES+[str(cc.JSON_PATH),str(cc.MD_PATH)]:
            target=root/path;target.parent.mkdir(parents=True,exist_ok=True)
            target.write_text((ROOT/path).read_text(encoding='utf-8'),encoding='utf-8')

    def test_generated_artifacts_current(self):cc.check_artifacts(ROOT,self.data)

    def test_exact_descriptor_counts_and_nonmusical_accounting(self):
        # Independent source-review expectations, not copied from generator output.
        self.assertEqual(self.data['descriptor_counts'],{
            '909':[11,10,9,9,9,7,8,8,8,8,8,2],
            '808':[10,10,9,9,9,9,10,8,8,8,8,2],
            '303':[12,12],'BREAK':[2],'FX':[16],'MIX':[4]*5,'MASTER':[14]})
        byid={x['id']:x for x in self.data['entries']}
        self.assertEqual(byid['arrangement.mode']['range'],[0,9])
        self.assertEqual(byid['arrangement.duration']['range'],[1,32])
        self.assertEqual(byid['pad.channel']['allowed_values'],list(range(16))+[255])
        self.assertIn('allowed_values[round',byid['pad.channel']['scaling'])
        self.assertEqual(byid['generator.3']['semantic_offset'],1)
        self.assertEqual(byid['lane.rate']['range'],[0,7])
        self.assertIn('engine_lane_set',byid['lane.rate']['runtime_parameter'])
        self.assertEqual(byid['lane.mode']['range'],[0,1])
        self.assertEqual(byid['lane.rate']['choices'],['1/16','1/16T','1/32','1/8T','1/4','1/8','1/4T','1/32T'])
        self.assertEqual(byid['lane.length']['parameter_number'],1128)
        self.assertEqual(byid['lane.rate']['parameter_number'],1129)
        self.assertEqual(byid['lane.mode']['parameter_number'],1131)
        self.assertEqual(byid['step.break_slice']['context']['step'],[0,63])
        for prefix in ['generator.','break.generator.','step.','motion.','probability.','performance.',
                       'arrangement.','follow.','pad.','slice.','route.','file.','action.']:
            self.assertTrue(any(k.startswith(prefix) for k in byid),prefix)
        self.assertEqual(sum(k.startswith('strip.') for k in byid),25*10)
        self.assertIsNone(byid['pref.BATT_P']['address'])
        self.assertEqual(byid['pref.SQ_THEME']['category'],'nonmusical')
        self.assertTrue(all(x['runtime_cc']==cc.RUNTIME_NOTE for x in byid.values()))

    def test_collision_reserved_and_fine_pair_rejection(self):
        for kind in ['collision','channel_mode','fine','range','false_claim']:
            data=copy.deepcopy(self.data);a,b=data['entries'][:2]
            if kind=='collision':b['address']=dict(a['address'])
            if kind=='channel_mode':a['address']['cc']=120
            if kind=='fine':a['optional_cc14']['lsb']=34
            if kind=='range':a['range']=[9,3]
            if kind=='false_claim':a['runtime_cc']='implemented'
            with self.assertRaises(ValueError,msg=kind):cc.validate(data)

    def test_descriptor_append_discovered_and_stale_inventory_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);self.clone(root);p=root/'firmware/src/dsp/drum909.c'
            p.write_text(p.read_text(encoding='utf-8').replace('DRIVE(0), DIST, SENDS, PAN,','DRIVE(0), DIST, SENDS, PAN, LIN("New", F_TUNE, 3, D9_R_UNIT),',1),encoding='utf-8')
            changed=cc.build(root);self.assertEqual(changed['descriptor_counts']['909'][0],12)
            with self.assertRaisesRegex(ValueError,'drift'):cc.check_artifacts(root,changed)
            before={r['id']:r['address'] for r in self.data['entries']}
            after={r['id']:r['address'] for r in changed['entries']}
            self.assertTrue(all(after[k]==v for k,v in before.items()))

    def test_macro_default_is_read_from_primary_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);self.clone(root);p=root/'firmware/src/dsp/drum808.c'
            p.write_text(p.read_text(encoding='utf-8').replace('#define P_DRIVE {"Drive", 127, 0, 0}', '#define P_DRIVE {"Drive", 127, 7, 0}'),encoding='utf-8')
            data=cc.build(root)
            row=next(r for r in data['entries'] if r['id']=='sound.808.BD.5')
            self.assertEqual(row['default'],7)
            with self.assertRaisesRegex(ValueError,'drift'):cc.check_artifacts(root,data)

    def test_new_ui_kind_and_unknown_descriptor_syntax_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);self.clone(root);p=root/'firmware/src/app/ui.c'
            p.write_text(p.read_text(encoding='utf-8').replace('R_NONE, R_ENG','R_NONE, R_NEW_EDITABLE, R_ENG',1),encoding='utf-8')
            with self.assertRaisesRegex(ValueError,'coverage'):cc.build(root)
        with self.assertRaises(ValueError):cc.descriptors('static x0x_param_t p[]={UNKNOWN("X",4)};','p',{})

    def test_new_structured_control_fails_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);self.clone(root);p=root/'firmware/src/seq/pattern.h'
            p.write_text(p.read_text(encoding='utf-8').replace('uint8_t density, accent, slide;', 'uint8_t density, accent, slide, new_probability;'),encoding='utf-8')
            with self.assertRaisesRegex(ValueError,'Structured field coverage changed'):cc.build(root)

    def test_sparse_coarse_and_fine_values_always_valid(self):
        rows={x['id']:x for x in self.data['entries']}
        for key in ['pad.channel','arrangement.parent']:
            row=rows[key];allowed=row['allowed_values']
            self.assertEqual(cc.scaled_value(row,0),allowed[0]);self.assertEqual(cc.scaled_value(row,127),allowed[-1])
            coarse={cc.scaled_value(row,v) for v in range(128)}
            fine={cc.scaled_value(row,v,14) for v in range(16384)}
            self.assertEqual(coarse,set(allowed));self.assertEqual(fine,set(allowed))
            if row.get('optional_cc14'):self.assertIn('allowed_values',row['optional_cc14']['scaling'])
            self.assertIn(cc.scaled_value(row,8192,14),allowed)
        self.assertNotEqual(cc.scaled_value(rows['pad.channel'],8192,14),128)

    def test_long_blocks_and_scale_keep_legacy_coarse_values(self):
        rows={x['id']:x for x in self.data['entries']}
        for raw in range(128):
            self.assertEqual(cc.scaled_value(rows['follow.mode'],raw),(raw*2+63)//127)
            self.assertEqual(cc.scaled_value(rows['arrangement.duration'],raw),1+(raw*7+63)//127)
        self.assertEqual(cc.exact_value(rows['follow.mode'],3),3)
        self.assertEqual(cc.exact_value(rows['arrangement.duration'],32),32)

    def test_motion_target_domains_and_all_condition_exact_transactions(self):
        rows={x['id']:x for x in self.data['entries']};motion=rows['motion.value']
        with self.assertRaisesRegex(ValueError,'target required'):cc.value_domain(motion)
        for row in self.data['entries']:
            if not row['id'].startswith('sound.'):continue
            domain=cc.value_domain(motion,row['id'])
            self.assertEqual(domain,list(range(row['range'][1]+1))+[255])
            for bits,mid in [(7,64),(14,8192)]:
                self.assertIn(cc.scaled_value(motion,0,bits,row['id']),domain)
                self.assertIn(cc.scaled_value(motion,mid,bits,row['id']),domain)
                self.assertEqual(cc.scaled_value(motion,(1<<bits)-1,bits,row['id']),255)
            with self.assertRaises(ValueError):cc.exact_value(motion,row['range'][1]+1,row['id'])
        for condition in range(101,136):
            # Encode/decode the actual proposed three-chunk exact CC transaction.
            chunks=[condition&127,(condition>>7)&127,(condition>>14)&3]
            decoded=chunks[0]|chunks[1]<<7|chunks[2]<<14
            self.assertEqual(cc.exact_value(motion,decoded,'T_PROB'),condition)
        self.assertEqual(cc.exact_value(motion,255,'T_PROB'),255)
        for invalid in [136,200,254]:
            with self.assertRaises(ValueError):cc.exact_value(motion,invalid,'T_PROB')
        domain=cc.value_domain(motion,'T_PROB')
        self.assertEqual(domain,list(range(136))+[255])
        self.assertEqual({cc.scaled_value(motion,v,14,'T_PROB') for v in range(16384)},set(domain))


if __name__=='__main__':unittest.main()
