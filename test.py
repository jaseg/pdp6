#!/usr/bin/env python3
import pydp
import ctypes
from pprint import pprint
import unittest

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
		until_any, until_all, ignore = set(until_any), set(until_all), set(ignore)

		self.e.pulse(pulse)
		self.pulses.append([pulse])

		if steps is not None:
			np = set(self.e.nextpulses)
			while np:
				self.pulseCycle()

				for b in ignore:
					self.e.dequeue_pulse(b)
				_np = self.e.nextpulses
				np = set(_np)
				if len(np) != len(_np):
					raise ValueError('Duplicate pulse in nextpulses')

				if np & until_any:
					break

				if until_all:
					until_all -= np
					if not until_all:
						break
				
				steps -= 1
				if steps == 0:
					raise RuntimeError('Step limit exceeded')
	
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

def ShiftTests(EmuTest)
	def _ash_sim(self, ins, ar, mq):
		return { 'ASHR':  ((ar>>1) | (ar&F0),
						   mq)
				 'ASHL':  (((ar<<1)&FW), mq)
				 'ASHCR': ((ar>>1) | (ar&F0),
						   ((mq&0o177777777777)>>1) | (bool(ar&F35)*F1) | (ar&F0)),
				 'ASHCL': (((ar&0o377777777776)<<1) | (ar&F0) | (bool(mq&F1)*F35),
						   (((mq&0o177777777777)<<1) | (ar&F0))),
				 'ROTR':  (
				 'ROTL':
				 'ROTCR':
				 'ROTCL':
				 'LSHR':
				 'LSHL':
				 'LSHCR':
				 'LSHCL': }

if __name__ == '__main__':
	unittest.main()
