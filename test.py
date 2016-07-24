#!/usr/bin/env python3
import pydp
import ctypes
from pprint import pprint

e = pydp.Emu()
c = e._emu.apr.contents

def flags(s):
	print(s, '1=0x{:08x} 2=0x{:08x} 3=0x{:08x} 4=0x{:08x} 5=0x{:08x}'.format(
			c._flags1.ivalue, c._flags2.ivalue, c._flags3.ivalue, c._flags4.ivalue, c._flags5.ivalue))
	print('    _flags1:')
	pprint(c._flags1.value)
	print('    _flags2:')
	pprint(c._flags2.value)
	print('    _flags3:')
	pprint(c._flags3.value)
	print('    _flags4:')
	pprint(c._flags4.value)
	print('    _flags5:')
	pprint(c._flags5.value)

print('apr {:x}'.format(ctypes.addressof(c)))
print('before', c.run, c.af3)
flags('flags before')
e.pulse('mr_pwr_clr')
print('after', c.run, c.af3)
flags('flags after')
