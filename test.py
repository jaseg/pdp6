#!/usr/bin/env python3

import ctypes
from pprint import pprint
import unittest
from itertools import product, chain

import pydp

FW   = 0o777777777777
RT   = 0o000000777777
LT   = 0o777777000000
SGN  = 0o400000000000
RSGN = 0o000000400000

FCRY = 0o1000000000000
F0   = 0o400000000000
F1   = 0o200000000000
F2   = 0o100000000000
F3   = 0o040000000000
F4   = 0o020000000000
F5   = 0o010000000000
F6   = 0o004000000000
F7   = 0o002000000000
F8   = 0o001000000000
F9   = 0o000400000000
F10  = 0o000200000000
F11  = 0o000100000000
F12  = 0o000040000000
F13  = 0o000020000000
F14  = 0o000010000000
F15  = 0o000004000000
F16  = 0o000002000000
F17  = 0o000001000000
F18  = 0o000000400000
F19  = 0o000000200000
F20  = 0o000000100000
F21  = 0o000000040000
F22  = 0o000000020000
F23  = 0o000000010000
F24  = 0o000000004000
F25  = 0o000000002000
F26  = 0o000000001000
F27  = 0o000000000400
F28  = 0o000000000200
F29  = 0o000000000100
F30  = 0o000000000040
F31  = 0o000000000020
F32  = 0o000000000010
F33  = 0o000000000004
F34  = 0o000000000002
F35  = 0o000000000001

H0  = 0o400000
H1  = 0o200000
H2  = 0o100000
H3  = 0o040000
H4  = 0o020000
H5  = 0o010000
H6  = 0o004000
H7  = 0o002000
H8  = 0o001000
H9  = 0o000400
H10 = 0o000200
H11 = 0o000100
H12 = 0o000040
H13 = 0o000020
H14 = 0o000010
H15 = 0o000004
H16 = 0o000002
H17 = 0o000001


whex = lambda x: '{:09x}'.format(x)

class EmuTest(unittest.TestCase):
    def setUp(self):
        self.e = pydp.Emu()
        self.a = self.e._apr
        self.pulses = []

    def tearDown(self):
        del self.e

    def pulseCycle(self):
        self.pulses.append(self.e.nextpulses)
        self.e.apr_cycle()

    def pulseRun(self, pulse, until_any=[], until_all=[], steps=None, ignore=[]):
        #print('===========', pulse, '===========')
        until_any, until_all, ignore = set(until_any), set(until_all), set(ignore)

        self.e.clear_pulses()
        self.pulses = []

        self.e.pulse(pulse)
        self.pulses.append([pulse])
        print(' ', self.a.nnextpulses, self.a.ncurpulses, self.e.nextpulses)

        if steps is not None:
            np = set(self.e.nextpulses)
            i = 1
            while np:
                self.pulseCycle()
                print(' ', self.a.nnextpulses, self.a.ncurpulses, self.e.nextpulses)

                for b in ignore:
                    self.e.dequeue_pulse(b)
                _np = self.e.nextpulses
                np = set(_np)
# FIXME re-enable this
#                if len(np) != len(_np):
#                    raise ValueError('Duplicate pulse in nextpulses:', _np)

                if np & until_any:
                    break

                if until_all:
                    until_all -= np
                    if not until_all:
                        break
                
                i += 1
                if i == steps:
                    raise RuntimeError('Step limit exceeded')
        print('{}/{} steps.'.format(i, steps))
        print()
    
    def countPulse(self, name):
        return sum( (1 if p == name else 0) for ps in pulses for p in ps )

    def assertFlag(self, name, value):
        self.assertEqual(getattr(self.a, name), value, 'APR flag {} is incorrect'.format(name))
    def assertReg(self, name, value):
        self.assertEqual(getattr(self.a, name), value, 'APR register {} is incorrect'.format(name))
    
    def assertPulse(self, name):
        self.assertEqual(self.countPulse(name), 1, 'Pulse {} did not run'.format(name))

    def assertNotPulse(self, name):
        self.assertEqual(self.countPulse(name), 0, 'Pulse {} did not run'.format(name))

_2xxsh = (0b010100<<12)
regerr = lambda reg, xp, act:\
        '{} not correct, expected {:09x} actual {:09x}'.format(
                reg, xp, act)
regerro = lambda reg, xp, act:\
        '{} not correct, expected {:012o} actual {:012o}'.format(
                 reg, xp, act)
class ShiftTests(EmuTest):
    OPS =  {  'ASHR': lambda a, m: (a[0]  + a[0:35]        , m                            ),
              'ASHL': lambda a, m: (a[0]  + a[2:36] + '0'  , m                            ),
             'ASHCR': lambda a, m: (a[0]  + a[0:35]        , a[0]+ a[35] + m[1:35]        ),
             'ASHCL': lambda a, m: (a[0]  + a[2:36] + m[1] , a[0]        + m[2:36] + '0'  ),
              'ROTR': lambda a, m: (a[35] + a[0:35]        ,               m              ), 
              'ROTL': lambda a, m: (        a[1:36] + a[0] ,               m              ),
             'ROTCR': lambda a, m: (m[35] + a[0:35]        , a[35]       + m[0:35]        ),
             'ROTCL': lambda a, m: (        a[1:36] + m[0] ,               m[1:36] + a[0] ),
              'LSHR': lambda a, m: ('0'   + a[0:35]        ,               m              ),
              'LSHL': lambda a, m: (        a[1:36] + '0'  ,               m              ),
             'LSHCR': lambda a, m: ('0'   + a[0:35]        , a[35]       + m[0:35]        ),
             'LSHCL': lambda a, m: (        a[1:36] + m[0] ,               m[1:36] + '0'  ) }

    OPCODES = { 'ASH':  pydp.Opcode.ASH,  'ROT':  pydp.Opcode.ROT,  'LSH':  pydp.Opcode.LSH,
                'ASHC': pydp.Opcode.ASHC, 'ROTC': pydp.Opcode.ROTC, 'LSHC': pydp.Opcode.LSHC }

    FUNNY_VALS_A = [ 0o777777777777, 0o000000000000, 0o111111111111, 0o222222222222, 0o444444444444, 0o333333333333,
                     0o666666666666, 0o555555555555, 0o666666666666,
                     0x111111111,    0x222222222,    0x444444444,    0x888888888,
                     0o400000000000, 0o200000000000, 0o300000000000, 0o000000000001, 0o000000000002, 0o000000000003,
                     0o172631651263, 0o010124010234, 0o010100100110, 0o020310200012, 0o776567756471, 0o123456701234 ] # cue me mashing my keyboard ^jaseg

    FUNNY_VALS_B = [ 0o777777777777, 0o000000000000,
                     0x111111111,    0o400000000000, 0o200000000000, 0o000000000001,
                     0o761226345521, 0o012040124612 ]
                   

    def _sh_sim(self, ins, ar, mq, n=1):
        a, m = '{:036b}'.format(ar), '{:036b}'.format(mq)
        for i in range(n):
            a, m = ShiftTests.OPS[ins](a, m)
        return int(a, 2), int(m, 2)
    
    def test_shift_single(self):
        for ins, a in product(('ASH', 'ROT', 'LSH'),
                        chain(ShiftTests.FUNNY_VALS_A, ShiftTests.FUNNY_VALS_B)):
            baseop = _2xxsh | (ShiftTests.OPCODES[ins].value<<9);
            for n in range(1, 37):
                with self.subTest(n=n, ins=ins, direction='LEFT', ar=whex(a), mq=whex(0)):
                    self.e.pulse('mr_clr')
                    self.a.ir = baseop
                    self.a.mb = n
                    self.a.ar, self.a.mq = a, 0
                    self.e.decode_ir()
                    self.assertTrue(self.a.shift_op, 'instruction not decoded correctly')
                    self.assertEqual(self.a.inst, ShiftTests.OPCODES[ins].value, 'instruction not decoded correctly')
                    self.pulseRun('ft6a', until_any=['et10'], steps=128)
                    ar, mq = self._sh_sim(ins + 'L', a, 0, n=n)
                    aar, amq = self.a.ar.value, self.a.mq.value
                    self.assertEqual(aar, ar, regerr('AR', ar, aar))
                    #TODO: May MQ be clobbered here or not?
                    #self.assertEqual(amq, mq, regerr('MQ', mq, amq))

                with self.subTest(n=n, ins=ins, direction='RIGHT', ar=whex(a), mq=whex(0)):
                    self.e.pulse('mr_clr')
                    self.a.ir = baseop
                    self.a.mb = F18 | ((~(n-1))&0xFF) # two's complement
                    self.a.ar, self.a.mq = a, 0
                    self.e.decode_ir()
                    self.assertTrue(self.a.shift_op, 'instruction not decoded correctly')
                    self.assertEqual(self.a.inst, ShiftTests.OPCODES[ins].value, 'instruction not decoded correctly')
                    self.pulseRun('ft6a', until_any=['et10'], steps=128)
                    ar, mq = self._sh_sim(ins + 'R', a, 0, n=n)
                    aar, amq = self.a.ar.value, self.a.mq.value
                    self.assertEqual(aar, ar, regerr('AR', ar, aar))
                    #TODO: May MQ be clobbered here or not?
                    #self.assertEqual(amq, mq, regerr('MQ', mq, amq))

    def test_shift_combined(self):
        for ins, (a, b) in product(('ASHC', 'ROTC', 'LSHC'),
                            chain(
                                product(ShiftTests.FUNNY_VALS_A, ShiftTests.FUNNY_VALS_B),
                                product(ShiftTests.FUNNY_VALS_B, ShiftTests.FUNNY_VALS_A))):
            baseop = _2xxsh | (ShiftTests.OPCODES[ins].value<<9);
            for n in range(1, 73):
                with self.subTest(n=n, ins=ins, direction='LEFT', ar=whex(a), mq=whex(b)):
                    self.e.pulse('mr_clr')
                    self.a.ir = baseop
                    self.a.mb = n
                    self.a.ar, self.a.mq = a, b
                    self.e.decode_ir()
                    self.assertTrue(self.a.shift_op, 'instruction not decoded correctly')
                    self.assertEqual(self.a.inst, ShiftTests.OPCODES[ins].value, 'instruction not decoded correctly')
                    self.pulseRun('ft6a', until_any=['et10'], steps=128)
                    ar, mq = self._sh_sim(ins + 'L', a, b, n=n)
                    aar, amq = self.a.ar.value, self.a.mq.value
                    self.assertEqual(aar, ar, regerr('AR', ar, aar))
                    self.assertEqual(amq, mq, regerr('MQ', mq, amq))

                with self.subTest(n=n, ins=ins, direction='RIGHT', ar=whex(a), mq=whex(b)):
                    self.e.pulse('mr_clr')
                    self.a.ir = baseop
                    self.a.mb = F18 | ((~(n-1))&0xFF) # two's complement
                    self.a.ar, self.a.mq = a, b
                    self.e.decode_ir()
                    self.assertTrue(self.a.shift_op, 'instruction not decoded correctly')
                    self.assertEqual(self.a.inst, ShiftTests.OPCODES[ins].value, 'instruction not decoded correctly')
                    self.pulseRun('ft6a', until_any=['et10'], steps=128)
                    ar, mq = self._sh_sim(ins + 'R', a, b, n=n)
                    aar, amq = self.a.ar.value, self.a.mq.value
                    self.assertEqual(aar, ar, regerr('AR', ar, aar))
                    self.assertEqual(amq, mq, regerr('MQ', mq, amq))

woct = lambda x: '{:012o}'.format(x)
class HWTTests(EmuTest):
	def _hwt_sim(self, w, xx, y, zz, ar, mb):
		if zz == 2:
			src = ar
		else:
			src = mb
		l, r = (mb>>18)&RT, mb&RT

		if zz == 2 or zz == 3:
			dl, dr = (mb>>18)&RT, mb&RT
		else:
			dl, dr = (ar>>18)&RT, ar&RT

		if y:
			l, r = r, l
			dl, dr = dr, dl

		if w:
			d, nd = l, r
			dd, dnd = dl, dr
		else:
			d, nd = r, l
			dd, dnd = dr, dl

		if xx == 1:
			nd = 0o000000
		elif xx == 2:
			nd = 0o777777
		elif xx == 3:
			nd = 0o777777 if d&0o400000 else 0o000000
		else:
			nd = dnd

		if w:
			dst = (nd<<18) | d
		else:
			dst = (d<<18) | nd

		if zz == 2:
			return ar, dst
		elif zz == 3:
			return dst, dst
		else:
			return dst, mb


	def test_hwt(self):
		for w, xx, y, zz in product([0, 1], [0, 1, 2, 3], [0, 1], [0, 1, 2, 3]):
			for ar, mb in [(0o111111222222, 0o333333444444),
					(0o000000777777, 0o000000000000), (0o000000000000, 0o000000777777),
					(0o777777000000, 0o000000000000), (0o000000000000, 0o777777000000),
					(0o123456712345, 0o671234567123)]:
				with self.subTest(w=w, xx=xx, y=y, zz=zz, ar=woct(ar), mb=woct(mb)):
					self.e.pulse('mr_clr')
					self.a.ir = 0b101<<15 | w<<14 | xx<<12 | y<<11 | zz<<9
					self.a.ar, self.a.mb = ar, mb
					self.e.decode_ir()
					self.assertEqual(self.a.inst>>6, 0b101, 'instruction not decoded correctly')

					self.pulseRun('et0a', until_any=['et10'], steps=32)

					ear, emb = self._hwt_sim(w, xx, y, zz, ar, mb)
					aar, amb = self.a.ar.value, self.a.mb.value
					self.assertEqual(aar, ear, regerro('AR', ear, aar))
					self.assertEqual(amb, emb, regerro('MB', emb, amb))
				break
			break

if __name__ == '__main__':
    unittest.main()

