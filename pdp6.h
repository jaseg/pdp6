#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>

#define nelem(a) (sizeof(a)/sizeof(a[0]))
#define print printf
#define fprint fprintf

#define debug_print(emu, ...) printf(__VA_ARGS__)

#define IO_MAX 128
#define APR_MAX_PULSES 5
#define boolex(_name) bool _name(Apr *apr)

typedef uint64_t word;
typedef uint32_t hword;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef unsigned char uchar;
typedef uchar bool;

/* Bit masks */
#define FW   0777777777777UL
#define RT   0000000777777UL
#define LT   0777777000000UL
#define SGN  0400000000000UL
#define RSGN 0000000400000UL

enum HalfwordBits {
    /* CAUTION! These are enumerated from the now-offbeat left side */
	H0  = 0400000, H1  = 0200000, H2  = 0100000,
	H3  = 0040000, H4  = 0020000, H5  = 0010000,
	H6  = 0004000, H7  = 0002000, H8  = 0001000,
	H9  = 0000400, H10 = 0000200, H11 = 0000100,
	H12 = 0000040, H13 = 0000020, H14 = 0000010,
	H15 = 0000004, H16 = 0000002, H17 = 0000001
};

/* CAUTION! These are enumerated from the now-offbeat left side */
#define FCRY 01000000000000UL
#define F0   0400000000000UL
#define F1   0200000000000UL
#define F2   0100000000000UL
#define F3   0040000000000UL
#define F4   0020000000000UL
#define F5   0010000000000UL
#define F6   0004000000000UL
#define F7   0002000000000UL
#define F8   0001000000000UL
#define F9   0000400000000UL
#define F10  0000200000000UL
#define F11  0000100000000UL
#define F12  0000040000000UL
#define F13  0000020000000UL
#define F14  0000010000000UL
#define F15  0000004000000UL
#define F16  0000002000000UL
#define F17  0000001000000UL
#define F18  0000000400000UL
#define F19  0000000200000UL
#define F20  0000000100000UL
#define F21  0000000040000UL
#define F22  0000000020000UL
#define F23  0000000010000UL
#define F24  0000000004000UL
#define F25  0000000002000UL
#define F26  0000000001000UL
#define F27  0000000000400UL
#define F28  0000000000200UL
#define F29  0000000000100UL
#define F30  0000000000040UL
#define F31  0000000000020UL
#define F32  0000000000010UL
#define F33  0000000000004UL
#define F34  0000000000002UL
#define F35  0000000000001UL

enum Opcode {
	FSC    = 0132,
	IBP    = 0133,
	CAO    = 0133,
	LDCI   = 0134,
	LDC    = 0135,
	DPCI   = 0136,
	DPC    = 0137,
	ASH    = 0240,
	ROT    = 0241,
        LSH    = 0242,
	ASHC   = 0244,
	ROTC   = 0245,
	LSHC   = 0246,
	EXCH   = 0250,
	BLT    = 0251,
	AOBJP  = 0252,
	AOBJN  = 0253,
	JRST   = 0254,
	JFCL   = 0255,
	XCT    = 0256,
	PUSHJ  = 0260,
	PUSH   = 0261,
	POP    = 0262,
	POPJ   = 0263,
	JSR    = 0264,
	JSP    = 0265,
	JSA    = 0266,
	JRA    = 0267,

	BLKI   = 0700000,
	DATAI  = 0700040,
	BLKO   = 0700100,
	DATAO  = 0700140,
	CONO   = 0700200,
	CONI   = 0700240,
	CONSZ  = 0700300,
	CONSO  = 0700340

};

typedef struct _Tty Tty;
typedef struct _Emu Emu;
typedef struct _Apr Apr;
typedef struct _Mem Mem;
typedef void Pulse(Apr *apr);

/*
 * PULSE HANDLING
 */

#ifdef PY_TESTING
/*
 * Part of the python unit testing framework. This inserts a shim before each pulse invocation that calls
 * py_pulse_begin_cb(const char*, Apr*) and py_pulse_end_cb(const char*, Apr*) before and after executing the pulse,
 * respectively. The generated function names for a pulse instantiated with pulse(foobar) would be foobar_impl for the
 * implementation of the pulse and foobar for the wrapper, doing the callbacks and calling the impl in turn.
 */
int py_pulse_begin_cb(const char *name, Apr *apr);
void py_pulse_end_cb(const char *name, Apr *apr);

#define pulse_decl(_PULSE_NAME) \
    static void _PULSE_NAME(Apr *apr); \
    static void _PULSE_NAME ## _impl(Apr *apr);

#define pulse(_PULSE_NAME) \
    static void _PULSE_NAME(Apr *apr) { \
        if (!py_pulse_begin_cb(#_PULSE_NAME, apr))
            _PULSE_NAME ## _impl(apr);
        py_pulse_end_cb(#_PULSE_NAME, apr);
    } \
    static void _PULSE_NAME ## _impl(Apr *apr)

#else /* !PY_TESTING */
#define pulse(_PULSE_NAME) static void _PULSE_NAME(Apr *apr)
#define pulse_decl(_PULSE_NAME) pulse(_PULSE_NAME)

#endif /* PY_TESTING */

/*
 * CORE CPU STUFF
 */

typedef enum {
    APR_OK = 0,
    APR_ERR_TOO_MANY_PULSES
} AprError;

struct _Apr {
	word mi;
	word data;
	hword ir;
	hword pc;
	hword ma;
	hword mas;
	word mb;
	word ar;
	word mq;
	u16 sc, fe;
	u8 pr, rlr, rla;
    u8 _pad1;
	bool mq36:1;
	bool run:1;
	bool sw_addr_stop:1, sw_repeat:1, sw_mem_disable:1, sw_power:1;
	bool sw_rim_maint:1;
	/* keys */
	bool key_start:1, key_readin:1;
	bool key_mem_cont:1, key_inst_cont:1;
	bool key_mem_stop:1, key_inst_stop:1;
	bool key_io_reset:1, key_exec:1;
	bool key_dep:1, key_dep_nxt:1;
	bool key_ex:1, key_ex_nxt:1;
	bool key_rd_off:1, key_rd_on:1;
	bool key_pt_rd:1, key_pt_wr:1;

	/* PI */
	u8 pio, pir, pih, pi_req;
	bool pi_active:1;
	bool pi_ov, pi_cyc:1;

	/* flip-flops */
	bool ex_mode_sync:1, ex_uuo_sync:1, ex_pi_sync:1, ex_ill_op:1, ex_user:1;
	bool ar_pc_chg_flag:1, ar_ov_flag:1, ar_cry0_flag:1, ar_cry1_flag:1;
	bool ar_cry0:1, ar_cry1:1, ar_com_cont:1;
	bool ar_cry0_xor_cry1:1;

	bool key_ex_st:1, key_ex_sync:1;
	bool key_dep_st:1, key_dep_sync:1;
	bool key_rd_wr:1, key_rim_sbr:1;

	bool mc_rd:1, mc_wr:1, mc_rq:1, mc_stop:1, mc_stop_sync:1, mc_split_cyc_sync:1;

	bool cpa_iot_user:1, cpa_illeg_op:1, cpa_non_exist_mem:1,
	     cpa_clock_enable:1, cpa_clock_flag:1;
    /* one flag word full */
	u32 cpa_pia;
    bool cpa_pc_chg_enable:1, cpa_pdl_ov:1, cpa_arov_enable:1;

	bool iot_go:1;

	/* ?? */
	bool a_long:1;

	/* sbr flip-flops */
	bool if1a:1;
	bool af0:1, af3:1, af3a:1;
	bool f1a:1, f4a:1, f6a:1;
	bool et4_ar_pse:1;
	bool chf1:1, chf2:1, chf3:1, chf4:1, chf5:1, chf6:1, chf7:1;
	bool lcf1:1, dcf1:1;
	bool sf3:1, sf5a:1, sf7:1;
	bool shf1:1;
	bool mpf1:1, mpf2:1;
	bool msf1:1;
	bool fsf1:1;
	bool fmf1:1, fmf2:1;
    /* one flag word full */
	bool dsf1:1, dsf2:1, dsf3:1, dsf4:1, dsf5:1, dsf6:1, dsf7:1, dsf8:1, dsf9:1;
	bool fdf1:1, fdf2:1;
	bool faf1:1, faf2:1, faf3:1, faf4:1;
	bool fpf1:1, fpf2:1;
	bool nrf1:1, nrf2:1, nrf3:1;
	bool iot_f0a:1;
	bool blt_f0a:1, blt_f3a:1, blt_f5a:1;
	bool uuo_f1:1;

	/* temporaries */
	bool ex_inh_rel:1;

	/* decoded instructions */
	bool ir_fp:1;
	bool ir_fwt:1;
	bool fwt_00:1, fwt_01:1, fwt_10:1, fwt_11:1;
    /* one flag word full */
	int inst, io_inst;
	bool shift_op:1, ir_md:1, ir_jp:1, ir_as:1;
	bool ir_boole:1;
	bool boole_as_00:1, boole_as_01:1, boole_as_10:1, boole_as_11:1;
	bool ir_hwt:1;
	bool hwt_00:1, hwt_01:1, hwt_10:1, hwt_11:1;
	bool ir_acbm:1;
	bool ex_ir_uuo:1, ir_iot:1, ir_jrst:1;

	bool fc_e_pse:1;
	bool pc_set:1;

	u32 ir_boole_op;

	/* needed for the emulation */
	u32 extpulse;

	Pulse *pulses1[APR_MAX_PULSES], *pulses2[APR_MAX_PULSES];
	Pulse **clist, **nlist;
	u32 ncurpulses, nnextpulses;

    AprError emulation_error;
    pthread_t thr;
    Emu *emu;

	bool ia_inh:1;	/* this is asserted for some time */
    int pulse_single_step:1;
};

void nextpulse(Apr *apr, Pulse *p);
void apr_cycle(Emu *emu);
void apr_poweron(Emu *emu);
Apr *apr_init(Emu *emu);
void apr_recalc_req(Apr *apr);
void apr_recalc_cpa_req(Apr *apr);


/*
 * MEMORY
 */

struct _Mem {
    /* 0 is cable 1 & 2 (above bits); 1 is cable 3 & 4 (data) */
    word membus0, membus1;
    word *hold;
    size_t size;
    word fmem[16]; /* "Fast memory", used as CPU registers. In the original, this were the first 16 (core) memory
                      locations which usually would be replaced with transistor flip-flops. */
    word memory[]; /* Main memory (core). Length == memsize */
};

void mem_dump(const Mem *mem, const char *filename);
int mem_wake(Mem *mem);
Mem *mem_init(size_t memsize, const char *memfile, const char *regfile);
void mem_read(const char *fname, word *mem, word size);

// 7-2, 7-10
#define MEMBUS_MA21         0000000000001UL
#define MEMBUS_WR_RQ        0000000000004UL
#define MEMBUS_RD_RQ        0000000000010UL
#define MEMBUS_MA_FMC_SEL0  0000001000000UL
#define MEMBUS_MA_FMC_SEL1  0000002000000UL
#define MEMBUS_MA35_0       0000004000000UL
#define MEMBUS_MA35_1       0000010000000UL
#define MEMBUS_MA21_0       0000020000000UL
#define MEMBUS_MA21_1       0000040000000UL
#define MEMBUS_MA20_0       0000100000000UL
#define MEMBUS_MA20_1       0000200000000UL
#define MEMBUS_MA19_0       0000400000000UL
#define MEMBUS_MA19_1       0001000000000UL
#define MEMBUS_MA18_0       0002000000000UL
#define MEMBUS_MA18_1       0004000000000UL
#define MEMBUS_RQ_CYC       0020000000000UL
#define MEMBUS_WR_RS        0100000000000UL
#define MEMBUS_MAI_RD_RS    0200000000000UL
#define MEMBUS_MAI_ADDR_ACK 0400000000000UL


/*
 * TELETYPES
 */

typedef enum {
    TTY_NOT_STARTED = -1,
    TTY_OK = 0,
    TTY_ERR_SOCKET,
    TTY_ERR_BIND,
    TTY_ERR_ACCEPT
} TTYError;

struct _Tty{
    Emu *emu;
    pthread_t thr;
    TTYError error_code;
	int pia;
	int fd;
    int port;
	uchar tto, tti;
	bool tto_busy:1, tto_flag:1;
	bool tti_busy:1, tti_flag:1;
};

void tty_recalc_req(Tty *tty);
void *tty_thread_handler(void *arg);
Tty *tty_init(Emu *emu, int port);


/*
 * TEH EMULATOR
 */

typedef void Wakefunc(void *arg);
typedef struct {
    Wakefunc *func;
    void *arg;
} IoWake;

struct _Emu {
    Apr *apr;
    Mem *mem;
    /* every entry is a function to wake up the device */
    /* TODO: how to handle multiple APRs? */
    IoWake iobusmap[IO_MAX];
    /* current PI req for each device */
    u8 ioreq[IO_MAX];
    /* 0 is cable 1 & 2 (data); 1 is cable 3 & 4 (above bits) */
    word iobus0, iobus1;
};

Emu *emu_init(size_t memsize);
void emu_destroy(Emu *emu);

// 7-10
#define IOBUS_PI_REQ_7      F35
#define IOBUS_PI_REQ_6      F34
#define IOBUS_PI_REQ_5      F33
#define IOBUS_PI_REQ_4      F32
#define IOBUS_PI_REQ_3      F31
#define IOBUS_PI_REQ_2      F30
#define IOBUS_PI_REQ_1      F29
#define IOBUS_IOB_STATUS    F23
#define IOBUS_IOB_DATAI     F22
#define IOBUS_CONO_SET      F21
#define IOBUS_CONO_CLEAR    F20
#define IOBUS_DATAO_SET     F19
#define IOBUS_DATAO_CLEAR   F18
#define IOBUS_IOS9_1        F17
#define IOBUS_IOS9_0        F16
#define IOBUS_IOS8_1        F15
#define IOBUS_IOS8_0        F14
#define IOBUS_IOS7_1        F13
#define IOBUS_IOS7_0        F12
#define IOBUS_IOS6_1        F11
#define IOBUS_IOS6_0        F10
#define IOBUS_IOS5_1        F9
#define IOBUS_IOS5_0        F8
#define IOBUS_IOS4_1        F7
#define IOBUS_IOS4_0        F6
#define IOBUS_IOS3_1        F5
#define IOBUS_IOS3_0        F4
#define IOBUS_MC_DR_SPLIT   F3
#define IOBUS_POWER_ON      F1
#define IOBUS_IOB_RESET     F0
#define IOBUS_PULSES        (IOBUS_CONO_SET | IOBUS_CONO_CLEAR | IOBUS_DATAO_SET | IOBUS_DATAO_CLEAR)

bool iob_reset(Emu *emu);
bool iob_datao_clear(Emu *emu);
bool iob_datao_set(Emu *emu);
bool iob_cono_clear(Emu *emu);
bool iob_cono_set(Emu *emu);
bool iob_status(Emu *emu);
bool iob_datai(Emu *emu);

/* Instruction name constants for pretty-printing */
char *insnames[0700];
char *ionames[010];

