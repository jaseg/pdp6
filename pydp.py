from ctypes import *
from ctypes.util import *

lib = CDLL('./libpdp6.so')


class c_bitfield_parent(c_uint32):
	pass

def c_bitfield(name, *bits):
	def fget(idx):
		def fun(self):
			return bool(super().value & (1<<idx))
		return fun
	def fset(idx):
		def fun(self, val):
			super().value = (self.value & ~(1<<idx)) | (int(bool(val))<<idx);
		return fun
	def getbits(self):
		val = super().value
		return { bit: bool(val & (1<<idx)) for idx, bit in enumerate(bits) }
	def setbits(self, vals):
		for bit, val in vals:
			setattr(self, bit, val)
	cdict = {'bits': bits, 'value': property(getbits, setbits)}
	cdict.update({ bit: property(fget(idx), fset(idx)) for idx, bit in enumerate(bits) })
	return type(name, (c_bitfield_parent, ), cdict)


class Constants:
	IO_MAX = 128
	APR_MAX_PULSES = 5


class c_word(c_uint64):
	pass

class c_hword(c_uint32):
	pass

class c_Apr(Structure):
	pass

p_Pulse = CFUNCTYPE(None, POINTER(c_Apr))

class c_Mem(Structure):
	_fields_ = [
			('membus0', c_word),
			('membus1', c_word),
			('hold', POINTER(c_word)),
			('size', c_size_t),
			('fmem', c_word * 16),
			('_memory', c_word * 0) ]

	@property
	def memory(self):
		return (c_word * self.size).from_address(addressof(self._memory))

	@property
	def mem_array(self):
		return [ loc.value for loc in self.memory ]

	@mem_array.setter
	def mem_array_set(self, vals):
		if len(vals) != self.size:
			raise ValueError('Wrong length argument for memory set')
		mem = self.memory
		for i, val in enumerate(vals):
			mem[i].value = val

	@property
	def fmem_array(self):
		return [ word.value for word in self.fmem ]

	@fmem_array.setter
	def fmem_array_set(self, vals):
		if len(val) != len(self.fmem):
			raise ValueError('Wrong length argument for fmem')
		fmem = self.fmem
		for idx, val in enumerate(vals):
			fmem[idx].value = val


class c_IoWake(Structure):
	_fields_ = [
			('func', CFUNCTYPE(None, c_void_p)),
			('arg', c_void_p) ]

class c_Emu(Structure):
	_fields_ = [
			('apr', POINTER(c_Apr)),
			('mem', POINTER(c_Mem)),
			('iobusmap', c_IoWake * Constants.IO_MAX),
			('ioreq', c_ubyte * Constants.IO_MAX),
			('iobus0', c_word),
			('iobus1', c_word) ]

c_Apr._fields_ = [
	('mi', c_word),
	('data', c_word),
	('ir', c_hword),
	('pc', c_hword),
	('ma', c_hword),
	('mas', c_hword),
	('mb', c_word),
	('ar', c_word),
	('mq', c_word),
	('sc', c_uint16),
	('fe', c_uint16),
	('pr', c_uint8),
	('rlr', c_uint8),
	('rla', c_uint8),
	('_pad', c_uint8),
	('_flags1', c_bitfield('apr_flags1',
		'mq36',
		'run',
		'sw_addr_stop',
		'sw_repeat',
		'sw_mem_disable',
		'sw_power',
		'sw_rim_maint',
		'key_start',
		'key_readin',
		'key_mem_cont',
		'key_inst_cont',
		'key_mem_stop',
		'key_inst_stop',
		'key_io_reset',
		'key_exec',
		'key_dep',
		'key_dep_nxt',
		'key_ex',
		'key_ex_nxt',
		'key_rd_off',
		'key_rd_on',
		'key_pt_rd',
		'key_pt_wr')),
	('pio', c_uint8),
	('pir', c_uint8),
	('pih', c_uint8),
	('pi_req', c_uint8),
	('_flags2', c_bitfield('apr_flags2',
		'pi_active',
		'pi_ov',
		'pi_cyc',
		'ex_mode_sync',
		'ex_uuo_sync',
		'ex_pi_sync',
		'ex_ill_op',
		'ex_user',
		'ar_pc_chg_flag',
		'ar_ov_flag',
		'ar_cry0_flag',
		'ar_cry1_flag',
		'ar_cry0',
		'ar_cry1',
		'ar_com_cont',
		'ar_cry0_xor_cry1',
		'key_ex_st',
		'key_ex_sync',
		'key_dep_st',
		'key_dep_sync',
		'key_rd_wr',
		'key_rim_sbr',
		'mc_rd',
		'mc_wr',
		'mc_rq',
		'mc_stop',
		'mc_stop_sync',
		'mc_split_cyc_sync',
		'cpa_iot_user',
		'cpa_illeg_op',
		'cpa_non_exist_mem',
		'cpa_clock_enable')),
	('cpa_pia', c_uint32),
	('_flags3', c_bitfield('apr_flags3',
		'cpa_clock_flag',
		'cpa_pc_chg_enable',
		'cpa_pdl_ov',
		'cpa_arov_enable',
		'iot_go',
		'a_long',
		'if1a',
		'af0',
		'af3',
		'af3a',
		'f1a',
		'f4a',
		'f6a',
		'et4_ar_pse',
		'chf1',
		'chf2',
		'chf3',
		'chf4',
		'chf5',
		'chf6',
		'chf7',
		'lcf1',
		'dcf1',
		'sf3',
		'sf5a',
		'sf7',
		'shf1',
		'mpf1',
		'mpf2',
		'msf1',
		'fsf1',
		'fmf1')),
	('_flags4', c_bitfield('apr_flags4',
		'fmf2',
		'dsf1',
		'dsf2',
		'dsf3',
		'dsf4',
		'dsf5',
		'dsf6',
		'dsf7',
		'dsf8',
		'dsf9',
		'fdf1',
		'fdf2',
		'faf1',
		'faf2',
		'faf3',
		'faf4',
		'fpf1',
		'fpf2',
		'nrf1',
		'nrf2',
		'nrf3',
		'iot_f0a',
		'blt_f0a',
		'blt_f3a',
		'blt_f5a',
		'uuo_f1',
		'ex_inh_rel',
		'ir_fp',
		'ir_fwt')),
	('inst', c_int),
	('io_inst', c_int),
	('_flags5', c_bitfield('apr_flags5',
		'fwt_00',
		'fwt_01',
		'fwt_10',
		'fwt_11',
		'shift_op',
		'ir_md',
		'ir_jp',
		'ir_as',
		'ir_boole',
		'boole_as_00',
		'boole_as_01',
		'boole_as_10',
		'boole_as_11',
		'ir_hwt',
		'hwt_00',
		'hwt_01',
		'hwt_10',
		'hwt_11',
		'ir_acbm',
		'ex_ir_uuo',
		'ir_iot',
		'ir_jrst',
		'fc_e_pse',
		'pc_set')),
	('ir_boole_op', c_uint32),
	('extpulse', c_uint32),
	('pulses1', p_Pulse * Constants.APR_MAX_PULSES),
	('pulses2', p_Pulse * Constants.APR_MAX_PULSES),
	('clist', POINTER(p_Pulse)),
	('nlist', POINTER(p_Pulse)),
	('ncurpulses', c_uint32),
	('nnextpulses', c_uint32),
	('emulation_error', c_int),
	('thr', c_ulong),
	('emu', POINTER(c_Emu)),
	('_flags6', c_bitfield('apr_flags6',
		'ia_inh',
		'pulse_single_step')) ]

for fieldname, ctype in c_Apr._fields_:
	if isinstance(ctype, c_bitfield_parent):
		for bit in ctype.bits:
			def fget(self):
				return getattr(getattr(self, fieldname), bit)
			def fset(self, val):
				setattr(getattr(self, fieldname), bit, val)
			c_Apr.__dict__[bit] = property(fget, fset)

class Emu:
	def __init__(self, memsize=65536):
		rv = c_void_p(lib.emu_init(c_size_t(memsize)))
		self._emu = cast(rv, POINTER(c_Emu)).contents
		self._mem = self._emu.mem.contents
	
	def pulse(self, name):
		getattr(lib, name)(self._emu.apr)

	def wake(self, wid):
		w = self._emu.iobusmap[wid]
		w.func(w.arg)
	

	@property
	def iobus0(self):
		return self._emu.iobus0.value

	@iobus0.setter
	def iobus0_set(self, val):
		self._emu.iobus0.value = val

	@property
	def iobus1(self):
		return self._emu.iobus1.value

	@iobus1.setter
	def iobus1_set(self, val):
		self._emu.iobus1.value = val

	@property
	def membus0(self):
		return self._mem.membus0.value

	@membus0.setter
	def membus0_set(self, val):
		self._mem.membus0.value = val

	@property
	def membus1(self):
		return self._mem.membus1.value

	@membus1.setter
	def membus1_set(self, val):
		self._mem.membus1.value = val

	@property
	def mem_array(self):
		return self._mem.mem_array

	@mem_array.setter
	def mem_array_set(self, vals):
		self._mem.mem_array = vals

	@property
	def fmem_array(self):
		return self._mem.fmem_array

	@fmem_array.setter
	def fmem_array_set(self, vals):
		self._mem.fmem_array = vals

	@property
	def memory(self):
		return self._mem.memory

