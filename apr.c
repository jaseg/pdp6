#include "pdp6.h"
#include <unistd.h>
#include <string.h>

#define DBG_AR print("AR: %012llo\n", apr->ar)
#define DBG_MB print("MB: %012llo\n", apr->mb)
#define DBG_MQ print("MQ: %012llo\n", apr->mq)
#define DBG_MA print("MA: %06o\n", apr->ma)
#define DBG_IR print("IR: %06o\n", apr->ir)

int dotrace = 0;

void trace(char *fmt, ...) {
    va_list ap;
    if (!dotrace)
        return;
    va_start(ap, fmt);
    printf("  ");
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

#define SWAPLTRT(a) (a = (((a)<<18) & LT) | (((a)>>18) & RT))
#define CONS(a, b) (((a)&LT) | ((b)&RT))

void swap(word *a, word *b) {
    word tmp;
    tmp = *a;
    *a = *b;
    *b = tmp;
}

// 6-10
boolex(ar_ov_set) {
    return apr->ar_cry0 != apr->ar_cry1;
}

boolex(ar0_xor_arov) {
    return !!(apr->ar & F0) != apr->ar_cry0_xor_cry1;
}

boolex(ar0_xor_ar1) {
    return !!(apr->ar & F0) != !!(apr->ar & F1);
}

boolex(ar0_xor_mb0) {
    return !!(apr->ar & F0) != !!(apr->mb & F0);
}

boolex(ar0_eq_sc0) {
    return !!(apr->ar & F0) == !!(apr->sc & 0400);
}

boolex(ar_eq_fp_half) {
    return ((apr->ar & 0377777777) == 0) && (apr->ar & F9);
}

static inline void set_overflow(Apr *apr) {
    apr->ar_ov_flag = 1;
    apr_recalc_cpa_req(apr);
}

// 6-13
boolex(mq35_xor_mb0) {
    return !!(apr->mq & F35) != !!(apr->mb & F0);
}

/* FIXME
 * I'm a bit inconsistent with the decoding.
 * Some things are cached in variables,
 * some are macros...or not even that.
 */

// 5-8
boolex(ir_fpch) {
    return (apr->inst & 0700) == 0100;
}

// 6-19
boolex(ch_inc) {
    return  (apr->inst == CAO  ||
             apr->inst == LDCI ||
             apr->inst == DPCI)
                && !apr->chf5;
}

boolex(ch_inc_op) {
    return ch_inc(apr) && !apr->chf7;
}

boolex(ch_n_inc_op) {
    return  ((apr->inst == LDC || apr->inst == DPC)
                && !apr->chf5) ||
            (ch_inc(apr) && apr->chf7);
}

// 6-20
boolex(ch_load) {
    return (apr->inst == LDCI || apr->inst == LDC) && apr->chf5;
}

boolex(ch_dep) {
    return (apr->inst == DPCI || apr->inst == DPC) && apr->chf5;
}

// 5-8
boolex(ir_fp) {
    return (apr->inst & 0740) == 0140;
}

boolex(ir_fad) {
    return (apr->inst & 0770) == 0140;
}

boolex(ir_fsb) {
    return (apr->inst & 0770) == 0150;
}

boolex(ir_fmp) {
    return (apr->inst & 0770) == 0160;
}

boolex(ir_fdv) {
    return (apr->inst & 0770) == 0170;
}

boolex(ir_fp_dir) {
    return (apr->inst & 0743) == 0140;
}

boolex(ir_fp_rem) {
    return (apr->inst & 0743) == 0141;
}

boolex(ir_fp_mem) {
    return (apr->inst & 0743) == 0142;
}

boolex(ir_fp_both) {
    return (apr->inst & 0743) == 0143;
}

// 5-8
boolex(fwt_movs) {
    return (apr->inst & 0770) == 0200;
}

boolex(fwt_movnm) {
    return (apr->inst & 0770) == 0210;
}

// 5-9
boolex(fwt_swap) {
   return apr->ir_fwt && ((apr->inst & 014) == 004);
}

boolex(fwt_negate) {
    return fwt_movnm(apr) && ((apr->inst & 04) == 0 || (apr->ar & SGN));
}

// 5-8
boolex(ir_mul) {
    return (apr->inst & 0770) == 0220;
}

boolex(ir_div) {
    return (apr->inst & 0770) == 0230;
}

boolex(ir_md_fc_e) {
    return apr->ir_md && ((apr->inst & 03) != 1);
}

boolex(ir_md_fac2) {
    return ir_div(apr) && (apr->ir & H6);
}

boolex(ir_md_sc_e) {
    return apr->ir_md && (apr->ir & H7);
}

boolex(ir_md_sac_inh) {
    return apr->ir_md && ((apr->ir & (H7|H8)) == H7);
}

boolex(ir_md_sac2) {
    return  (ir_div(apr) ||
                (ir_mul(apr) && (apr->ir & H6)))
            && !ir_md_sac_inh(apr);
}

// 6-20
boolex(sh_ac2) {
    return  (apr->inst == ASHC) ||
            (apr->inst == ROTC) ||
            (apr->inst == LSHC);
}

// 5-10
boolex(as_add) {
   return apr->ir_as && !(apr->ir & H6);
}

boolex(as_sub) {
    return apr->ir_as && (apr->ir & H6);
}

// 5-8
boolex(ir_254_7) {
    return (apr->inst & 0774) == 0254;
}

// 6-18
boolex(blt_done) {
    return apr->pi_req || !(apr->mq & F0);
}

boolex(blt_last) {
    return (apr->inst == BLT) && !(apr->mq & F0);
}

// 5-10
boolex(jp_jmp) {
    return   apr->ir_jp &&
            (apr->inst != PUSH) &&
            (apr->inst != POP);
}

boolex(jp_flag_stor) {
    return  (apr->inst == PUSHJ) ||
            (apr->inst == JSR) ||
            (apr->inst == JSP);
}

// 6-3
boolex(mb_pc_sto) {
    return  (apr->inst == PUSHJ) ||
            (apr->inst == JSR) ||
            (apr->inst == JSP) ||
             apr->ir_jrst;
}

// 5-9
boolex(ir_accp_memac) {
    return (apr->inst & 0700) == 0300;
}

boolex(accp) {
    return (apr->inst & 0760) == 0300;
}

boolex(accp_dir) {
    return accp(apr) && (apr->inst & 010);
}
boolex(memac_tst) {
    return (apr->inst & 0760) == 0320;
}

boolex(memac_p1) {
    return (apr->inst & 0760) == 0340;
}

boolex(memac_m1) {
    return (apr->inst & 0760) == 0360;
}

boolex(memac) {
    return memac_tst(apr) || memac_p1(apr) || memac_m1(apr);
}

boolex(memac_mem) {
    return memac(apr) && (apr->inst & 010);
}

boolex(memac_ac) {
    return memac(apr) && ((apr->inst & 010) == 0);
}

boolex(accp_etc_cond) {
    return  (ir_accp_memac(apr) && (apr->ir & H8) && ar0_xor_arov(apr)) ||
            ((apr->ir & H7) && (apr->ar == 0));
}

boolex(accp_et_al_test) {
    return accp_etc_cond(apr) != !!(apr->ir & H6);
}

// 5-9
boolex(hwt_lt) {
    return apr->ir_hwt && !(apr->inst & 040);
}

boolex(hwt_rt) {
    return apr->ir_hwt && (apr->inst & 040);
}

boolex(hwt_swap) {
    return apr->ir_hwt && (apr->inst & 04);
}

boolex(hwt_ar_0) {
    return apr->ir_hwt && (apr->inst & 030);
}

boolex(hwt_lt_set) {
    return hwt_rt(apr) && (apr->inst & 020) && (!(apr->inst & 010) || (apr->mb & RSGN));
}

boolex(hwt_rt_set) {
    return hwt_lt(apr) && (apr->inst & 020) && (!(apr->inst & 010) || (apr->mb & SGN));
}


// 5-9
boolex(acbm_dir) {
    return apr->ir_acbm && (apr->inst & 010);
}

boolex(acbm_swap) {
    return apr->ir_acbm && (apr->inst & 01);
}

boolex(acbm_dn) {
    return apr->ir_acbm && ((apr->inst & 060) == 000);
}

boolex(acbm_cl) {
    return apr->ir_acbm && ((apr->inst & 060) == 020);
}

boolex(acbm_com) {
    return apr->ir_acbm && ((apr->inst & 060) == 040);
}

boolex(acbm_set) {
    return apr->ir_acbm && ((apr->inst & 060) == 060);
}

// 5-8
boolex(uuo_a) {
    return (apr->inst & 0700) == 0;
}

boolex(iot_a) {
    return (apr->inst & 0700) == 0700;
}

boolex(jrst_a) {
    return apr->inst == JRST;
}

// 8-1
boolex(iot_blki) {
    return apr->ir_iot && apr->io_inst == BLKI;
}

boolex(iot_datai) {
    return apr->ir_iot && apr->io_inst == DATAI;
}

boolex(iot_blko) {
    return apr->ir_iot && apr->io_inst == BLKO;
}

boolex(iot_datao) {
    return apr->ir_iot && apr->io_inst == DATAO;
}

boolex(iot_cono) {
    return apr->ir_iot && apr->io_inst == CONO;
}

boolex(iot_coni) {
    return apr->ir_iot && apr->io_inst == CONI;
}

boolex(iot_consz) {
    return apr->ir_iot && apr->io_inst == CONSZ;
}

boolex(iot_conso) {
    return apr->ir_iot && apr->io_inst == CONSO;
}

boolex(iot_blk) {
    return apr->ir_iot && (apr->io_inst == BLKI || apr->io_inst == BLKO);
}

boolex(iot_outgoing) {
    return apr->ir_iot && (apr->io_inst == DATAO || apr->io_inst == CONO);
}

boolex(iot_status) {
    return apr->ir_iot && (apr->io_inst == CONI || apr->io_inst == CONSZ || apr->io_inst == CONSO);
}

boolex(iot_dataio) {
    return apr->ir_iot && (apr->io_inst == DATAI || apr->io_inst == DATAO);
}


void decode_ir(Apr *apr) {
    Emu *emu = apr->emu;
    apr->inst = apr->ir>>9 & 0777;
    apr->io_inst = apr->ir & 0700340;

    // 5-7
    emu->iobus1 &= ~037777000000LL;
    emu->iobus1 |= (apr->ir & H3) ? IOBUS_IOS3_1 : IOBUS_IOS3_0;
    emu->iobus1 |= (apr->ir & H4) ? IOBUS_IOS4_1 : IOBUS_IOS4_0;
    emu->iobus1 |= (apr->ir & H5) ? IOBUS_IOS5_1 : IOBUS_IOS5_0;
    emu->iobus1 |= (apr->ir & H6) ? IOBUS_IOS6_1 : IOBUS_IOS6_0;
    emu->iobus1 |= (apr->ir & H7) ? IOBUS_IOS7_1 : IOBUS_IOS7_0;
    emu->iobus1 |= (apr->ir & H8) ? IOBUS_IOS8_1 : IOBUS_IOS8_0;
    emu->iobus1 |= (apr->ir & H9) ? IOBUS_IOS9_1 : IOBUS_IOS9_0;

    apr->ir_fp = (apr->inst & 0740) == 0140;

    /* 2xx */
    apr->ir_fwt = fwt_movs(apr) || fwt_movnm(apr);
    apr->fwt_00 = apr->fwt_01 = apr->fwt_10 = apr->fwt_11 = 0;
    if (apr->ir_fwt) {
        // 5-9
        apr->fwt_00 = (apr->inst & 03) == 0;
        apr->fwt_01 = (apr->inst & 03) == 1;
        apr->fwt_10 = (apr->inst & 03) == 2;
        apr->fwt_11 = (apr->inst & 03) == 3;
    }
    apr->ir_md = (apr->inst & 0760) == 0220;
    apr->shift_op = (apr->inst & 0770) == 0240 &&
                   (apr->inst & 03) != 3;       // 6-20
    apr->ir_jp = (apr->inst & 0770) == 0260;
    apr->ir_as = (apr->inst & 0770) == 0270;

    /* BOOLE */
    apr->boole_as_00 = apr->boole_as_01 = 0;
    apr->boole_as_10 = apr->boole_as_11 = 0;
    apr->ir_boole = (apr->inst & 0700) == 0400;     // 5-8
    if (apr->ir_boole)   
        apr->ir_boole_op = apr->inst>>2 & 017;

    /* HWT */
    apr->hwt_00 = apr->hwt_01 = apr->hwt_10 = apr->hwt_11 = 0;
    apr->ir_hwt = (apr->inst & 0700) == 0500;   // 5-8
    if (apr->ir_hwt) {
        // 5-9
        apr->hwt_00 = (apr->inst & 03) == 0;
        apr->hwt_01 = (apr->inst & 03) == 1;
        apr->hwt_10 = (apr->inst & 03) == 2;
        apr->hwt_11 = (apr->inst & 03) == 3;
    }

    if (apr->ir_boole || apr->ir_as) {
        apr->boole_as_00 = (apr->inst & 03) == 0;
        apr->boole_as_01 = (apr->inst & 03) == 1;
        apr->boole_as_10 = (apr->inst & 03) == 2;
        apr->boole_as_11 = (apr->inst & 03) == 3;
    }

    /* ACBM */
    apr->ir_acbm = (apr->inst & 0700) == 0600;  // 5-8

    // 5-13
    apr->ex_ir_uuo =
        (uuo_a(apr) && apr->ex_uuo_sync) ||
        (iot_a(apr) && !apr->ex_pi_sync && apr->ex_user && !apr->cpa_iot_user) ||
        (jrst_a(apr) && (apr->ir & 0000600) && apr->ex_user);
    apr->ir_jrst = !apr->ex_ir_uuo && jrst_a(apr);       // 5-8
    apr->ir_iot = !apr->ex_ir_uuo && iot_a(apr);         // 5-8
}

void ar_jfcl_clr(Apr *apr) { // 6-10
    if (apr->ir & H9)  apr->ar_ov_flag = 0;
    if (apr->ir & H10) apr->ar_cry0_flag = 0;
    if (apr->ir & H11) apr->ar_cry1_flag = 0;
    if (apr->ir & H12) apr->ar_pc_chg_flag = 0;
    apr_recalc_cpa_req(apr);
}

void ar_cry(Apr *apr) { // 6-10
    apr->ar_cry0_xor_cry1 = ar_ov_set(apr) && !memac(apr);
}

void ar_cry_in(Apr *apr, word c) {
    word a = (apr->ar & ~F0) + c;
    apr->ar += c;
    apr->ar_cry0 = !!(apr->ar & FCRY);
    apr->ar_cry1 = !!(a       &  F0);
    apr->ar &= FW;
    ar_cry(apr);
}

void set_ex_mode_sync(Apr *apr, bool value) {
    apr->ex_mode_sync = value;
    if (apr->ex_mode_sync)
        apr->ex_user = 1; // 5-13
}

void set_pi_cyc(Apr *apr, bool value) {
    apr->pi_cyc = value;
    if (apr->pi_cyc)
        apr->ex_pi_sync = 1; // 5-13
}

/* get highest priority request above highest currently held */
int get_pi_req(Apr *apr) { // 8-3
    int chan;
    if (apr->pi_active) {
        for(chan = 0100; chan; chan >>= 1) {
            if (apr->pih & chan)
                return 0;
            else if (apr->pir & chan)
                return chan;
        }
    }
    return 0;
}

/* clear highest held break */
void clear_pih(Apr *apr) { // 8-3
    int chan;
    if (apr->pi_active) {
        for(chan = 0100; chan; chan >>= 1) {
            if (apr->pih & chan) {
                apr->pih &= ~chan;
                apr->pi_req = get_pi_req(apr);
                return;
            }
        }
    }
}

void set_pir(Apr *apr, int pir) { // 8-3
    /* held breaks aren't retriggered */
    apr->pir = pir & ~apr->pih;
    apr->pi_req = get_pi_req(apr);
}

void set_pih(Apr *apr, int pih) { // 8-3
    apr->pih = pih;
    apr->pir &= ~apr->pih;
    apr->pi_req = get_pi_req(apr);
}

/* Recalculate the PI requests on the IO bus from each device */
void apr_recalc_req(Apr *apr) {
    int req = 0;
    for (int i = 0; i < 128; i++)
        req |= apr->emu->ioreq[i];
    apr->emu->iobus1 = (apr->emu->iobus1 & ~0177LL) | (req & 0177);
}

void apr_recalc_cpa_req(Apr *apr) { // 8-5
    u8 req = 0;
    if (apr->cpa_illeg_op || apr->cpa_non_exist_mem || apr->cpa_pdl_ov ||
           (apr->cpa_clock_flag && apr->cpa_clock_enable) ||
           (apr->ar_pc_chg_flag && apr->cpa_pc_chg_enable) ||
           (apr->ar_ov_flag && apr->cpa_arov_enable)) {
        req = apr->cpa_pia;
    }
    if (apr->emu->ioreq[0] != req) {
        apr->emu->ioreq[0] = req;
        apr_recalc_req(apr);
    }
}

void set_mc_rq(Apr *apr, bool value) {
    Mem *mem = apr->emu->mem;

    apr->mc_rq = value; // 7-9
    if (value && (apr->mc_rd || apr->mc_wr))
        mem->membus0 |= MEMBUS_RQ_CYC;
    else
        mem->membus0 &= ~MEMBUS_RQ_CYC;
}

void set_mc_wr(Apr *apr, bool value) {
    Mem *mem = apr->emu->mem;

    apr->mc_wr = value; // 7-9
    if (value)
        mem->membus0 |= MEMBUS_WR_RQ;
    else
        mem->membus0 &= ~MEMBUS_WR_RQ;
    set_mc_rq(apr, apr->mc_rq); // 7-9
}

void set_mc_rd(Apr *apr, bool value) {
    Mem *mem = apr->emu->mem;

    apr->mc_rd = value; // 7-9
    if (value)
        mem->membus0 |= MEMBUS_RD_RQ;
    else
        mem->membus0 &= ~MEMBUS_RD_RQ;
    set_mc_rq(apr, apr->mc_rq); // 7-9
}

void set_key_rim_sbr(Apr *apr, bool value) {
    // TODO not sure if this is correct
    apr->key_rim_sbr = value | apr->sw_rim_maint; // 5-2
}

/* Relocation
 * MA is divided into 8:10 bits, the upper 8 bits (18-25) are relocated in RLA
 * by adding RLR. If MA18-25 is above PR we have a relocation error. */
bool relocate(Apr *apr) {
    Mem *mem = apr->emu->mem;

    u8 ma18_25 = (apr->ma>>10) & 0377;
    apr->ex_inh_rel = !apr->ex_user ||  // 5-13
                       apr->ex_pi_sync ||
                      (apr->ma & 0777760) == 0 ||
                           apr->ex_ill_op;
    bool ma_ok = ma18_25 <= apr->pr; // 7-4, PR18 OK
    bool ma_fmc_select = !apr->key_rim_sbr && (apr->ma & 0777760) == 0;  // 7-2
    // 7-5
    apr->rla = ma18_25;
    if (!apr->ex_inh_rel)
        apr->rla += apr->rlr;

    // 7-2, 7-10
    mem->membus0 &= ~0007777777761LL;
    mem->membus0 |= ma_fmc_select ? MEMBUS_MA_FMC_SEL1 : MEMBUS_MA_FMC_SEL0;
    mem->membus0 |= (apr->ma&01777) << 4;
    mem->membus0 |= ((word)apr->rla&017) << 14;
    mem->membus0 |= apr->rla & 0020 ? MEMBUS_MA21_1|MEMBUS_MA21 : MEMBUS_MA21_0;
    mem->membus0 |= apr->rla & 0040 ? MEMBUS_MA20_1 : MEMBUS_MA20_0;
    mem->membus0 |= apr->rla & 0100 ? MEMBUS_MA19_1 : MEMBUS_MA19_0;
    mem->membus0 |= apr->rla & 0200 ? MEMBUS_MA18_1 : MEMBUS_MA18_0;
    mem->membus0 |= apr->ma & 01 ? MEMBUS_MA35_1 : MEMBUS_MA35_0;
    return ma_ok;
}

// 6-17, 6-9
#define cfac_ar_add ar_ast1
#define cfac_ar_sub ar_ast0
#define cfac_ar_negate ar_negate_t0

/*
 * A pulse may first be declared with pulse_decl(pulse_name); , producing the function prototypes and then be defined
 * with pulse(same_pulse_name) { ... } .
 */
pulse_decl(kt1);
pulse_decl(kt4);
pulse_decl(mc_rs_t0);
pulse_decl(mc_addr_ack);
pulse_decl(key_rd_wr_ret);
pulse_decl(it0);
pulse_decl(it1);
pulse_decl(it1a);
pulse_decl(ft0);
pulse_decl(ft1a);
pulse_decl(at0);
pulse_decl(at1);
pulse_decl(at3a);
pulse_decl(iat0);
pulse_decl(et4);
pulse_decl(et5);
pulse_decl(et9);
pulse_decl(et10);
pulse_decl(st7);
pulse_decl(pir_stb);
pulse_decl(mc_wr_rs);
pulse_decl(mc_rd_rq_pulse);
pulse_decl(mc_split_rd_rq);
pulse_decl(mc_wr_rq_pulse);
pulse_decl(mc_rdwr_rq_pulse);
pulse_decl(mc_rd_wr_rs_pulse);
pulse_decl(ar_negate_t0);
pulse_decl(ar_pm1_t1);
pulse_decl(ar_ast0);
pulse_decl(ar_ast1);
pulse_decl(sht1a);
pulse_decl(cht3);
pulse_decl(cht3a);
pulse_decl(cht8a);
pulse_decl(lct0a);
pulse_decl(dct0a);
pulse_decl(mst2);
pulse_decl(mpt0a);
pulse_decl(fst0a);
pulse_decl(fmt0a);
pulse_decl(fmt0b);
pulse_decl(fdt0a);
pulse_decl(fdt0b);
pulse_decl(fpt1a);
pulse_decl(fpt1b);
pulse_decl(nrt0_5);
pulse_decl(fat1a);
pulse_decl(fat5a);
pulse_decl(fat10);

// TODO: find A LONG, it probably doesn't exist

pulse(pi_reset) {
    apr->pi_active = 0; // 8-4
    apr->pih = 0;       // 8-4
    apr->pir = 0;       // 8-4
    apr->pi_req = get_pi_req(apr);
    apr->pio = 0;       // 8-3
}

pulse(ar_flag_clr) {
    apr->ar_ov_flag = 0;        // 6-10
    apr->ar_cry0_flag = 0;      // 6-10
    apr->ar_cry1_flag = 0;      // 6-10
    apr->ar_pc_chg_flag = 0;    // 6-10
    apr->chf7 = 0;          // 6-19
    apr_recalc_cpa_req(apr);
}

pulse(ar_flag_set) {
    apr->ar_ov_flag     = !!(apr->mb & F0); // 6-10
    apr->ar_cry0_flag   = !!(apr->mb & F1); // 6-10
    apr->ar_cry1_flag   = !!(apr->mb & F2); // 6-10
    apr->ar_pc_chg_flag = !!(apr->mb & F3); // 6-10
    apr->chf7           = !!(apr->mb & F4); // 6-19
    if (apr->mb & F5)
        set_ex_mode_sync(apr, 1);   // 5-13
    apr_recalc_cpa_req(apr);
}

pulse(mp_clr) {
    // 6-19
    apr->chf1 = 0;
    apr->chf2 = 0;
    apr->chf3 = 0;
    apr->chf4 = 0;
    apr->chf5 = 0;
    apr->chf6 = 0;
    // 6-20
    apr->lcf1 = 0;
    apr->shf1 = 0;
    // 6-21
    apr->mpf1 = 0;
    apr->mpf2 = 0;
    apr->msf1 = 0;      // 6-24
    // 6-22
    apr->fmf1 = 0;
    apr->fmf2 = 0;
    apr->fdf1 = 0;
    apr->fdf2 = 0;
    apr->faf1 = 0;
    apr->faf2 = 0;
    apr->faf3 = 0;
    apr->faf4 = 0;
    // 6-23
    apr->fpf1 = 0;
    apr->fpf2 = 0;
    // 6-27
    apr->nrf1 = 0;
    apr->nrf2 = 0;
    apr->nrf3 = 0;
}

pulse(ds_clr) {
    // 6-25, 6-26
    apr->dsf1 = 0;
    apr->dsf2 = 0;
    apr->dsf3 = 0;
    apr->dsf4 = 0;
    apr->dsf5 = 0;
    apr->dsf6 = 0;
    apr->dsf8 = 0;
    apr->dsf9 = 0;
    apr->fsf1 = 0;  // 6-19
}

pulse(mr_clr) {
    apr->ir = 0;    // 5-7
    apr->mq = 0;    // 6-13
    apr->mq36 = 0;  // 6-13
    apr->sc = 0;    // 6-15
    apr->fe = 0;    // 6-15

    apr->mc_rd = 0;     // 7-9
    apr->mc_wr = 0;     // 7-9
    apr->mc_rq = 0;     // 7-9
    apr->mc_stop = 0;   // 7-9
    apr->mc_stop_sync = 0;  // 7-9
    apr->mc_split_cyc_sync = 0; // 7-9

    set_ex_mode_sync(apr, 0);   // 5-13
    apr->ex_uuo_sync = 0;   // 5-13
    apr->ex_pi_sync = 0;    // 5-13

    apr->a_long = 0;    // ?? nowhere to be found
    apr->ar_com_cont = 0;   // 6-9
    mp_clr(apr);        // 6-21
    ds_clr(apr);        // 6-26

    apr->iot_go = 0;    // 8-1

    /* sbr flip-flops */
    apr->key_rd_wr = 0; // 5-2
    apr->if1a = 0;      // 5-3
    apr->af0 = 0;       // 5-3
    apr->af3 = 0;       // 5-3
    apr->af3a = 0;      // 5-3
    apr->f1a = 0;       // 5-4
    apr->f4a = 0;       // 5-4
    apr->f6a = 0;       // 5-4
    apr->et4_ar_pse = 0;    // 5-5
    apr->sf3 = 0;       // 5-6
    apr->sf5a = 0;      // 5-6
    apr->sf7 = 0;       // 5-6
    apr->dsf7 = 0;      // 6-25
    apr->iot_f0a = 0;   // 8-1
    apr->blt_f0a = 0;   // 6-18
    apr->blt_f3a = 0;   // 6-18
    apr->blt_f5a = 0;   // 6-18
    apr->uuo_f1 = 0;    // 5-10
    apr->dcf1 = 0;      // 6-20

    // EX UUO SYNC
    decode_ir(apr);
}

pulse(ex_clr) {
    apr->pr = 0;        // 7-4
    apr->rlr = 0;       // 7-5
}

pulse(ex_set) {
    apr->pr  = (apr->emu->iobus0>>28) & 0377;   // 7-4
    apr->rlr = (apr->emu->iobus0>>10) & 0377;   // 7-5
}

pulse(mr_start) {

    // 8-1
    apr->emu->iobus1 |= IOBUS_IOB_RESET;

    // 8-5
    apr->cpa_iot_user = 0;
    apr->cpa_illeg_op = 0;
    apr->cpa_non_exist_mem = 0;
    apr->cpa_clock_enable = 0;
    apr->cpa_clock_flag = 0;
    apr->cpa_pc_chg_enable = 0;
    apr->cpa_pdl_ov = 0;
    apr->cpa_arov_enable = 0;
    apr->cpa_pia = 0;
    apr->emu->ioreq[0] = 0;

    // PI
    apr->pi_ov = 0;     // 8-4
    set_pi_cyc(apr, 0); // 8-4
    nextpulse(apr, pi_reset);   // 8-4
    ar_flag_clr(apr);   // 6-10

    nextpulse(apr, ex_clr); 
    apr->ex_user = 0;   // 5-13
    apr->ex_ill_op = 0; // 5-13
    apr->rla = 0;
}

pulse(mr_pwr_clr) {
    apr->run = 0;   // 5-1
    /* order matters because of EX PI SYNC,
     * better call directly before external pulses can trigger stuff */
    // TODO: is this correct then?
    mr_start(apr);  // 5-2
    mr_clr(apr);    // 5-2
}

/* CPA and PI devices */

void wake_cpa(void *arg) { // 8-5
    Apr *apr = (Apr *)arg;
    Emu *emu = apr->emu;

    if (iob_status(emu)) {
        if (apr->cpa_pdl_ov)        emu->iobus0 |= F19;
        if (apr->cpa_iot_user)      emu->iobus0 |= F20;
        if (apr->ex_user)           emu->iobus0 |= F21;
        if (apr->cpa_illeg_op)      emu->iobus0 |= F22;
        if (apr->cpa_non_exist_mem) emu->iobus0 |= F23;
        if (apr->cpa_clock_enable)  emu->iobus0 |= F25;
        if (apr->cpa_clock_flag)    emu->iobus0 |= F26;
        if (apr->cpa_pc_chg_enable) emu->iobus0 |= F28;
        if (apr->ar_pc_chg_flag)    emu->iobus0 |= F29;
        if (apr->cpa_arov_enable)   emu->iobus0 |= F31;
        if (apr->ar_ov_flag)        emu->iobus0 |= F32;
        emu->iobus0 |= apr->cpa_pia & 7;
    }
    if (iob_cono_set(emu)) {
        if (emu->iobus0 & F18) apr->cpa_pdl_ov = 0;
        if (emu->iobus0 & F19) emu->iobus1 |= IOBUS_IOB_RESET; // 8-1
        if (emu->iobus0 & F22) apr->cpa_illeg_op = 0;
        if (emu->iobus0 & F23) apr->cpa_non_exist_mem = 0;
        if (emu->iobus0 & F24) apr->cpa_clock_enable = 0;
        if (emu->iobus0 & F25) apr->cpa_clock_enable = 1;
        if (emu->iobus0 & F26) apr->cpa_clock_flag = 0;
        if (emu->iobus0 & F27) apr->cpa_pc_chg_enable = 0;
        if (emu->iobus0 & F28) apr->cpa_pc_chg_enable = 1;
        if (emu->iobus0 & F29) apr->ar_pc_chg_flag = 0;    // 6-10
        if (emu->iobus0 & F30) apr->cpa_arov_enable = 0;
        if (emu->iobus0 & F31) apr->cpa_arov_enable = 1;
        if (emu->iobus0 & F32) apr->ar_ov_flag = 0;        // 6-10
        apr->cpa_pia = emu->iobus0 & 7;
        apr_recalc_cpa_req(apr);
    }

    // 5-2
    if (iob_datai(emu))
        apr->ar = apr->data;
    // 5-13
    if (iob_datao_clear(emu))
        ex_clr(apr);
    if (iob_datao_set(emu))
        ex_set(apr);
}

void wake_pi(void *arg) {
    Apr *apr = (Apr *)arg;
    Emu *emu = apr->emu;

    // 8-4, 8-5
    if (iob_status(emu)) {
        trace("PI STATUS %llo\n", emu->iobus0);
        if (apr->pi_active)
            emu->iobus0 |= F28;
        emu->iobus0 |= apr->pio;
    }

    // 8-4, 8-3
    if (iob_cono_clear(emu)) {
        trace("PI CONO CLEAR %llo\n", emu->iobus0);
        if (emu->iobus0 & F23)
            // can call directly
            pi_reset(apr);
    }

    if (iob_cono_set(emu)) {
        trace("PI CONO SET %llo\n", emu->iobus0);
        if (emu->iobus0 & F24)
            set_pir(apr, apr->pir | (emu->iobus0&0177));
        if (emu->iobus0 & F25)
            apr->pio |= emu->iobus0&0177;
        if (emu->iobus0 & F26)
            apr->pio &= ~(emu->iobus0&0177);
        if (emu->iobus0 & F27)
            apr->pi_active = 0;
        if (emu->iobus0 & F28)
            apr->pi_active = 1;
    }
}

/*
 * IOT
 * Here be dragons.
 */

pulse(iot_t4) {
    Emu *emu = apr->emu;

    /* Clear what was set in IOT T2 */
    emu->iobus1 &= ~(IOBUS_IOB_STATUS | IOBUS_IOB_DATAI);
    emu->iobus0 = 0;
}

pulse(iot_t3a) {
    /* delay before reset in IOT T4. Otherwise bus will be
     * reset before bus-pulses triggered by IOT T3 are executed. */
    nextpulse(apr, iot_t4);
}

pulse(iot_t3) {
    Emu *emu = apr->emu;
    // 8-1
    /* Pulses, cleared in the main loop. */
    if (iot_datao(apr))
        emu->iobus1 |= IOBUS_DATAO_SET;
    if (iot_cono(apr))
        emu->iobus1 |= IOBUS_CONO_SET;
    apr->ar |= emu->iobus0; // 6-8
    nextpulse(apr, et5);    // 5-5
}

pulse(iot_t2) {
    Emu *emu = apr->emu;
    // 8-1
    apr->iot_go = 0;
    /* These are asserted during INIT SETUP, IOT T2 and FINAL SETUP.
     * We clear them in IOT T3A which happens after FINAL SETUP */
    if (iot_outgoing(apr))
        emu->iobus0 |= apr->ar;
    if (iot_status(apr))
        emu->iobus1 |= IOBUS_IOB_STATUS;
    if (iot_datai(apr))
        emu->iobus1 |= IOBUS_IOB_DATAI;
    /* Pulses, cleared in the main loop. */
    if (iot_datao(apr))
        emu->iobus1 |= IOBUS_DATAO_CLEAR;
    if (iot_cono(apr))
        emu->iobus1 |= IOBUS_CONO_CLEAR;
    nextpulse(apr, iot_t3);
    nextpulse(apr, iot_t3a);
}

pulse(iot_t0a) {
    apr->iot_f0a = 0;           // 8-1
    apr->ir |= H12;             // 5-8
    decode_ir(apr);
    apr->ma = 0;                // 7-3
    if (apr->pi_cyc && apr->ar_cry0)
        apr->pi_ov = 1;         // 8-4
    if (!apr->pi_cyc && !apr->ar_cry0)
        apr->pc = (apr->pc+1) & RT;   // 5-12
    nextpulse(apr, ft0);            // 5-4
}

pulse(iot_t0) {
    apr->iot_f0a = 1;           // 8-1
    nextpulse(apr, mc_rd_wr_rs_pulse);  // 7-8
}

/*
 * UUO subroutine
 */

pulse(uuo_t2) {
    apr->if1a = 1;          // 5-3
    nextpulse(apr, mc_rd_rq_pulse); // 7-8
}

pulse(uuo_t1) {
    apr->uuo_f1 = 0;        // 5-10
    apr->ma = (apr->ma+1) & RT;   // 7-3
    nextpulse(apr, mr_clr);     // 5-2
    nextpulse(apr, uuo_t2);     // 5-10
}

/*
 * BLT subroutine
 */

pulse(blt_t6) {
    swap(&apr->mb, &apr->ar);   // 6-3
    nextpulse(apr, ft1a);       // 5-4
}

pulse(blt_t5a) {
    apr->blt_f5a = 0;           // 6-18
    if (!(apr->mq & F0))
        apr->pc = (apr->pc+1) & RT;   // 5-12
    nextpulse(apr, blt_done(apr) ? et10 : blt_t6);   // 5-5, 6-18
}

pulse(blt_t5) {
    swap(&apr->mb, &apr->mq);   // 6-17
    apr->blt_f5a = 1;       // 6-18
    nextpulse(apr, ar_pm1_t1);  // 6-9
}

pulse(blt_t4) {
    swap(&apr->mb, &apr->ar);   // 6-3
    nextpulse(apr, pir_stb);    // 8-4
    nextpulse(apr, blt_t5);     // 6-18
}

pulse(blt_t3a) {
    apr->blt_f3a = 0;       // 6-18
    swap(&apr->mb, &apr->mq);   // 6-17
    nextpulse(apr, blt_t4);     // 6-18
}

pulse(blt_t3) {
    apr->blt_f3a = 1;       // 6-18
    nextpulse(apr, ar_ast0);    // 6-9
}

pulse(blt_t2) {
    apr->ar &= RT;          // 6-8
    nextpulse(apr, blt_t3);     // 6-18
}

pulse(blt_t1) {
    swap(&apr->mb, &apr->ar);   // 6-3
    nextpulse(apr, blt_t2);     // 6-18
}

pulse(blt_t0a) {
    apr->blt_f0a = 0;       // 6-18
    apr->mb = apr->mq;      // 6-3
    nextpulse(apr, blt_t1);     // 6-18
}

pulse(blt_t0) {
    swap(&apr->mb, &apr->mq);   // 6-17
    apr->blt_f0a = 1;       // 6-18
    nextpulse(apr, mc_wr_rq_pulse); // 7-8
}

/*
 * Shift subroutines
 */

// 6-14
static inline void sc_com(Apr *apr) {
    apr->sc = ~apr->sc & 0777;
}

static inline void sc_inc(Apr *apr) {
    apr->sc = (apr->sc+1) & 0777;
}

boolex(sc_data) {
    if (apr->chf1)
        return ((~apr->mb>>30) & 077) | 0700;
    else if (apr->chf2)
        return (apr->mb>>24) & 077;
    else if (apr->fsf1 || apr->fpf1 || apr->faf2)
        return (apr->ar>>27) & 0777;
    else if (apr->fpf2 || apr->faf1)
        return (apr->mb>>27) & 0777;
    return 0;
}

static inline void sc_pad(Apr *apr) {
    apr->sc ^= sc_data(apr);
}

static inline void sc_cry(Apr *apr) {
    apr->sc += (~apr->sc & sc_data(apr)) << 1;
}

// 6-7
boolex(shc_ashc) {
    return apr->inst == ASHC || apr->nrf2 || apr->faf3;
}

boolex(shc_div) {
    return ir_div(apr) || ir_fdv(apr) || !apr->nrf2;
}

boolex(ms_mult) {
    return apr->mpf1 || apr->fmf2; // 6-24
}

/* Shift counter */

// 6-7, 6-17, 6-13
static inline void ar_sh_lt(Apr *apr, word ar0_shl_inp, word ar35_shl_inp) {
    apr->ar = ((apr->ar<<1) & 0377777777776) | ar0_shl_inp | ar35_shl_inp;
}

static inline void mq_sh_lt(Apr *apr, word mq0_shl_inp, word mq35_shl_inp) {
    apr->mq = ((apr->mq<<1) & 0377777777776) | mq0_shl_inp | mq35_shl_inp;
}

static inline void ar_sh_rt(Apr *apr, word ar0_shr_inp) {
    apr->ar = ((apr->ar>>1) & 0377777777777) | ar0_shr_inp;
}

static inline void mq_sh_rt(Apr *apr, word mq0_shr_inp, word mq1_shr_inp) {
    apr->mq36 = apr->mq&F35;
    apr->mq = ((apr->mq>>1) & 0177777777777) | mq0_shr_inp | mq1_shr_inp;
}

pulse(sct2) {
    if (apr->shf1) nextpulse(apr, sht1a); // 6-20
    if (apr->chf4) nextpulse(apr, cht8a); // 6-19
    if (apr->lcf1) nextpulse(apr, lct0a); // 6-20
    if (apr->dcf1) nextpulse(apr, dct0a); // 6-20
    if (apr->faf3) nextpulse(apr, fat5a); // 6-22
}

pulse(sct1) {
    word ar0_shl_inp, ar0_shr_inp, ar35_shl_inp;
    word mq0_shl_inp, mq0_shr_inp, mq1_shr_inp, mq35_shl_inp;
    sc_inc(apr); // 6-16

    // 6-7 What a mess, and many things aren't even used here
    if (apr->inst != ASH && !shc_ashc(apr))
        ar0_shl_inp = (apr->ar & F1) << 1;
    else
        ar0_shl_inp = apr->ar & F0;

    if (apr->inst == ROTC)
        ar0_shr_inp = (apr->mq & F35) << 35;
    else if (apr->inst == ROT)
        ar0_shr_inp = (apr->ar & F35) << 35;
    else if (ir_div(apr))
        ar0_shr_inp = (~apr->mq & F35) << 35;
    else if (apr->inst == LSH || apr->inst == LSHC || ch_load(apr))
        ar0_shr_inp = 0;
    else if (apr->inst == ASH || shc_ashc(apr) || ms_mult(apr) || ir_fdv(apr))
        ar0_shr_inp = apr->ar & F0;

    if (apr->inst == ROT)
        ar35_shl_inp = (apr->ar & F0) >> 35;
    else if (apr->inst == ASHC)
        ar35_shl_inp = (apr->mq & F1) >> 34;
    else if (apr->inst == ROTC || apr->inst == LSHC || shc_div(apr))
        ar35_shl_inp = (apr->mq & F0) >> 35;
    else if (ch_dep(apr) || apr->inst == LSH || apr->inst == ASH)
        ar35_shl_inp = 0;

    if (shc_ashc(apr)) {
        mq0_shl_inp = apr->ar & F0;
        mq1_shr_inp = (apr->ar & F35) << 34;
    }else{
        mq0_shl_inp = (apr->mq & F1) << 1;
        mq1_shr_inp = (apr->mq & F0) >> 1;
    }

    if ((ms_mult(apr) && apr->sc == 0777) || shc_ashc(apr))
        mq0_shr_inp = apr->ar & F0;
    else
        mq0_shr_inp = (apr->ar & F35) << 35;

    if (apr->inst == ROTC)
        mq35_shl_inp = (apr->ar & F0) >> 35;
    else if (shc_div(apr))
        mq35_shl_inp = (~apr->ar & F0) >> 35;
    else if (ch_n_inc_op(apr) || ch_inc_op(apr))
        mq35_shl_inp = 1;
    else if (apr->inst == LSHC || shc_ashc(apr) || ch_dep(apr))
        mq35_shl_inp = 0;

    // 6-17
    if (apr->shift_op && !(apr->mb & F18)) {
        ar_sh_lt(apr, ar0_shl_inp, ar35_shl_inp);
        mq_sh_lt(apr, mq0_shl_inp, mq35_shl_inp);
    }
    if ((apr->shift_op && (apr->mb & F18)) || apr->faf3) {
        ar_sh_rt(apr, ar0_shr_inp);
        mq_sh_rt(apr, mq0_shr_inp, mq1_shr_inp);
    }
    if (apr->chf4)
        mq_sh_lt(apr, mq0_shl_inp, mq35_shl_inp);
    if (apr->lcf1)
        ar_sh_rt(apr, ar0_shr_inp);
    if (apr->dcf1) {
        ar_sh_lt(apr, ar0_shl_inp, ar35_shl_inp);
        mq_sh_lt(apr, mq0_shl_inp, mq35_shl_inp);
    }

    if (!(apr->mb & F18) && (apr->inst == ASH || apr->inst == ASHC) && ar0_xor_ar1(apr)) {
        apr->ar_ov_flag = 1;            // 6-10
        apr_recalc_cpa_req(apr);
    }

    if (apr->sc == 0777) // 6-15, 6-16
        nextpulse(apr, sct2);
    else
        nextpulse(apr, sct1);
}

pulse(sct0) {
    if (apr->sc == 0777) // 6-15, 6-16
        nextpulse(apr, sct2);
    else
        nextpulse(apr, sct1);
}

/* Shift adder */

pulse(sat3) {
    if (apr->chf2) nextpulse(apr, cht3a);    // 6-19
    if (apr->fsf1) nextpulse(apr, fst0a);    // 6-19
    if (apr->fpf1) nextpulse(apr, fpt1a);    // 6-23
    if (apr->fpf2) nextpulse(apr, fpt1b);    // 6-23
    if (apr->faf2) nextpulse(apr, fat1a);    // 6-22
}

pulse(sat2_1) {
    sc_cry(apr); // 6-15
    nextpulse(apr, sat3);   // 6-16
}

pulse(sat2) {
    nextpulse(apr, sat2_1); // 6-16
}

pulse(sat1) {
    sc_pad(apr);            // 6-15
    nextpulse(apr, sat2);   // 6-16
}

pulse(sat0) {
    apr->chf1 = 0;          // 6-19
    nextpulse(apr, sat1);   // 6-16
}

/*
 * Shift operations subroutine
 */

pulse(sht1a) {
    apr->shf1 = 0;          // 6-20
    nextpulse(apr, et10);   // 5-5
}

pulse(sht1) {
    if (apr->mb & F18)
        sc_com(apr);        // 6-15
    apr->shf1 = 1;          // 6-20
    nextpulse(apr, sct0);   // 6-16
}

pulse(sht0) {
    sc_inc(apr);            // 6-16
}

/*
 * Character subroutines
 */

pulse(dct3) {
    apr->mb &= apr->ar;     // 6-3
    apr->chf7 = 0;          // 6-19
    nextpulse(apr, et10);   // 5-5
}

pulse(dct2) {
    apr->ar = ~apr->ar & FW;    // 6-17
    nextpulse(apr, dct3);       // 6-20
}

pulse(dct1) {
    apr->ar &= apr->mb;         // 6-8
    apr->mb = apr->mq;          // 6-17
    nextpulse(apr, dct2);       // 6-20
}

pulse(dct0a) {
    apr->dcf1 = 0;              // 6-20
    swap(&apr->mb, &apr->mq);   // 6-17, 6-13 (dct0b)
    apr->ar = ~apr->ar & FW;    // 6-17
    nextpulse(apr, dct1);       // 6-20
}

pulse(dct0) {
    apr->dcf1 = 1;              // 6-20
    sc_com(apr);                // 6-15
    nextpulse(apr, sct0);       // 6-16
}

pulse(lct0a) {
    apr->lcf1 = 0;              // 6-20
    apr->ar &= apr->mb;         // 6-8
    apr->chf7 = 0;              //  6-19
    nextpulse(apr, et10);       // 5-5
}

pulse(lct0) {
    apr->ar = apr->mb;          // 6-8
    apr->mb = apr->mq;          // 6-17
    sc_com(apr);                // 6-15
    apr->lcf1 = 1;              // 6-20
    nextpulse(apr, sct0);       // 6-16
}

pulse(cht9) {
    apr->sc = apr->fe;          // 6-15
    apr->chf5 = 1;              // 6-19
    apr->chf7 = 1;              // 6-19
    nextpulse(apr, at0);        // 5-3
}

pulse(cht8a) {
    apr->chf4 = 0;              // 6-19
    apr->sc = 0;                // 6-15
    apr->ir &= ~037;            // 5-7
    nextpulse(apr, cht9);
}

pulse(cht8b) {
    apr->chf2 = 0;              // 6-19
    apr->chf6 = 0;              // 6-19
    apr->fe = apr->mb>>30 & 077;// 6-14, 6-15
    sc_com(apr);                // 6-15
    if (apr->inst == CAO) {
        nextpulse(apr, st7);    // 5-6
    } else {
        apr->chf4 = 1;          // 6-19
        nextpulse(apr, sct0);   // 6-16
    }
}

pulse(cht8) {
    apr->chf6 = 1;                      // 6-19
    nextpulse(apr, mc_rd_wr_rs_pulse);  // 7-8
}

pulse(cht7) {
    sc_pad(apr);                    // 6-15
    if (ch_inc_op(apr)) {
        swap(&apr->mb, &apr->ar);   // 6-17
        nextpulse(apr, cht8);       // 6-19
    }
    if (ch_n_inc_op(apr))
        nextpulse(apr, cht8b);      // 6-19
}

pulse(cht6) {
    apr->ar = (apr->ar & 0007777777777) |
        (((word)apr->sc & 077) << 30);  // 6-9, 6-4
    if (ch_inc_op(apr))
        apr->sc = 0;                    // 6-15
    apr->chf2 = 1;                      // 6-19
    nextpulse(apr, cht7);               // 6-19
}

pulse(cht5) {
    sc_com(apr);                // 6-15
    nextpulse(apr, cht6);       // 6-19
}

pulse(cht4a) {
    apr->chf3 = 0;              // 6-19
    apr->sc |= 0433;
    nextpulse(apr, cht3);       // 6-19
}

pulse(cht4) {
    apr->sc = 0;                // 6-15
    apr->chf3 = 1;              // 6-19
    nextpulse(apr, ar_pm1_t1);  // 6-9
}

pulse(cht3a) {
    apr->chf2 = 0;              // 6-19
    if (apr->sc & 0400)         // 6-19
        nextpulse(apr, cht5);
    else
        nextpulse(apr, cht4);
}

pulse(cht3) {
    apr->chf2 = 1;              // 6-19
    nextpulse(apr, sat0);       // 6-16
}

pulse(cht2) {
    sc_pad(apr);                // 6-15
    nextpulse(apr, cht3);       // 6-19
}

pulse(cht1) {
    apr->ar = apr->mb;          // 6-8
    apr->chf1 = 1;              // 6-19
    nextpulse(apr, cht2);       // 6-19
}

/*
 * Multiply subroutine
 */

// 6-13
boolex(mq35_eq_mq36) {
    return (apr->mq&F35) == apr->mq36;
}

pulse(mst6) {
    if (apr->mpf1)
        nextpulse(apr, mpt0a);  // 6-21
    if (apr->fmf2)
        nextpulse(apr, fmt0b);  // 6-22
}

pulse(mst5) {
    word mq0_shr_inp, mq1_shr_inp;
    mq0_shr_inp = apr->ar & F0;                 // 6-17
    mq1_shr_inp = (apr->mq & F0) >> 1;          // 6-17
    mq_sh_rt(apr, mq0_shr_inp, mq1_shr_inp);    // 6-17
    apr->sc = 0;                                // 6-15
    nextpulse(apr, mst6);                       // 6-24
}

pulse(mst4) {
    apr->msf1 = 1;                  // 6-24
    nextpulse(apr, cfac_ar_sub);    // 6-17
}

pulse(mst3a) {
    apr->msf1 = 0;              // 6-24
    if (apr->sc == 0777)        // 6-24
        nextpulse(apr, mst5);
    else
        nextpulse(apr, mst2);
}

pulse(mst3) {
    apr->msf1 = 1;                  // 6-24
    nextpulse(apr, cfac_ar_add);    // 6-17
}

pulse(mst2) {
    word ar0_shr_inp, mq0_shr_inp, mq1_shr_inp;
    ar0_shr_inp = apr->ar & F0;                 // 6-7
    mq0_shr_inp = (apr->ar & F35) << 35;        // 6-7
    mq1_shr_inp = (apr->mq & F0) >> 1;          // 6-7
    ar_sh_rt(apr, ar0_shr_inp);                 // 6-17
    mq_sh_rt(apr, mq0_shr_inp, mq1_shr_inp);    // 6-17
    sc_inc(apr);                                // 6-16
    if (mq35_eq_mq36(apr)) {
        if (apr->sc == 0777)                    // 6-24
            nextpulse(apr, mst5);
        else
            nextpulse(apr, mst2);
    }
    if (!(apr->mq&F35) && apr->mq36)
        nextpulse(apr, mst3);   // 6-24
    if (apr->mq&F35 && !apr->mq36)
        nextpulse(apr, mst4);   // 6-24
}

pulse(mst1) {
    apr->mq = apr->mb;          // 6-13
    apr->mb = apr->ar;          // 6-3
    apr->ar = 0;                // 6-8
    if (mq35_eq_mq36(apr) && apr->sc != 0777)
        nextpulse(apr, mst2);   // 6-24
    if (!(apr->mq&F35) && apr->mq36)
        nextpulse(apr, mst3);   // 6-24
    if (apr->mq&F35 && !apr->mq36)
        nextpulse(apr, mst4);   // 6-24
}

/*
 * Divide subroutine
 */

boolex(ds_div) {
    return ir_div(apr) & apr->ir & H6; /* FIXME */
}

boolex(ds_divi) {
    return ir_div(apr) & !(apr->ir & H6);
}

boolex(dsf7_xor_mq0) {
    return apr->dsf7 != !!(apr->mq & F0);
}

pulse(dst21a) {
    apr->dsf9 = 0;                          // 6-26
    swap(&apr->mb, &apr->mq);               // 6-17
    if (ir_div(apr))
        nextpulse(apr, et9);                // 5-5
    if (apr->fdf2) nextpulse(apr, fdt0b);    // 6-22
}

pulse(dst21) {
    apr->dsf9 = 1;                  // 6-26
    nextpulse(apr, cfac_ar_negate); // 6-17
}

pulse(dst20) {
    apr->sc = 0;                    // 6-15
    swap(&apr->mb, &apr->ar);       // 6-17
    if (dsf7_xor_mq0(apr))          // 6-26
        nextpulse(apr, dst21);
    else
        nextpulse(apr, dst21a);
}

pulse(dst19a) {
    apr->dsf8 = 0;                  // 6-26
    swap(&apr->mb, &apr->mq);       // 6-17
    nextpulse(apr, dst20);          // 6-26
}

pulse(dst19) {
    apr->dsf8 = 1;                  // 6-26 DST19B
    nextpulse(apr, cfac_ar_negate); // 6-17
}

pulse(dst18) {
    apr->dsf6 = 1;                  // 6-26
    nextpulse(apr, cfac_ar_sub);    // 6-17
}

pulse(dst17a) {
    apr->dsf6 = 0;                  // 6-26
    if (apr->dsf7)                  // 6-26
        nextpulse(apr, dst19);
    else
        nextpulse(apr, dst19a);
}

pulse(dst17) {
    apr->dsf6 = 1;                  // 6-26
    nextpulse(apr, cfac_ar_add);    // 6-17
}

pulse(dst16) {
    word ar0_shr_inp;
    // 6-7
    if (ir_fdv(apr))
        ar0_shr_inp = apr->ar & F0;
    if (ir_div(apr))
        ar0_shr_inp = (~apr->mq & F35) << 35;
    ar_sh_rt(apr, ar0_shr_inp);         // 6-17
    if (apr->ar & F0) {                 // 6-26
        if (apr->mb & F0)
            nextpulse(apr, dst18);
        else
            nextpulse(apr, dst17);
    } else {
        nextpulse(apr, dst17a);         // 6-26
    }
}

pulse(dst15) {
    apr->dsf5 = 1;                  // 6-26
    nextpulse(apr, cfac_ar_add);    // 6-17
}

pulse_decl(dst14);

pulse(dst14b) {
    if (apr->sc == 0777)
        nextpulse(apr, dst16);      // 6-26
    else if (mq35_xor_mb0(apr))     // 6-26
        nextpulse(apr, dst14);
    else
        nextpulse(apr, dst15);
}

pulse(dst14a) {
    word ar0_shl_inp, ar35_shl_inp;
    word mq0_shl_inp, mq35_shl_inp;
    apr->dsf5 = 0;                              // 6-26
    sc_inc(apr);                                // 6-16
    // 6-7
    ar0_shl_inp = (apr->ar & F1) << 1;
    ar35_shl_inp = (apr->mq & F0) >> 35;
    mq0_shl_inp = (apr->mq & F1) << 1;
    mq35_shl_inp = (~apr->ar & F0) >> 35;
    ar_sh_lt(apr, ar0_shl_inp, ar35_shl_inp);   // 6-17
    mq_sh_lt(apr, mq0_shl_inp, mq35_shl_inp);   // 6-17
    nextpulse(apr, dst14b);                     // 6-26
}

pulse(dst14) {
    apr->dsf5 = 1;                  // 6-26
    nextpulse(apr, cfac_ar_sub);    // 6-17
}

pulse(dst13) {
    set_overflow(apr);              // 6-17
    nextpulse(apr, st7);            // 5-6
}

pulse(dst12) {
    apr->dsf4 = 1;                  // 6-25
    nextpulse(apr, cfac_ar_sub);    // 6-17
}

pulse(dst11a) {
    apr->dsf4 = 0;                  // 6-25
    if (apr->ar & F0)               // 6-25, 6-26
        nextpulse(apr, dst14a);
    else
        nextpulse(apr, dst13);
}

pulse(dst11) {
    apr->dsf4 = 1;                  // 6-25
    nextpulse(apr, cfac_ar_add);    // 6-17
}

pulse(dst10b) {
    word mq0_shl_inp, mq35_shl_inp;
    mq0_shl_inp = (apr->mq & F1) << 1;          // 6-7
    mq35_shl_inp = (~apr->ar & F0) >> 35;       // 6-7
    mq_sh_lt(apr, mq0_shl_inp, mq35_shl_inp);   // 6-17
    if (apr->mb & F0)                           // 6-15
        nextpulse(apr, dst11);
    else
        nextpulse(apr, dst12);
}

pulse(dst10a) {
    word ar0_shr_inp;
    apr->mq = (apr->mq & ~F0) | ((apr->ar & F35) << 35);  // 6-13
    ar0_shr_inp = apr->ar & F0;                         // 6-7
    ar_sh_rt(apr, ar0_shr_inp);                         // 6-17
    nextpulse(apr, apr->mb & F0 ? dst11 : dst12);       // 6-15
}
 
pulse(dst10) {
    apr->dsf3 = 0;              // 6-25
    if (ir_fdv(apr))
        nextpulse(apr, dst10a); // 6-25
    if (ir_div(apr))
        nextpulse(apr, dst10b); // 6-25
}

pulse(dst9) {
    swap(&apr->mb, &apr->mq);       // 6-17
    apr->dsf3 = 1;                  // 6-25
    nextpulse(apr, cfac_ar_negate); // 6-17
}

pulse(dst8) {
    swap(&apr->mb, &apr->ar);   // 6-17
    nextpulse(apr, dst9);       // 6-25
}

pulse(dst7) {
    swap(&apr->mb, &apr->mq);   // 6-17
    apr->ar = ~apr->ar & FW;    // 6-17
    nextpulse(apr, dst10);      // 6-25
}

pulse(dst6) {
    swap(&apr->mb, &apr->ar);   // 6-17
    nextpulse(apr, dst7);       // 6-25
}

pulse(dst5a) {
    apr->dsf2 = 0;              // 6-25
    if (apr->ar)                // 6-25
        nextpulse(apr, dst6);
    else
        nextpulse(apr, dst8);
}

pulse(dst5) {
    apr->dsf2 = 1;                  // 6-25
    nextpulse(apr, cfac_ar_negate); // 6-17
}

pulse(dst4) {
    swap(&apr->mb, &apr->ar);   // 6-17
    nextpulse(apr, dst5);       // 6-25
}

pulse(dst3) {
    swap(&apr->mb, &apr->mq);   // 6-17
    apr->dsf7 = 1;              // 6-25
    nextpulse(apr, dst4);       // 6-25
}

pulse(dst2) {
    swap(&apr->mb, &apr->mq);   // 6-17
    apr->ar = 0;                // 6-8
    nextpulse(apr, dst10);      // 6-25
}

pulse(dst1) {
    apr->mq = apr->mb;          // 6-13
    apr->mb = apr->ar;          // 6-3
    nextpulse(apr, dst2);       // 6-25
}

pulse(dst0a) {
    apr->dsf1 = 0;              // 6-25
    if (ds_divi(apr))           // 6-25
        nextpulse(apr, dst1);
    else
        nextpulse(apr, dst10);
}

pulse(dst0) {
    apr->dsf7 = 1;                  // 6-25
    apr->dsf1 = 1;                  // 6-25
    nextpulse(apr, cfac_ar_negate); // 6-17
}

pulse(ds_div_t0) {
    apr->sc = 0733;         // 6-14
}

/*
 * Floating point subroutines
 */

/* Normalize return */

// 6-27
boolex(ar_0_and_mq1_0) {
    return apr->ar == 0 && !(apr->mq & F1);
}

boolex(ar9_eq_ar0) {
    return !!(apr->ar & F9) == !!(apr->ar & F0);
}

boolex(nr_round) {
    return (apr->ir & H6) && (apr->mq & F1) && !apr->nrf3;
}

pulse(nrt6) {
    nextpulse(apr, et10);   // 5-5
}

pulse(nrt5a) {
    apr->nrf1 = 0;          // 6-27
    apr->nrf3 = 1;          // 6-27
    nextpulse(apr, nrt0_5); // 6-27
}

pulse(nrt5) {
    apr->nrf1 = 1;              // 6-27
    nextpulse(apr, ar_pm1_t1);  // 6-9
}

pulse(nrt4) {
    apr->ar |= (apr->ar & 0400777777777) | ((word)apr->sc&0377)<<27;     // 6-4, 6-9
    nextpulse(apr, nrt6); // 6-27
}

pulse(nrt3) {
    if (!(apr->sc & 0400))
        set_overflow(apr);                  // 6-17
    if (!(apr->ar & F0) || nr_round(apr))
        sc_com(apr);                        // 6-15
    if (nr_round(apr))                      // 6-27
        nextpulse(apr, nrt5);
    else
        nextpulse(apr, nrt4);
}

pulse(nrt2) {
    word ar0_shl_inp, ar35_shl_inp;
    word mq0_shl_inp, mq35_shl_inp;
    sc_inc(apr);                                // 6-16
    // 6-7
    ar0_shl_inp = apr->ar & F0;
    mq0_shl_inp = apr->ar & F0;
    ar35_shl_inp = (apr->mq & F1) >> 34;
    mq35_shl_inp = 0;
    ar_sh_lt(apr, ar0_shl_inp, ar35_shl_inp);   // 6-17
    mq_sh_lt(apr, mq0_shl_inp, mq35_shl_inp);   // 6-17
    if (ar_eq_fp_half(apr) || !ar9_eq_ar0(apr))
        nextpulse(apr, nrt3);                   // 6-27
    else
        nextpulse(apr, nrt2);                   // 6-27
}

pulse(nrt1) {
    sc_com(apr);                // 6-15
    if (ar_eq_fp_half(apr) || !ar9_eq_ar0(apr))
        nextpulse(apr, nrt3);   // 6-27
    else
        nextpulse(apr, nrt2);   // 6-27
}

pulse(nrt0) {
    word ar0_shr_inp;
    word mq0_shr_inp, mq1_shr_inp;
    sc_inc(apr);                                // 6-16
    // 6-7
    ar0_shr_inp = apr->ar & F0;
    mq0_shr_inp = apr->ar & F0;
    mq1_shr_inp = (apr->ar & F35) << 34;
    ar_sh_rt(apr, ar0_shr_inp);                 // 6-17
    mq_sh_rt(apr, mq0_shr_inp, mq1_shr_inp);    // 6-17
    if (ar_0_and_mq1_0(apr))
        nextpulse(apr, nrt6);                   // 6-27
    else
        nextpulse(apr, nrt1);                   // 6-27
}

pulse(nrt0_5) {
    apr->nrf2 = 1;          // 6-27
    nextpulse(apr, nrt0);   // 6-27
}

/* Scale */

pulse(fst1) {
    sc_inc(apr); // 6-16
}

pulse(fst0a) {
    apr->fsf1 = 0;  // 6-19
    if (!ar0_eq_sc0(apr))
        set_overflow(apr);  // 6-17
    apr->ar |= (apr->ar & 0400777777777) | ((word)apr->sc&0377)<<27; // 6-4, 6-9
    nextpulse(apr, et10);   // 5-5
}

pulse(fst0) {
    apr->fsf1 = 1;          // 6-19
    nextpulse(apr, sat0);   // 6-16
}

/* Exponent calculate */

// 6-23
boolex(ar0_xor_fmf1) {
    return !!(apr->ar & F0) != !!apr->fmf1;
}

boolex(ar0_xor_mb0_xor_fmf1) {
    return ar0_xor_fmf1(apr) != !!(apr->mb & F0);
}

boolex(mb0_eq_fmf1) {
    return !!(apr->mb & F0) == !!apr->fmf1;
}

pulse(fpt4) {
    // 6-22
    if (apr->fmf1)
        nextpulse(apr, fmt0a);
    if (apr->fdf1)
        nextpulse(apr, fdt0a);
}

pulse(fpt3) {
    apr->fe |= apr->sc; // 6-15
    apr->sc = 0;        // 6-15
    // 6-3, 6-4
    if (apr->mb & F0) apr->mb |= 0377000000000LL;
    else             apr->mb &= ~0377000000000LL;
    // 6-9, 6-4
    if (apr->ar & F0) apr->ar |= 0377000000000LL;
    else             apr->ar &= ~0377000000000LL;
    nextpulse(apr, fpt4);   // 6-23
}

pulse(fpt2) {
    sc_inc(apr); // 6-17
}

pulse(fpt1b) {
    apr->fpf2 = 0;          // 6-23
    if (mb0_eq_fmf1(apr))
        sc_com(apr);        // 6-15
    nextpulse(apr, fpt3);   // 6-23
}

pulse(fpt1aa) {
    apr->fpf2 = 1;          // 6-23
    nextpulse(apr, sat0);   // 6-15
}

pulse(fpt1a) {
    apr->fpf1 = 0;                  // 6-23
    if (ar0_xor_mb0_xor_fmf1(apr))
        nextpulse(apr, fpt2);       // 6-23
    else
        sc_com(apr);                // 6-15
    nextpulse(apr, fpt1aa);         // 6-23
}

pulse(fpt1) {
    apr->fpf1 = 1;                  // 6-23
    if (ar0_xor_fmf1(apr))
        sc_com(apr);                // 6-15
    nextpulse(apr, sat0);           // 6-16
}

pulse(fpt0) {
    apr->sc |= 0200;        // 6-14
    nextpulse(apr, fpt1);   // 6-23
}

/* Multiply */

pulse(fmt0b) {
    apr->fmf2 = 0;              // 6-22
    apr->sc |= apr->fe;         // 6-15
    apr->nrf2 = 1;              // 6-27
    if (ar_0_and_mq1_0(apr))
        nextpulse(apr, nrt6);   // 6-27
    else
        nextpulse(apr, nrt1);   // 6-27
}

pulse(fmt0a) {
    apr->fmf1 = 0;          // 6-22
    apr->fmf2 = 1;          // 6-22
    apr->sc |= 0744;        // 6-14
    nextpulse(apr, mst1);   // 6-24
}

pulse(fmt0) {
    apr->fmf1 = 1;      // 6-22
    nextpulse(apr, fpt0);   // 6-23
}

/* Divide */

pulse(fdt1) {
    word ar0_shr_inp;
    word mq0_shr_inp, mq1_shr_inp;
    // 6-7
    ar0_shr_inp = apr->ar & F0;
    mq0_shr_inp = apr->ar & F0;
    mq1_shr_inp = (apr->ar & F35) << 34;
    ar_sh_rt(apr, ar0_shr_inp);                 // 6-17
    mq_sh_rt(apr, mq0_shr_inp, mq1_shr_inp);    // 6-17
    nextpulse(apr, nrt0_5);                     // 6-27
}

pulse(fdt0b) {
    apr->fdf2 = 0;          // 6-22
    apr->sc = apr->fe;      // 6-15
    apr->nrf2 = 1;          // 6-27
    nextpulse(apr, fdt1);   // 6-22
}

pulse(fdt0a) {
    apr->fdf1 = 0;      // 6-22
    apr->fdf2 = 1;      // 6-22
    apr->sc = 0741;     // 6-14
    if (apr->ar & F0)   // 6-25
        nextpulse(apr, dst0);
    else
        nextpulse(apr, dst10);
}

pulse(fdt0) {
    apr->fdf1 = 1;      // 6-22
    nextpulse(apr, fpt0);   // 6-23
}

/* Add/Subtract */

pulse(fat10) {
    apr->faf4 = 0;      // 6-22
    apr->faf1 = 0;      // 6-22
    nextpulse(apr, nrt0_5); // 6-27
}

pulse(fat9) {
    apr->faf4 = 1;      // 6-22
    nextpulse(apr, cfac_ar_add);    // 6-17
}

pulse(fat8a) {
    // 6-3, 6-4
    if (apr->mb & F0) apr->mb |=  0377000000000;
    else             apr->mb &= ~0377000000000;
    nextpulse(apr, fat9);   // 6-22
}

pulse(fat8) {
    sc_pad(apr);            // 6-15
    nextpulse(apr, fat8a);  // 6-22
}

pulse(fat7) {
    if (apr->mb & F0)
        sc_com(apr);        // 6-15
    nextpulse(apr, fat8);   // 6-22
}

pulse(fat6) {
    apr->ar = 0;        // 6-8
    nextpulse(apr, fat5a);  // 6-22
}

pulse(fat5a) {
    apr->faf3 = 0;      // 6-22
    apr->sc = 0;        // 6-15
    apr->faf1 = 1;      // 6-22
    nextpulse(apr, fat7);   // 6-22
}

pulse(fat5) {
    // 6-9, 6-4
    if (apr->ar & F0) apr->ar |=  0377000000000;
    else             apr->ar &= ~0377000000000;
    apr->faf3 = 1;      // 6-22
    nextpulse(apr, sct0);   // 6-16
}

pulse(fat4) {
    if ((apr->sc & 0700) == 0700) // 6-22
        nextpulse(apr, fat5);
    else
        nextpulse(apr, fat6);
}

pulse(fat3) {
    sc_com(apr);         // 6-15
    if ((apr->sc & 0700) == 0700) // 6-22
        nextpulse(apr, fat5);
    else
        nextpulse(apr, fat6);
}

pulse(fat2) {
    sc_inc(apr);            // 6-16
    nextpulse(apr, fat3);   // 6-22
}

pulse(fat1b) {
    apr->faf1 = 0;      // 6-22
    apr->faf2 = 1;      // 6-22
}

pulse(fat1a) {
    apr->faf2 = 0;                  // 6-22
    if (!!(apr->ar & F0) == !!(apr->sc & 0400))
        swap(&apr->mb, &apr->ar);   // 6-17
    if (apr->sc & 0400)             // 6-22
        nextpulse(apr, fat4);
    else
        nextpulse(apr, fat2);
}

pulse(fat1) {
    sc_pad(apr);            // 6-15
    nextpulse(apr, fat1b);  // 6-22
    nextpulse(apr, sat0);   // 6-16
}

pulse(fat0) {
    if (!ar0_xor_mb0(apr))
        sc_com(apr);        // 6-15
    apr->faf1 = 1;          // 6-22
    nextpulse(apr, fat1);   // 6-22
}

/*
 * Fixed point multiply
 */

pulse(mpt2) {
    apr->ar = apr->mb;      // 6-8
    nextpulse(apr, nrt6);   // 6-27
}

pulse(mpt1) {
    apr->mb = apr->mq;      // 6-17
    if (apr->ar != 0)
        set_overflow(apr);  // 6-17
    nextpulse(apr, mpt2);   // 6-21
}

pulse(mpt0a) {
    apr->mpf1 = 0;                              // 6-21
    if (!(apr->ir & H6) && apr->ar & F0)
        apr->ar = ~apr->ar & FW;                // 6-17
    if (apr->ar & F0 && apr->mpf2)
        set_overflow(apr);                      // 6-17
    if (apr->ir & H6) // 6-21, 6-27
        nextpulse(apr, nrt6);
    else
        nextpulse(apr, mpt1);
}

pulse(mpt0) {
    apr->sc |= 0734;        // 6-14
    apr->mpf1 = 1;          // 6-21
    if ((apr->ar & F0) && (apr->mb & F0))
        apr->mpf2 = 1;      // 6-21
    nextpulse(apr, mst1);   // 6-24
}

/*
 * AR subroutines
 */

// 6-9
boolex(ar_sub) {
    return as_sub(apr) || accp(apr);
}

boolex(ar_add) {
    return as_add(apr);
}

boolex(ar_p1) {
    return  memac_p1(apr) ||
            apr->inst == PUSH ||
            apr->inst == PUSHJ ||
            iot_blk(apr) ||
            apr->inst == AOBJP ||
            apr->inst == AOBJN;
}

boolex(ar_m1) {
    return memac_m1(apr) || apr->inst == POP || apr->inst == POPJ;
}

boolex(ar_pm1_ltrt) {
    return  iot_blk(apr) ||
            apr->inst == AOBJP ||
            apr->inst == AOBJN ||
            (apr->ir_jp && !(apr->inst & 0004));
}

boolex(ar_sbr) {
    return  fwt_negate(apr) ||
            ar_add(apr) ||
            ar_sub(apr) ||
            ar_p1(apr) ||
            ar_m1(apr) ||
            ir_fsb(apr);
}


pulse(art3) {
    apr->ar_com_cont = 0; // 6-9

    if (apr->af3a)       nextpulse(apr, at3a);       // 5-3
    if (apr->et4_ar_pse) nextpulse(apr, et4);        // 5-5
    if (apr->blt_f3a)    nextpulse(apr, blt_t3a);    // 6-18
    if (apr->blt_f5a)    nextpulse(apr, blt_t5a);    // 6-18
    if (apr->chf3)       nextpulse(apr, cht4a);      // 6-19
    if (apr->msf1)       nextpulse(apr, mst3a);      // 6-24
    if (apr->dsf1)       nextpulse(apr, dst0a);      // 6-25
    if (apr->dsf2)       nextpulse(apr, dst5a);      // 6-25
    if (apr->dsf3)       nextpulse(apr, dst10);      // 6-25
    if (apr->dsf4)       nextpulse(apr, dst11a);     // 6-25
    if (apr->dsf5)       nextpulse(apr, dst14a);     // 6-26
    if (apr->dsf6)       nextpulse(apr, dst17a);     // 6-26
    if (apr->dsf8)       nextpulse(apr, dst19a);     // 6-26
    if (apr->dsf9)       nextpulse(apr, dst21a);     // 6-26
    if (apr->nrf1)       nextpulse(apr, nrt5a);      // 6-27
    if (apr->faf4)       nextpulse(apr, fat10);      // 6-22
}

pulse(ar_cry_comp) {
    if (apr->ar_com_cont) {
        apr->ar = ~apr->ar & FW;    // 6-8
        nextpulse(apr, art3);       // 6-9
    }
}

pulse(ar_pm1_t1) {
    ar_cry_in(apr, 1);          // 6-6
    if (apr->inst == BLT || ar_pm1_ltrt(apr))
        ar_cry_in(apr, 01000000);   // 6-9
    if (!apr->ar_com_cont)
        nextpulse(apr, art3);       // 6-9
    nextpulse(apr, ar_cry_comp);        // 6-9
}

pulse(ar_pm1_t0) {
    apr->ar = ~apr->ar & FW;    // 6-8
    apr->ar_com_cont = 1;       // 6-9
    nextpulse(apr, ar_pm1_t1);  // 6-9
}

pulse(ar_negate_t0) {
    apr->ar = ~apr->ar & FW;    // 6-8
    nextpulse(apr, ar_pm1_t1);  // 6-9
}

pulse(ar_ast2) {
    ar_cry_in(apr, (~apr->ar & apr->mb) << 1);  // 6-8
    if (!apr->ar_com_cont)
        nextpulse(apr, art3);           // 6-9
    nextpulse(apr, ar_cry_comp);            // 6-9
}

pulse(ar_ast1) {
    apr->ar ^= apr->mb;     // 6-8
    nextpulse(apr, ar_ast2);    // 6-9
}

pulse(ar_ast0) {
    apr->ar = ~apr->ar & FW;    // 6-8
    apr->ar_com_cont = 1;       // 6-9
    nextpulse(apr, ar_ast1);    // 6-9
}


pulse(xct_t0) {
    nextpulse(apr, mr_clr);     // 5-2
    nextpulse(apr, it1a);       // 5-3
}

/*
 * Priority Interrupt
 */

// 5-3
boolex(ia_not_int) {
    return (!apr->pi_req || apr->pi_cyc) && !apr->ia_inh;
}

// 8-4
boolex(pi_blk_rst) {
    return !apr->pi_ov && iot_dataio(apr);
}

boolex(pi_rst) {
    return (apr->ir_jrst && (apr->ir & H9)) || (pi_blk_rst(apr) && apr->pi_cyc);
}

boolex(pi_hold) {
    return (!apr->ir_iot || pi_blk_rst(apr)) && apr->pi_cyc;
}


pulse(pir_stb) {
    set_pir(apr, apr->pir | (apr->pio & apr->emu->iobus1)); // 8-3
}

pulse(pi_sync) {
    /* Call directly, we need the result in this pulse */
    if (!apr->pi_cyc)
        pir_stb(apr);           // 8-4
    if (apr->pi_req && !apr->pi_cyc)
        nextpulse(apr, iat0);       // 5-3
    if (ia_not_int(apr))
        nextpulse(apr, apr->if1a ? it1 : at1);  // 5-3
}

// 5-1
boolex(key_manual_cond) {
    return  apr->key_readin ||
            apr->key_start ||
            apr->key_inst_cont ||
            apr->key_mem_cont ||
            apr->key_ex ||
            apr->key_dep ||
            apr->key_ex_nxt ||
            apr->key_dep_nxt ||
            apr->key_exec ||
            apr->key_io_reset;
}

boolex(key_ma_mas) {
    return  apr->key_readin ||
            apr->key_start ||
            apr->key_ex_sync ||
            apr->key_dep_sync;
}

boolex(key_clr_rim) {
    return !(apr->key_readin || apr->key_inst_cont || apr->key_mem_cont);
}

boolex(key_execute) {
    return apr->key_exec && !apr->run;
}

boolex(key_ex_ex_nxt) {
    return apr->key_ex_sync || apr->key_ex_nxt;
}

boolex(key_dp_dp_nxt) {
    return apr->key_dep_sync || apr->key_dep_nxt;
}

boolex(key_execute_dp_dpnxt) {
    return key_execute(apr) || key_dp_dp_nxt(apr);
}


/*
 * Store
 */

// 5-6
boolex(sc_e) {
    return  apr->inst == PUSHJ ||
            apr->inst == PUSH ||
            apr->inst == POP ||
            apr->inst == JSR ||
            apr->inst == JSA ||
            iot_datai(apr) ||
            iot_coni(apr) ||
            apr->fwt_10 ||
            ir_md_sc_e(apr) ||
            ir_fp_mem(apr) ||
            ir_fp_both(apr);
}

boolex(sac0) {
    return (apr->ir & 0740) == 0;
}

boolex(sac_inh_if_ac_0) {
    return sac0(apr) && (apr->fwt_11 || apr->hwt_11 || memac_mem(apr));
}

boolex(sac_inh) {
    return  apr->boole_as_10 ||
            apr->hwt_10 ||
            blt_last(apr) ||
            accp(apr) ||
            apr->inst == JSR ||
            apr->fwt_10 ||
            acbm_dn(apr) ||
            apr->ir_iot ||
            ir_md_sac_inh(apr) ||
            ch_dep(apr) ||
            ir_fp_mem(apr) ||
            ir_254_7(apr) ||
            sac_inh_if_ac_0(apr);
}

boolex(sac2) {
    return sh_ac2(apr) || ir_fp_rem(apr) || ir_md_sac2(apr);
}


pulse(st7) {
    apr->sf7 = 0; // 5-6
    if (apr->run) {
        if (apr->key_ex_st || apr->key_dep_st) // 5-1, 5-3
            nextpulse(apr, kt1);
        else
            nextpulse(apr, it0);
    }
    if (apr->key_start || apr->key_readin || apr->key_inst_cont) {
        nextpulse(apr, kt4);    // 5-2
    }
}

pulse(st6a) {
    apr->sf7 = 1; // 5-6
}

pulse(st6) {
    /* We know SAC2 is asserted
     * so clamp to fast memory addresses */
    apr->ma = (apr->ma+1) & 017;      // 7-3
    apr->mb = apr->mq;              // 6-3
    nextpulse(apr, st6a);           // 5-6
    nextpulse(apr, mc_wr_rq_pulse); // 7-8
}

pulse(st5a) {
    apr->sf5a = 0;  // 5-6
    if (sac2(apr))       // 5-6
        nextpulse(apr, st6);
    else
        nextpulse(apr, st7);
}

pulse(st5) {
    apr->sf5a = 1;                  // 5-6
    apr->ma |= apr->ir>>5 & 017;    // 7-3
    apr->mb = apr->ar;              // 6-3
    nextpulse(apr, mc_wr_rq_pulse); // 7-8
}

pulse(st3a) {
    nextpulse(apr, st5);
}

pulse(st3) {
    apr->sf3 = 0;               // 5-6
    if (sac_inh(apr)) {
        nextpulse(apr, st7);    // 5-6
    } else {
        apr->ma = 0;            // 7-3
        nextpulse(apr, st3a);   // 5-6
    }
}

pulse(st2) {
    apr->sf3 = 1;                       // 5-6
    nextpulse(apr, mc_rd_wr_rs_pulse);  // 7-8
}

pulse(st1) {
    apr->sf3 = 1;                   // 5-6
    nextpulse(apr, mc_wr_rq_pulse); // 7-8
}

/*
 * Execute
 */

// 5-5
boolex(et4_inh) {
    return apr->inst == BLT ||
            apr->inst == XCT ||
            apr->ex_ir_uuo ||
            apr->shift_op ||
            ar_sbr(apr) ||
            apr->ir_md ||
            ir_fpch(apr);
}

boolex(et5_inh) {
    return apr->ir_iot || ir_fsb(apr);
}

// 5-12
boolex(pc_p1_inh) {
    return  key_execute(apr) ||
            ((ch_inc_op(apr) || ch_n_inc_op(apr)) && apr->inst != CAO) ||
            apr->inst == XCT ||
            apr->ex_ir_uuo ||
            apr->pi_cyc ||
            iot_blk(apr) ||
            apr->inst == BLT;
}

// 5-12
boolex(jfcl_flags) {
    return  (apr->ir & 0400 && apr->ar_ov_flag) ||
            (apr->ir & 0200 && apr->ar_cry0_flag) ||
            (apr->ir & 0100 && apr->ar_cry1_flag) ||
            (apr->ir & 0040 && apr->ar_pc_chg_flag);
}

boolex(pc_set_enable) {
    return (memac_ac(apr) && accp_et_al_test(apr)) ||
           (apr->inst == AOBJN && apr->ar & SGN) ||
           (apr->inst == AOBJP && !(apr->ar & SGN)) ||
           (apr->inst == JFCL && jfcl_flags(apr));
}

boolex(pc_inc_enable) {
    return ((memac_mem(apr) || accp(apr) || apr->ir_acbm) && accp_et_al_test(apr)) || 
            (iot_conso(apr) && apr->ar != 0) ||
            (iot_consz(apr) && apr->ar == 0);
}

boolex(pc_set) {
    return pc_set_enable(apr) || jp_jmp(apr) || apr->ir_jrst;
}

boolex(pc_inc_et9) {
    return apr->inst == JSR || apr->inst == JSA || pc_inc_enable(apr);
}

// 5-5
boolex(e_long) {
    return  iot_consz(apr) ||
            apr->ir_jp ||
            apr->ir_acbm ||
            apr->pc_set ||
            mb_pc_sto(apr) ||
            pc_inc_et9(apr) ||
            iot_conso(apr) ||
            ir_accp_memac(apr);
}


pulse(et10) {
    bool sc_e_c = sc_e(apr);

    if (pi_hold(apr)) {
        // 8-4
        apr->pi_ov = 0;
        apr->pi_cyc = 0;
    }
    if (apr->inst == PUSH || apr->inst == PUSHJ || apr->ir_jrst) {
        apr->ma |= apr->mb & RT;            // 7-3
    }
    if ((apr->fc_e_pse || sc_e_c) &&
           !(apr->ir_jp || apr->inst == EXCH || ch_dep(apr))) {
        apr->mb = apr->ar;                  // 6-3
    }
    if (apr->ir_jp && !(apr->ir & H6)) {
        swap(&apr->mb, &apr->ar);           // 6-3
    }
    if (memac(apr) || apr->ir_as) {
        // 6-10
        apr->ar_cry0_flag = apr->ar_cry0;
        apr->ar_cry1_flag = apr->ar_cry1;
    }

    if ((apr->ir_fwt && !apr->ar_cry0 && apr->ar_cry1) ||
           ((memac(apr) || apr->ir_as) && ar_ov_set(apr))) {
        apr->ar_ov_flag = 1;                // 6-10
        apr_recalc_cpa_req(apr);
    }
    if (apr->ir_jp && !(apr->ir & H6) && apr->ar_cry0) {
        apr->cpa_pdl_ov = 1;                // 8-5
        apr_recalc_cpa_req(apr);
    }
    if (apr->inst == JFCL)
        ar_jfcl_clr(apr);                   // 6-10
    // 5-6
    if (sc_e_c)
        nextpulse(apr, st1);
    else if (apr->fc_e_pse)
        nextpulse(apr, st2);
    else
        nextpulse(apr, st3);
}

pulse(et9) {
    bool pc_inc;

    pc_inc = pc_inc_et9(apr);
    if (pc_inc)
        apr->pc = (apr->pc+1) & RT;   // 5-12
    if ((apr->pc_set || pc_inc) && !(apr->ir_jrst && apr->ir & H11)) {
        apr->ar_pc_chg_flag = 1;    // 6-10
        apr_recalc_cpa_req(apr);
    }

    if (apr->ir_acbm || (apr->ir_jp && apr->inst != JSR))
        swap(&apr->mb, &apr->ar);   // 6-3
    if (apr->inst == PUSH || apr->inst == PUSHJ || apr->ir_jrst)
        apr->ma = 0;                // 7-3
    if (apr->inst == JSR)
        apr->chf7 = 0;              // 6-19
    nextpulse(apr, et10);           // 5-5
}

pulse(et8) {
    if (apr->pc_set)
        apr->pc |= apr->ma;         // 5-12
    if (apr->inst == JSR)
        apr->ex_ill_op = 0;         // 5-13
    nextpulse(apr, et9);            // 5-5
}

pulse(et7) {
    if (apr->pc_set)
        apr->pc = 0;                // 5-12
    if (apr->inst == JSR && (apr->ex_pi_sync || apr->ex_ill_op))
        apr->ex_user = 0;           // 5-13
    if (apr->ir_acbm)
        apr->ar = ~apr->ar & FW;    // 6-8
    nextpulse(apr, et8);            // 5-5
}

pulse(et6) {
    if (mb_pc_sto(apr) || apr->inst == JSA)
        apr->mb |= apr->pc;         // 6-3
    if (acbm_cl(apr))
        apr->mb &= apr->ar;         // 6-3
    if (jp_flag_stor(apr)) {        // 6-4
        // 6-4
        if (apr->ar_ov_flag)     apr->mb |= F0;
        if (apr->ar_cry0_flag)   apr->mb |= F1;
        if (apr->ar_cry1_flag)   apr->mb |= F2;
        if (apr->ar_pc_chg_flag) apr->mb |= F3;
        if (apr->chf7)           apr->mb |= F4;
        if (apr->ex_user)        apr->mb |= F5;
    }
    if (iot_consz(apr) || iot_conso(apr))
        apr->ar &= apr->mb; // 6-8
    nextpulse(apr, et7);        // 5-5
}

pulse(et5) {
    if (mb_pc_sto(apr))
        apr->mb = 0;            // 6-3
    if (apr->ir_acbm)
        apr->ar = ~apr->ar & FW;    // 6-8
    apr->pc_set = pc_set(apr);
    if (e_long(apr))
        nextpulse(apr, et6);
    else
        nextpulse(apr, et10);    // 5-5
}

pulse(et4) {
    apr->et4_ar_pse = 0;        // 5-5
    if (iot_blk(apr))
        apr->ex_ill_op = 0; // 5-13

    // 6-8
    if (apr->ir_boole && (apr->ir_boole_op == 04 ||
                          apr->ir_boole_op == 010 ||
                          apr->ir_boole_op == 011 ||
                          apr->ir_boole_op == 014 ||
                          apr->ir_boole_op == 015 ||
                          apr->ir_boole_op == 016 ||
                          apr->ir_boole_op == 017))
        apr->ar = ~apr->ar & FW;
    if (hwt_lt(apr) || iot_cono(apr))
        apr->ar = CONS(apr->mb, apr->ar);
    if (hwt_rt(apr))
        apr->ar = CONS(apr->ar, apr->mb);
    if (hwt_lt_set(apr))
        apr->ar = CONS(~apr->ar, apr->ar);
    if (hwt_rt_set(apr))
        apr->ar = CONS(apr->ar, ~apr->ar);

    if (fwt_swap(apr) || iot_blk(apr) || apr->ir_acbm)
        swap(&apr->mb, &apr->ar);   // 6-3

    if (iot_blk(apr))
        nextpulse(apr, iot_t0);     // 8-1
    else if (apr->ir_iot)
        apr->iot_go = 1;        // 8-1

    if (ir_fsb(apr))
        nextpulse(apr, fat0);       // 6-22
    if (!et5_inh(apr))
        nextpulse(apr, et5);        // 5-5
}

pulse(et3) {
    if (apr->ex_ir_uuo) {
        // MBLT <- IR(1) (UUO T0) on 6-3
        apr->mb |= ((word)apr->ir&0777740) << 18;   // 6-1
        apr->ma |= F30;             // 7-3
        apr->uuo_f1 = 1;            // 5-10
        nextpulse(apr, mc_wr_rq_pulse);     // 7-8
    }

    if (apr->inst == POPJ || apr->inst == BLT)
        apr->ma = apr->mb & RT;     // 7-3

    // AR SBR, 6-9
    if (fwt_negate(apr) || ir_fsb(apr))
        nextpulse(apr, ar_negate_t0);
    if (ar_sub(apr))
        nextpulse(apr, ar_ast0);
    if (ar_add(apr))
        nextpulse(apr, ar_ast1);
    if (ar_m1(apr))
        nextpulse(apr, ar_pm1_t0);
    if (ar_p1(apr))
        nextpulse(apr, ar_pm1_t1);

    if (ir_fpch(apr) && !(apr->ir & H3) &&
       (apr->inst == 0130 || apr->inst == 0131 || !(apr->ir & H4) || !(apr->ir & H5)))
        nextpulse(apr, st7);        // 5-6
    if (apr->inst == BLT)
        nextpulse(apr, blt_t0);     // 6-18
    if (apr->shift_op)
        nextpulse(apr, sht1);       // 6-20
    if (apr->inst == FSC) {
        if (apr->ar & F0)
            nextpulse(apr, fst1);   // 6-19
        nextpulse(apr, fst0);       // 6-19
    }
    if (apr->inst == XCT)
        nextpulse(apr, xct_t0);     // 5-10
    if (ar_sbr(apr))
        apr->et4_ar_pse = 1;        // 5-5
    if (!et4_inh(apr))
        nextpulse(apr, et4);        // 5-5
}

pulse(et1) {
    if (apr->ex_ir_uuo) {
        apr->ex_ill_op = 1;         // 5-13
        apr->mb &= RT;              // 6-3
    }
    if (apr->ir_jrst && apr->ir & H12)
        apr->ex_mode_sync = 1;      // 5-13
    if (apr->ir_jrst && apr->ir & H11)
        ar_flag_set(apr);           // 6-10
    if (pi_rst(apr))
        clear_pih(apr);             // 8-4

    // 6-3
    if (apr->ir_acbm)
        apr->mb &= apr->ar;
    // 6-8
    if ((apr->ir_boole && (apr->ir_boole_op == 06   ||
                           apr->ir_boole_op == 011  ||
                           apr->ir_boole_op == 014)) ||
            acbm_com(apr))
        apr->ar ^= apr->mb;
    if (apr->ir_boole && (apr->ir_boole_op == 01  ||
                          apr->ir_boole_op == 02  ||
                          apr->ir_boole_op == 015 ||
                          apr->ir_boole_op == 016))
        apr->ar &= apr->mb;
    if ((apr->ir_boole && (apr->ir_boole_op == 03   ||
                           apr->ir_boole_op == 04   ||
                           apr->ir_boole_op == 07   ||
                           apr->ir_boole_op == 010  ||
                           apr->ir_boole_op == 013)) ||
            acbm_set(apr))
        apr->ar |= apr->mb;
    if (hwt_ar_0(apr) || iot_status(apr) || iot_datai(apr))
        apr->ar = 0;

    if (hwt_swap(apr) || fwt_swap(apr) || apr->inst == BLT)
        SWAPLTRT(apr->mb);

    if (apr->inst == POPJ || apr->ex_ir_uuo || apr->inst == BLT)
        apr->ma = 0;                // 7-3
    if (apr->shift_op && apr->mb & F18)
        nextpulse(apr, sht0);       // 6-20
    if (apr->inst == FSC && !(apr->ar & F0))
        sc_com(apr);                // 6-15
    nextpulse(apr, et3);
}

pulse(et0) {
    if (!pc_p1_inh(apr))
        apr->pc = (apr->pc+1) & RT;   // 5-12
    apr->ar_cry0 = 0;               // 6-10
    apr->ar_cry1 = 0;               // 6-10
    ar_cry(apr);
    if (apr->ir_jrst && apr->ir & H11)
        ar_flag_clr(apr);           // 6-10
    if (ch_inc_op(apr))
        nextpulse(apr, cht1);       // 6-19
    if (ch_n_inc_op(apr))
        nextpulse(apr, cht6);       // 6-19
    if (ch_load(apr))
        nextpulse(apr, lct0);       // 6-20
    if (ch_dep(apr))
        nextpulse(apr, dct0);       // 6-20
    if (ir_mul(apr))
        nextpulse(apr, mpt0);       // 6-12
    if (ir_fad(apr))
        nextpulse(apr, fat0);       // 6-22
    if (ir_fmp(apr))
        nextpulse(apr, fmt0);       // 6-22
    if (ir_fdv(apr))
        nextpulse(apr, fdt0);       // 6-22
    if (ir_div(apr)) {
        nextpulse(apr, ds_div_t0);  // 6-25
        if (apr->ir & H6) {         // DIV
            if (apr->ar & F0)       // 6-25
                nextpulse(apr, dst3);
            else
                nextpulse(apr, dst10);
        } else {                    // IDIV
            if (apr->ar & F0)       // 6-25
                nextpulse(apr, dst0);
            else
                nextpulse(apr, dst1);
        }
    }
}

pulse(et0a) {
    static int gen = 0;
    debug_print(apr->emu, "%o: ", apr->pc);
    if ((apr->inst & 0700) != 0700)
        debug_print(apr->emu, "%d %s\n", gen++, insnames[apr->inst]);
    else
        debug_print(apr->emu, "%d %s\n", gen++, ionames[apr->io_inst>>5 & 7]);

    if (pi_hold(apr))
        set_pih(apr, apr->pi_req);  // 8-3, 8-4
    if (apr->key_ex_sync)
        apr->key_ex_st = 1;         // 5-1
    if (apr->key_dep_sync)
        apr->key_dep_st = 1;        // 5-1
    if (apr->key_inst_stop ||
           (apr->ir_jrst && (apr->ir & H10) && !apr->ex_user))
        apr->run = 0;               // 5-1

    if (apr->ir_boole && (apr->ir_boole_op == 00  ||
                          apr->ir_boole_op == 03  ||
                          apr->ir_boole_op == 014 ||
                          apr->ir_boole_op == 017))
        apr->ar = 0;                // 6-8
    if (apr->ir_boole && (apr->ir_boole_op == 02  ||
                          apr->ir_boole_op == 04  ||
                          apr->ir_boole_op == 012 ||
                          apr->ir_boole_op == 013 ||
                          apr->ir_boole_op == 015))
        apr->ar = ~apr->ar & FW;    // 6-8
    if (apr->fwt_00 ||
            apr->fwt_11 ||
            apr->hwt_11 ||
            memac_mem(apr) ||
            iot_blk(apr) ||
            iot_datao(apr)) {
        apr->ar = apr->mb;          // 6-8
    }
    if (apr->fwt_01 || apr->fwt_10 || iot_status(apr))
        apr->mb = apr->ar;          // 6-3
    if (apr->hwt_10 ||
            apr->inst == JSP ||
            apr->inst == EXCH || 
            apr->inst == BLT ||
            ir_fsb(apr)) {
        swap(&apr->mb, &apr->ar);   // 6-3
    }
    if (apr->inst == POP || apr->inst == POPJ || apr->inst == JRA)
        apr->mb = apr->mq;          // 6-3
    if (acbm_swap(apr) || iot_cono(apr) || apr->inst == JSA)
        SWAPLTRT(apr->mb);          // 6-3
    if (apr->inst == FSC || apr->shift_op)
        apr->sc |= (~apr->mb & 0377) | (~apr->mb>>9 & 0400); // 6-15
}

/*
 * Fetch
 *
 * After this stage we have:
 * AR = 0,E or (AC)
 * MQ = 0; (AC+1) or ((AC)LT|RT) if fetched
 * MB = [0?],E or (E)
 */

// 5-4
boolex(fac_inh) {
   return   apr->hwt_11 ||
            apr->fwt_00 ||
            apr->fwt_01 ||
            apr->fwt_11 ||
            (apr->inst == XCT) ||
            apr->ex_ir_uuo ||
            (apr->inst == JSP) ||
            (apr->inst == JSR) ||
            apr->ir_iot ||
            ir_254_7(apr) ||
            memac_mem(apr) ||
            ch_load(apr) ||
            ch_inc_op(apr) ||
            ch_n_inc_op(apr);
}

boolex(fac2) {
    return sh_ac2(apr) || ir_md_fac2(apr);
}

boolex(fc_c_acrt) {
    return (apr->inst == POP) ||
           (apr->inst == POPJ);
}

boolex(fc_c_aclt) {
    return (apr->inst == JRA) ||
           (apr->inst == BLT);
}

boolex(fc_e) {
    return  apr->hwt_00 ||
            apr->fwt_00 ||
            (apr->inst == XCT) ||
            (apr->inst == PUSH) ||
            iot_datao(apr) ||
            apr->ir_fp ||
            ir_md_fc_e(apr) ||
            ch_load(apr) ||
            ch_n_inc_op(apr) ||
            accp_dir(apr) ||
            acbm_dir(apr) ||
            apr->boole_as_00;
}

boolex(fc_e_pse) {
    return  apr->hwt_10 ||
            apr->hwt_11 ||
            apr->fwt_11 ||
            iot_blk(apr) ||
            (apr->inst == EXCH) ||
            ch_dep(apr) ||
            ch_inc_op(apr) ||
            memac_mem(apr) ||
            apr->boole_as_10 ||
            apr->boole_as_11;
}

pulse(ft7) {
    apr->f6a = 1;                       // 5-4
    if (apr->mc_split_cyc_sync)         // 7-8
        nextpulse(apr, mc_split_rd_rq);
    else
        nextpulse(apr, mc_rdwr_rq_pulse);
}

pulse(ft6a) {
    apr->f6a = 0;               // 5-4
    nextpulse(apr, et0a);       // 5-5
    nextpulse(apr, et0);        // 5-5
    nextpulse(apr, et1);        // 5-5
}

pulse(ft6) {
    apr->f6a = 1;                   // 5-4
    nextpulse(apr, mc_rd_rq_pulse); // 7-8
}

pulse(ft5) {
    // cache this because we need it again in ET10
    apr->fc_e_pse = fc_e_pse(apr);
    apr->ma = apr->mb & RT;     // 7-3
    // 5-4
    if (fc_e(apr))
        nextpulse(apr, ft6);
    else if (apr->fc_e_pse)
        nextpulse(apr, ft7);
    else
        nextpulse(apr, ft6a);
}

pulse(ft4a) {
    apr->f4a = 0;               // 5-4
    apr->ma = 0;                // 7-3
    swap(&apr->mb, &apr->mq);   // 6-3, 6-13
    nextpulse(apr, ft5);        // 5-4
}

pulse(ft4) {
    apr->mq = apr->mb;              // 6-13
    apr->f4a = 1;                   // 5-4
    nextpulse(apr, mc_rd_rq_pulse); // 7-8
}

pulse(ft3) {
    apr->ma = apr->mb & RT;     // 7-3
    swap(&apr->mb, &apr->ar);   // 6-3
    nextpulse(apr, ft4);        // 5-4
}

pulse(ft1a) {
    bool acltrt = fc_c_aclt(apr) || fc_c_acrt(apr);
    bool _fac2 = fac2(apr);
    apr->f1a = 0;                   // 5-4
    if (_fac2)
        apr->ma = (apr->ma+1) & 017;  // 7-1, 7-3
    else
        apr->ma = 0;                // 7-3
    if (!acltrt)
        swap(&apr->mb, &apr->ar);   // 6-3
    if (fc_c_aclt(apr))
        SWAPLTRT(apr->mb);          // 6-3
    // 5-4
    if (_fac2)
        nextpulse(apr, ft4);
    else if (acltrt)
        nextpulse(apr, ft3);
    else
        nextpulse(apr, ft5);
}

pulse(ft1) {
    apr->ma |= apr->ir>>5 & 017;    // 7-3
    apr->f1a = 1;           // 5-4
    nextpulse(apr, mc_rd_rq_pulse); // 7-8
}

pulse(ft0) {
    // 5-4
    if (fac_inh(apr))
        nextpulse(apr, ft5);
    else
        nextpulse(apr, ft1);
}

/*
 * Address
 */

pulse(at5) {
//  apr->a_long = 1;                // ?? nowhere to be found
    apr->af0 = 1;                   // 5-3
    apr->ma |= apr->mb & RT;        // 7-3
    apr->ir &= ~037;                // 5-7
    nextpulse(apr, mc_rd_rq_pulse); // 7-8
}

pulse(at4) {
    apr->ar &= ~LT;         // 6-8
    // TODO: what is MC DR SPLIT? what happens here anyway?
    if (apr->sw_addr_stop || apr->key_mem_stop)
        apr->mc_split_cyc_sync = 1; // 7-9
    // 5-3, 5-4
    if (apr->ir & 020)
        nextpulse(apr, at5);
    else
        nextpulse(apr, ft0);
}

pulse(at3a) {
    apr->af3a = 0;              // 5-3
    apr->mb = apr->ar;          // 6-3
    nextpulse(apr, at4);        // 5-3
}

pulse(at3) {
    apr->af3 = 0;               // 5-3
    apr->ma = 0;                // 7-3
    apr->af3a = 1;              // 5-3
    nextpulse(apr, ar_ast1);    // 6-9
}

pulse(at2) {
//  apr->a_long = 1;                // ?? nowhere to be found
    apr->ma |= apr->ir & 017;       // 7-3
    apr->af3 = 1;                   // 5-3
    nextpulse(apr, mc_rd_rq_pulse); // 7-8
}

pulse(at1) {
    apr->ex_uuo_sync = 1;           // 5-13
    // decode here because of EX UUO SYNC
    decode_ir(apr);
    // 5-3
    if ((apr->ir & 017) == 0)
        nextpulse(apr, at4);
    else
        nextpulse(apr, at2);
}

pulse(at0) {
    apr->ar &= ~RT;                 // 6-8
    apr->ar |= apr->mb & RT;        // 6-8
    apr->ir |= apr->mb>>18 & 037;   // 5-7
    apr->ma = 0;                    // 7-3
    apr->af0 = 0;                   // 5-3
    nextpulse(apr, pi_sync);        // 8-4
}

/*
 * Instruction
 */

pulse(it1a) {
    apr->if1a = 0;              // 5-3
    apr->ir |= apr->mb>>18 & 0777740;   // 5-7
    if (apr->ma & 0777760)
        set_key_rim_sbr(apr, 0);    // 5-2
    nextpulse(apr, at0);            // 5-3
}

pulse(it1) {
    if (apr->pi_cyc) {
        // 7-3, 8-4
        apr->ma |= 040;
        if (apr->pi_req & 0017) apr->ma |= 010;
        if (apr->pi_req & 0063) apr->ma |= 004;
        if (apr->pi_req & 0125) apr->ma |= 002;
    } else {
        apr->ma |= apr->pc;             // 7-3
    }
    if (apr->pi_ov)
        apr->ma = (apr->ma+1)&RT;       // 7-3
    apr->if1a = 1;                      // 5-3
    nextpulse(apr, mc_rd_rq_pulse);     // 7-8
}

pulse(iat0) {
    // have to call directly because PI CYC sets EX PI SYNC
    mr_clr(apr);                // 5-2
    set_pi_cyc(apr, 1);         // 8-4
    nextpulse(apr, it1);        // 5-3
}

pulse(it0) {
    apr->ma = 0;                // 7-3
    // have to call directly because IF1A is set with a delay
    mr_clr(apr);                // 5-2
    apr->if1a = 1;              // 5-3
    nextpulse(apr, pi_sync);    // 8-4
}

/*
 * Memory Control
 */

pulse(mai_addr_ack) {
    nextpulse(apr, mc_addr_ack);    // 7-8
}

pulse(mai_rd_rs) {
    Mem *mem = apr->emu->mem;
    /* we do this here instead of whenever MC RD is set; 7-6, 7-9 */
    apr->mb = mem->membus1;
    if (apr->ma == apr->mas)
        apr->mi = apr->mb;          // 7-7
    if (!apr->mc_stop)
        nextpulse(apr, mc_rs_t0);   // 7-8
}

pulse(mc_rs_t1) {
    set_mc_rd(apr, 0);          // 7-9
    if (apr->key_ex_nxt || apr->key_dep_nxt)
        apr->mi = apr->mb;      // 7-7

    if (apr->key_rd_wr) nextpulse(apr, key_rd_wr_ret);  // 5-2
    if (apr->sf7)       nextpulse(apr, st7);            // 5-6
    if (apr->sf5a)      nextpulse(apr, st5a);           // 5-6
    if (apr->sf3)       nextpulse(apr, st3);            // 5-6
    if (apr->f6a)       nextpulse(apr, ft6a);           // 5-4
    if (apr->f4a)       nextpulse(apr, ft4a);           // 5-4
    if (apr->f1a)       nextpulse(apr, ft1a);           // 5-4
    if (apr->af0)       nextpulse(apr, at0);            // 5-3
    if (apr->af3)       nextpulse(apr, at3);            // 5-3
    if (apr->if1a)      nextpulse(apr, it1a);           // 5-3
    if (apr->iot_f0a)   nextpulse(apr, iot_t0a);        // 8-1
    if (apr->blt_f0a)   nextpulse(apr, blt_t0a);        // 6-18
    if (apr->blt_f3a)   nextpulse(apr, blt_t3a);        // 6-18
    if (apr->blt_f5a)   nextpulse(apr, blt_t5a);        // 6-18
    if (apr->uuo_f1)    nextpulse(apr, uuo_t1);         // 5-10
    if (apr->chf6)      nextpulse(apr, cht8b);          // 6-19
}

pulse(mc_rs_t0) {
//  apr->mc_stop = 0;               // ?? not found on 7-9
    nextpulse(apr, mc_rs_t1);       // 7-8
}

pulse(mc_wr_rs) {
    Mem *mem = apr->emu->mem;
    if (apr->ma == apr->mas)
        apr->mi = apr->mb;          // 7-7
    mem->membus1 = apr->mb;              // 7-8
    mem->membus0 |= MEMBUS_WR_RS;   // 7-8
    if (!apr->mc_stop)
        nextpulse(apr, mc_rs_t0);   // 7-8
}

pulse(mc_addr_ack) {
    set_mc_rq(apr, 0);              // 7-9
    if (!apr->mc_rd && apr->mc_wr)
        nextpulse(apr, mc_wr_rs);   // 7-8
}

pulse(mc_non_exist_rd) {
    if (!apr->mc_stop)
        nextpulse(apr, mc_rs_t0);   // 7-8
}

pulse(mc_non_exist_mem_rst) {
    nextpulse(apr, mc_addr_ack);            // 7-8
    if (apr->mc_rd)
        nextpulse(apr, mc_non_exist_rd);    // 7-9
}

pulse(mc_non_exist_mem) {
    apr->cpa_non_exist_mem = 1;         // 8-5
    apr_recalc_cpa_req(apr);
    if (!apr->sw_mem_disable)
        nextpulse(apr, mc_non_exist_mem_rst);   // 7-9
}

pulse(mc_illeg_address) {
    apr->cpa_illeg_op = 1;              // 8-5
    apr_recalc_cpa_req(apr);
    nextpulse(apr, st7);                // 5-6
}

pulse(mc_stop_1) {
    apr->mc_stop = 1;       // 7-9
    if (apr->key_mem_cont)
        nextpulse(apr, kt4);    // 5-2
}

pulse(mc_rq_pulse) {
    bool ma_ok;
    /* have to call this to set flags, do relocation and set address */
    ma_ok = relocate(apr);
    apr->mc_stop = 0;                       // 7-9
    /* hack to catch non-existent memory */
    apr->extpulse |= 4;
    if (ma_ok || apr->ex_inh_rel)
        set_mc_rq(apr, 1);                  // 7-9
    else
        nextpulse(apr, mc_illeg_address);   // 7-9
    if (apr->key_mem_stop || (apr->ma == apr->mas && apr->sw_addr_stop))
        nextpulse(apr, mc_stop_1);          // 7-9
}

pulse(mc_rdwr_rq_pulse) {
    set_mc_rd(apr, 1);      // 7-9
    set_mc_wr(apr, 1);      // 7-9
    apr->mb = 0;            // 7-8
    apr->mc_stop_sync = 1;      // 7-9
    nextpulse(apr, mc_rq_pulse);    // 7-8
}

pulse(mc_rd_rq_pulse) {
    set_mc_rd(apr, 1);      // 7-9
    set_mc_wr(apr, 0);      // 7-9
    apr->mb = 0;            // 7-8
    nextpulse(apr, mc_rq_pulse);    // 7-8
}

pulse(mc_split_rd_rq) {
    nextpulse(apr, mc_rd_rq_pulse); // 7-8
}

pulse(mc_wr_rq_pulse) {
    set_mc_rd(apr, 0);      // 7-9
    set_mc_wr(apr, 1);      // 7-9
    nextpulse(apr, mc_rq_pulse);    // 7-8
}

pulse(mc_split_wr_rq) {
    nextpulse(apr, mc_wr_rq_pulse); // 7-8
}

pulse(mc_rd_wr_rs_pulse) {
    nextpulse(apr, apr->mc_split_cyc_sync ? mc_split_wr_rq : mc_wr_rs); // 7-8
}

/*
 * Keys
 */

pulse(key_rd_wr_ret) {
    apr->key_rd_wr = 0; // 5-2
//  apr->ex_ill_op = 0; // ?? not found on 5-13
    nextpulse(apr, kt4);    // 5-2
}

pulse(key_rd) {
    apr->key_rd_wr = 1;     // 5-2
    nextpulse(apr, mc_rd_rq_pulse); // 7-8
}

pulse(key_wr) {
    apr->key_rd_wr = 1;     // 5-2
    apr->mb = apr->ar;      // 6-3
    nextpulse(apr, mc_wr_rq_pulse); // 7-8
}

pulse(key_go) {
    apr->run = 1;       // 5-1
    apr->key_ex_st = 0; // 5-1
    apr->key_dep_st = 0;    // 5-1
    apr->key_ex_sync = 0;   // 5-1
    apr->key_dep_sync = 0;  // 5-1
    nextpulse(apr, it0);    // 5-3
}

pulse(kt4) {
    if (apr->run && (apr->key_ex_st || apr->key_dep_st))
        nextpulse(apr, key_go);     // 5-2
    // TODO check repeat switch
}

pulse(kt3) {
    if (apr->key_readin || apr->key_start)
        apr->pc |= apr->ma;         // 5-12
    if (key_execute(apr)) {
        apr->mb = apr->ar;          // 6-3
        nextpulse(apr, it1a);       // 5-3
        nextpulse(apr, kt4);        // 5-2
    }
    if (key_ex_ex_nxt(apr))
        nextpulse(apr, key_rd);     // 5-2
    if (key_dp_dp_nxt(apr))
        nextpulse(apr, key_wr);     // 5-2
    if (apr->key_start || apr->key_readin || apr->key_inst_cont)
        nextpulse(apr, key_go);     // 5-2
}

pulse(kt2) {
    if (key_ma_mas(apr))
        apr->ma |= apr->mas;        // 7-1
    if (key_execute_dp_dpnxt(apr))
        apr->ar |= apr->data;       // 5-2
    nextpulse(apr, kt3);            // 5-2
}

pulse(kt1) {
    if (apr->key_io_reset)
        nextpulse(apr, mr_start);   // 5-2
    if (key_manual_cond(apr) && !apr->key_mem_cont)
        nextpulse(apr, mr_clr);     // 5-2
    if (key_clr_rim(apr))
        set_key_rim_sbr(apr, 0);    // 5-2
    if (apr->key_mem_cont && apr->mc_stop)
        nextpulse(apr, mc_rs_t0);   // 7-8
    if (key_manual_cond(apr) && apr->mc_stop && apr->mc_stop_sync && !apr->key_mem_cont)
        nextpulse(apr, mc_wr_rs);   // 7-8

    if (apr->key_readin)
        set_key_rim_sbr(apr, 1);    // 5-2
    if (apr->key_readin || apr->key_start)
        apr->pc = 0;                // 5-12
    if (key_ma_mas(apr))
        apr->ma = 0;                // 5-2
    if (apr->key_ex_nxt  || apr->key_dep_nxt)
        apr->ma = (apr->ma+1)&RT;   // 5-2
    if (key_execute_dp_dpnxt(apr))
        apr->ar = 0;                // 5-2

    nextpulse(apr, kt2);            // 5-2
}

pulse(kt0a) {
    apr->key_ex_st = 0;                 // 5-1
    apr->key_dep_st = 0;                // 5-1
    apr->key_ex_sync = apr->key_ex;     // 5-1
    apr->key_dep_sync = apr->key_dep;   // 5-1
    if (!apr->run || apr->key_mem_cont)
        nextpulse(apr, kt1);            // 5-2
}

pulse(kt0) {
    nextpulse(apr, kt0a); // 5-2
}

pulse(key_manual) {
    nextpulse(apr, kt0); // 5-2
}

/* APR infrastructure */

void nextpulse(Apr *apr, Pulse *p) {
    if (apr->nnextpulses >= nelem(apr->pulses1))
        apr->emulation_error = APR_ERR_TOO_MANY_PULSES;
    else
        apr->nlist[apr->nnextpulses++] = p;
}

void apr_cycle(Emu *emu) {
    Apr *apr = emu->apr;
    Mem *mem = emu->mem;

    apr->ncurpulses = apr->nnextpulses;
    apr->nnextpulses = 0;

    Pulse **tmp = apr->clist;
    apr->clist = apr->nlist;
    apr->nlist = tmp;

    for(size_t i=0; i<apr->ncurpulses; i++)
        apr->clist[i](apr);

    /* KEY MANUAL */
    if (apr->extpulse & 1) {
        apr->extpulse &= ~1;
        /* FIXME BUG: without a print somewhere the thing doesn't work :(
         *            ^--wat Ô_ō */
        trace("KEY MANUAL\n");
        nextpulse(apr, key_manual);
    }

    /* KEY INST STOP */
    if (apr->extpulse & 2) {
        apr->extpulse &= ~2;
        apr->run = 0;
        /* hack: cleared when the pulse list was empty */
        apr->ia_inh = 1;
    }


    /* This is simplified, we have no IOT RESET, IOT INIT SET UP or
     * IOT FINAL SETUP really */
    if (apr->iot_go)
        nextpulse(apr, iot_t2);

    /* pulse and signals through IO bus */
    if (emu->iobus1 & (IOBUS_PULSES | IOBUS_IOB_STATUS | IOBUS_IOB_DATAI)) {
        int dev = /* these bits are loosely packed, thus the hassle below. */
            !!(emu->iobus1 & IOBUS_IOS3_1)<<6 |
            !!(emu->iobus1 & IOBUS_IOS4_1)<<5 |
            !!(emu->iobus1 & IOBUS_IOS5_1)<<4 |
            !!(emu->iobus1 & IOBUS_IOS6_1)<<3 |
            !!(emu->iobus1 & IOBUS_IOS7_1)<<2 |
            !!(emu->iobus1 & IOBUS_IOS8_1)<<1 |
            !!(emu->iobus1 & IOBUS_IOS9_1);
        IoWake *wake = &emu->iobusmap[dev];
        if (wake->func)
            wake->func(wake->arg);
        // TODO: clear IOB STATUS and IOB DATAI too?
        emu->iobus1 &= ~IOBUS_PULSES;
    }

    if (emu->iobus1 & IOBUS_IOB_RESET) {
        for(int d = 0; d < nelem(emu->iobusmap); d++) {
            IoWake *wake = &emu->iobusmap[d];
            if (wake->func)
                wake->func(wake->arg);
        }
        emu->iobus1 &= ~IOBUS_IOB_RESET;
    }

    /* Pulses to memory */
    if (mem->membus0 & (MEMBUS_WR_RS | MEMBUS_RQ_CYC)) {
        if (mem_wake(emu->mem)) {
            /* FIXME handle memory error */
        }
        /* Normally this should still be asserted but it is interpreted as a
         * pulse every loop iteration here. Clearing it is a hack */
        mem->membus0 &= ~MEMBUS_RQ_CYC;
    }

    /* Pulses from memory */
    if (mem->membus0 & MEMBUS_MAI_ADDR_ACK) {
        mem->membus0 &= ~MEMBUS_MAI_ADDR_ACK;
        apr->extpulse &= ~4;
        nextpulse(apr, mai_addr_ack);
    }

    if (mem->membus0 & MEMBUS_MAI_RD_RS) {
        mem->membus0 &= ~MEMBUS_MAI_RD_RS;
        nextpulse(apr, mai_rd_rs);
    }

    if (apr->extpulse & 4) {
        apr->extpulse &= ~4;
        if (apr->mc_rq && !apr->mc_stop)
            nextpulse(apr, mc_non_exist_mem);   // 7-9
    }

    /* FIXME what of this is obsolete and can be removed? */
    //if (!i) {
        /* no longer needed */
        apr->ia_inh = 0;
    //}
}

static void *apr_handler_thread(void *argemu) {
    Emu *emu = (Emu *)argemu;
    Apr *apr = emu->apr;

    while (apr->sw_power) {
        if (apr->pulse_single_step) { /* FIXME */
            int c;
            while(c = getchar(), c != EOF && c != '\n')
                if (c == 'x')
                    apr->pulse_single_step = 0;
        }
        /* usleep(50000); */

        apr_cycle(emu);
    }

    debug_print(emu, "power off\n");
    return NULL;
}

void apr_poweron(Emu *emu) {
    Apr *apr = emu->apr;

    apr->sw_power = 1;
    pthread_create(&apr->thr, NULL, apr_handler_thread, emu);
}

Apr *apr_init(Emu *emu) {
    Apr *apr = malloc(sizeof(Apr));
    if (!apr)
        return NULL;

    memset(&apr, 0xff, sizeof(Apr));
    apr->emu = emu;

    apr->extpulse = 0;
    apr->emulation_error = APR_OK;

    apr->clist = apr->pulses1;
    apr->nlist = apr->pulses2;
    apr->ncurpulses = 0;
    apr->nnextpulses = 0;
    apr->ia_inh = 0;

    emu->iobusmap[0].func = wake_cpa;
    emu->iobusmap[0].arg  = apr;
    emu->iobusmap[1].func = wake_pi;
    emu->iobusmap[1].arg  = apr;

    nextpulse(apr, mr_pwr_clr);

    return apr;
}

char *insnames[0700] = {
    "UUO00", "UUO01", "UUO02", "UUO03",
    "UUO04", "UUO05", "UUO06", "UUO07",
    "UUO10", "UUO11", "UUO12", "UUO13",
    "UUO14", "UUO15", "UUO16", "UUO17",
    "UUO20", "UUO21", "UUO22", "UUO23",
    "UUO24", "UUO25", "UUO26", "UUO27",
    "UUO30", "UUO31", "UUO32", "UUO33",
    "UUO34", "UUO35", "UUO36", "UUO37",
    "UUO40", "UUO41", "UUO42", "UUO43",
    "UUO44", "UUO45", "UUO46", "UUO47",
    "UUO50", "UUO51", "UUO52", "UUO53",
    "UUO54", "UUO55", "UUO56", "UUO57",
    "UUO60", "UUO61", "UUO62", "UUO63",
    "UUO64", "UUO65", "UUO66", "UUO67",
    "UUO70", "UUO71", "UUO72", "UUO73",
    "UUO74", "UUO75", "UUO76", "UUO77",

    "XX100", "XX101", "XX102", "XX103",
    "XX104", "XX105", "XX106", "XX107",
    "XX110", "XX111", "XX112", "XX113",
    "XX114", "XX115", "XX116", "XX117",
    "XX120", "XX121", "XX122", "XX123",
    "XX124", "XX125", "XX126", "XX127",
    "XX130", "XX131", "FSC", "CAO",
    "LDCI", "LDC", "DPCI", "DPC",
    "FAD", "FADL", "FADM", "FADB",
    "FADR", "FADLR", "FADMR", "FADBR",
    "FSB", "FSBL", "FSBM", "FSBB",
    "FSBR", "FSBLR", "FSBMR", "FSBBR",
    "FMP", "FMPL", "FMPM", "FMPB",
    "FMPR", "FMPLR", "FMPMR", "FMPBR",
    "FDV", "FDVL", "FDVM", "FDVB",
    "FDVR", "FDVLR", "FDVMR", "FDVBR",

    "MOVE", "MOVEI", "MOVEM", "MOVES",
    "MOVS", "MOVSI", "MOVSM", "MOVSS",
    "MOVN", "MOVNI", "MOVNM", "MOVNS",
    "MOVM", "MOVMI", "MOVMM", "MOVMS",
    "IMUL", "IMULI", "IMULM", "IMULB",
    "MUL", "MULI", "MULM", "MULB",
    "IDIV", "IDIVI", "IDIVM", "IDIVB",
    "DIV", "DIVI", "DIVM", "DIVB",
    "ASH", "ROT", "LSH", "XX243",
    "ASHC", "ROTC", "LSHC", "XX247",
    "EXCH", "BLT", "AOBJP", "AOBJN",
    "JRST", "JFCL", "XCT", "XX257",
    "PUSHJ", "PUSH", "POP", "POPJ",
    "JSR", "JSP", "JSA", "JRA",
    "ADD", "ADDI", "ADDM", "ADDB",
    "SUB", "SUBI", "SUBM", "SUBB",

    "CAI", "CAIL", "CAIE", "CAILE",
    "CAIA", "CAIGE", "CAIN", "CAIG",
    "CAM", "CAML", "CAME", "CAMLE",
    "CAMA", "CAMGE", "CAMN", "CAMG",
    "JUMP", "JUMPL", "JUMPE", "JUMPLE",
    "JUMPA", "JUMPGE", "JUMPN", "JUMPG",
    "SKIP", "SKIPL", "SKIPE", "SKIPLE",
    "SKIPA", "SKIPGE", "SKIPN", "SKIPG",
    "AOJ", "AOJL", "AOJE", "AOJLE",
    "AOJA", "AOJGE", "AOJN", "AOJG",
    "AOS", "AOSL", "AOSE", "AOSLE",
    "AOSA", "AOSGE", "AOSN", "AOSG",
    "SOJ", "SOJL", "SOJE", "SOJLE",
    "SOJA", "SOJGE", "SOJN", "SOJG",
    "SOS", "SOSL", "SOSE", "SOSLE",
    "SOSA", "SOSGE", "SOSN", "SOSG",

    "SETZ", "SETZI", "SETZM", "SETZB",
    "AND", "ANDI", "ANDM", "ANDB",
    "ANDCA", "ANDCAI", "ANDCAM", "ANDCAB",
    "SETM", "SETMI", "SETMM", "SETMB",
    "ANDCM", "ANDCMI", "ANDCMM", "ANDCMB",
    "SETA", "SETAI", "SETAM", "SETAB",
    "XOR", "XORI", "XORM", "XORB",
    "IOR", "IORI", "IORM", "IORB",
    "ANDCB", "ANDCBI", "ANDCBM", "ANDCBB",
    "EQV", "EQVI", "EQVM", "EQVB",
    "SETCA", "SETCAI", "SETCAM", "SETCAB",
    "ORCA", "ORCAI", "ORCAM", "ORCAB",
    "SETCM", "SETCMI", "SETCMM", "SETCMB",
    "ORCM", "ORCMI", "ORCMM", "ORCMB",
    "ORCB", "ORCBI", "ORCBM", "ORCBB",
    "SETO", "SETOI", "SETOM", "SETOB",

    "HLL", "HLLI", "HLLM", "HLLS",
    "HRL", "HRLI", "HRLM", "HRLS",
    "HLLZ", "HLLZI", "HLLZM", "HLLZS",
    "HRLZ", "HRLZI", "HRLZM", "HRLZS",
    "HLLO", "HLLOI", "HLLOM", "HLLOS",
    "HRLO", "HRLOI", "HRLOM", "HRLOS",
    "HLLE", "HLLEI", "HLLEM", "HLLES",
    "HRLE", "HRLEI", "HRLEM", "HRLES",
    "HRR", "HRRI", "HRRM", "HRRS",
    "HLR", "HLRI", "HLRM", "HLRS",
    "HRRZ", "HRRZI", "HRRZM", "HRRZS",
    "HLRZ", "HLRZI", "HLRZM", "HLRZS",
    "HRRO", "HRROI", "HRROM", "HRROS",
    "HLRO", "HLROI", "HLROM", "HLROS",
    "HRRE", "HRREI", "HRREM", "HRRES",
    "HLRE", "HLREI", "HLREM", "HLRES",

    "TRN", "TLN", "TRNE", "TLNE",
    "TRNA", "TLNA", "TRNN", "TLNN",
    "TDN", "TSN", "TDNE", "TSNE",
    "TDNA", "TSNA", "TDNN", "TSNN",
    "TRZ", "TLZ", "TRZE", "TLZE",
    "TRZA", "TLZA", "TRZN", "TLZN",
    "TDZ", "TSZ", "TDZE", "TSZE",
    "TDZA", "TSZA", "TDZN", "TSZN",
    "TRC", "TLC", "TRCE", "TLCE",
    "TRCA", "TLCA", "TRCN", "TLCN",
    "TDC", "TSC", "TDCE", "TSCE",
    "TDCA", "TSCA", "TDCN", "TSCN",
    "TRO", "TLO", "TROE", "TLOE",
    "TROA", "TLOA", "TRON", "TLON",
    "TDO", "TSO", "TDOE", "TSOE",
    "TDOA", "TSOA", "TDON", "TSON",
};

char *ionames[] = {
    "BLKI",
    "DATAI",
    "BLKO",
    "DATAO",
    "CONO",
    "CONI",
    "CONSZ",
    "CONSO"
};

void testinst(Apr *apr) {
    int inst;

    for (inst = 0; inst < 0700; inst++) {
//  for(inst = 0140; inst < 0141; inst++) {
        apr->ir = inst << 9 | 1 << 5;
        decode_ir(apr);
        printf("%06o %6s ", apr->ir, insnames[inst]);
/*
        print("%s ", FAC_INH ? "FAC_INH" : "       ");
        print("%s ", FAC2 ? "FAC2" : "    ");
        print("%s ", FC_C_ACRT ? "FC_C_ACRT" : "         ");
        print("%s ", FC_C_ACLT ? "FC_C_ACLT" : "         ");
        print("%s ", FC_E ? "FC_E" : "    ");
*/
        printf("%s ", fc_e_pse(apr) ? "FC_E_PSE" : "        ");
        printf("%s ", sc_e(apr) ? "SC_E" : "    ");
        printf("%s ", sac_inh(apr) ? "SAC_INH" : "       ");
        printf("%s ", sac2(apr) ? "SAC2" : "    ");
        printf("\n");
// FC_E_PSE
//print("FC_E_PSE: %d %d %d %d %d %d %d %d %d %d\n", apr->hwt_10 , apr->hwt_11 , apr->fwt_11 ,
//                  IOT_BLK , apr->inst == EXCH , CH_DEP , CH_INC_OP ,
//                  MEMAC_MEM , apr->boole_as_10 , apr->boole_as_11);
//print("CH: %d %d %d %d %d\n", CH_INC, CH_INC_OP, CH_N_INC_OP, CH_LOAD, CH_DEP);
//print("FAC_INH: %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
//  apr->hwt_11 , apr->fwt_00 , apr->fwt_01 , apr->fwt_11 ,
//  apr->inst == XCT , apr->ex_ir_uuo ,
//  apr->inst == JSP , apr->inst == JSR ,
//  apr->ir_iot , IR_254_7 , MEMAC_MEM ,
//  CH_LOAD , CH_INC_OP , CH_N_INC_OP);
    }
}
