/*
 * TMS320C28X translation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "disas/disas.h"
#include "tcg-op.h"
#include "qemu/log.h"
#include "qemu/bitops.h"
#include "qemu/qemu-print.h"
#include "exec/cpu_ldst.h"
#include "exec/translator.h"

#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/gen-icount.h"

#include "trace-tcg.h"
#include "exec/log.h"

/* is_jmp field values */
#define DISAS_EXIT    DISAS_TARGET_0  /* force exit to main loop */
#define DISAS_JUMP    DISAS_TARGET_1  /* exit via jmp_pc/jmp_pc_imm */

typedef struct DisasContext {
    DisasContextBase base;
    uint32_t repeat_counter;
    // uint32_t mem_idx;
    // uint32_t tb_flags;
    // uint32_t delayed_branch;
    // uint32_t cpucfgr;
    // uint32_t avr;

    // /* If not -1, jmp_pc contains this value and so is a direct jump.  */
    // target_ulong jmp_pc_imm;

    // /* The temporary corresponding to register 0 for this compilation.  */
    // TCGv R0;
} DisasContext;

static TCGv cpu_acc; /* Accumulator todo:ah,al */
static TCGv cpu_xar[8]; /* Auxiliary todo:ar[8] */
static TCGv cpu_dp; /* Data-page pointer */
static TCGv cpu_ifr; /* Interrupt flag register */
static TCGv cpu_ier; /* Interrupt enable register */
static TCGv cpu_dbgier; /* Debug interrupt enable register */
static TCGv cpu_p; /* Product register todo:ph,pl*/
static TCGv cpu_pc;          /* Program counter 22bit, reset to 0x3f ffc0*/
static TCGv cpu_rpc;         /* Return program counte 22bit*/
static TCGv cpu_sp; /* Stack pointer, reset to 0x0400 */
static TCGv cpu_st0; /* Status register 0 */
static TCGv cpu_st1; /* Status register 1, reset to 0x080b */
static TCGv cpu_xt;         /* Multiplicand register, todo:t,tl*/


void tms320c28x_translate_init(void)
{
    static const char * const regnames[] = {
        "xar0", "xar1", "xar2", "xar3", "xar4", "xar5", "xar6", "xar7",
    };
    int i;

    cpu_acc = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, acc), "acc");
    cpu_dp = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, dp), "dp");
    cpu_ifr = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, ifr), "ifr");
    cpu_ier = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, ier), "ier");
    cpu_dbgier = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, dbgier), "dbgier");
    cpu_p = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, p), "p");
    cpu_pc = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, pc), "pc");
    cpu_rpc = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, rpc), "rpc");
    cpu_sp = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, sp), "sp");
    cpu_st0 = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, st0), "st0");
    cpu_st1 = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, st1), "st1");
    cpu_xt = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, xt), "xt");
                                
    for (i = 0; i < 8; i++) {
        cpu_xar[i] = tcg_global_mem_new(cpu_env,offsetof(CPUTms320c28xState,xar[i]),regnames[i]);
    }
}

#include "decode-status.c"
#include "decode-base.c"

#include "decode-mov.c"
#include "decode-math.c"
#include "decode-bitop.c"
#include "decode-branch.c"
#include "decode-interrupt.c"


static int decode(Tms320c28xCPU *cpu , DisasContext *ctx, uint16_t insn) 
{
    /* Set the default instruction length.  */
    int length = 2;
    uint32_t insn32 = insn;
    bool set_repeat_counter = false;
    // qemu_log_mask(CPU_LOG_TB_IN_ASM ,"insn is: 0x%x \n",insn);

    switch ((insn & 0xf000) >> 12) {
        case 0b0000:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0000: //0000 0000 .... ....
                    switch ((insn & 0x00c0) >> 6) {
                        case 0b00:
                            if(((insn >> 5) & 1) == 0) { //0000 0000 000. ....
                                if(((insn >> 4) & 1) == 0) { //0000 0000 0000 ....
                                    switch(insn & 0x000f) {
                                        case 0: //0000 0000 0000 0000
                                            break;
                                        case 1: //0000 0000 0000 0001, ABORTI P124
                                            gen_helper_aborti(cpu_env);
                                            break;
                                        case 2: //0000 0000 0000 0010, POP IFR
                                            gen_pop_ifr(ctx);
                                            break;
                                        case 3: //0000 0000 0000 0011 POP AR1H:AR0H
                                            gen_pop_ar1h_ar0h(ctx);
                                            break;
                                        case 4: //0000 0000 0000 0100 PUSH RPC
                                            gen_push_rpc(ctx);
                                            break;
                                        case 5: //0000 0000 0000 0101 PUSH AR1H:AR0H
                                            gen_push_ar1h_ar0h(ctx);
                                            break;
                                        case 6: //0000 0000 0000 0110 LRETR
                                            gen_lretr(ctx);
                                            break;
                                        case 7: //0000 0000 0000 0111 POP RPC
                                            gen_pop_rpc(ctx);
                                            break;
                                        default: //0000 0000 0000 1nnn, BANZ 16bitOffset,ARn--
                                            break;
                                    }
                                }
                                else { //0000 0000 0001 CCCC INTR INTx
                                    uint32_t n = (insn & 0xf) + 1;//int1 = 0
                                    gen_intr(ctx, cpu, n);
                                }
                            }
                            else {// 0000 0000 001C CCCC, TRAP #VectorNumber

                            }
                            break;
                        case 0b01: /*0000 0000 01.. .... LB 22bit */
                        {
                            uint32_t dest = ((insn32 & 0x3f)<< 16) | translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                            // dest = dest * 2;
                            gen_lb_22bit(ctx, dest);
                            length = 4;
                            break;
                        }
                        case 0b10: // 0000 0000 10CC CCCC, LC 22bit
                            break;
                        case 0b11: // 0000 0000 11CC CCCC, FFC XAR7,22bit
                            break;
                    }
                    break;
                case 0b0001:
                    break;
                case 0b0010:
                    break;
                case 0b0011:
                    break;
                case 0b0100:
                    break;
                case 0b0101:
                    break;
                case 0b0110: //0000 0110 .... .... MOVL ACC,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_acc_loc32(ctx, mode);
                    break;
                }
                case 0b0111:
                    break;
                case 0b1000:
                    break;
                case 0b1001:
                    break;
                case 0b1010:
                    break;
                case 0b1011:
                    break;
                case 0b1100:
                    break;
                case 0b1101:
                    break;
                case 0b1110:
                    break;
                case 0b1111:
                    break;
            }
            break;
        case 0b0001:
            switch ((insn & 0x0f00) >> 8) {
                case 0b1001: //0001 1001 CCCC CCCC SUBB ACC,#8bit
                {
                    int32_t imm = insn & 0xff;
                    gen_subb_acc_8bit(ctx, imm);
                    break;
                }
                case 0b1110: //0001 1110 .... .... MOVL loc32,ACC
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_acc(ctx, mode);
                    break;
                }
            }
            break;
        case 0b0010:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0000:
                    break;
                case 0b0001:
                    break;
                case 0b0010:
                    break;
                case 0b0011:
                    break;
                case 0b0100:
                    break;
                case 0b0101: /* 0010 0101 .... .... MOV ACC, loc16<<#16 */
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_acc_loc16_shift(ctx, mode, 16);
                }
                    break;
                case 0b0110:
                    break;
                case 0b0111:
                    break;
                case 0b1000: /* MOV loc16, #16bit p260 */
                {
                    uint32_t imm = translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                    uint32_t mode = insn & 0xff;
                    gen_mov_loc16_16bit(ctx, mode, imm);
                    length = 4;
                    break;
                }
                case 0b1001:
                    break;
                case 0b1010:
                    break;
                case 0b1011:
                    break;
                case 0b1100:
                    break;
                case 0b1101:
                    break;
                case 0b1110:
                    break;
                case 0b1111:
                    break;
            }
            break;
        case 0b0011:
            switch ((insn & 0xf00) >> 8) {
                case 0b1010: //0011 1010 LLLL LLLL MOVL loc32, XAR0
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 0);
                    break;
                }
            }
            break;
        case 0b0100:
            break;
        case 0b0101:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0110://0101 0110 .... ....
                    switch ((insn & 0x00f0) >> 4) {
                        case 0b0000: //0101 0110 0000 ....
                            switch (insn & 0x000f) {
                                case 0b0011: /* 0101 0110 0000 0011  MOV ACC, loc16<<#1...15 */
                                {
                                    uint32_t insn2 = translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                                    length = 4;
                                    if ((insn2 & 0xf000) == 0) { //this 4bit should be 0
                                        /* MOV ACC, loc16<<#1...15 */
                                        uint32_t shift = (insn2 & 0x0f00) >> 8;
                                        uint32_t mode = (insn2 & 0xff);
                                        gen_mov_acc_loc16_shift(ctx, mode, shift);
                                    }
                                    break;
                                }
                            }
                            break;
                        case 0b1100: //0101 0110 1100 COND  BF 16bitOffset,COND
                        {
                            int16_t offset = translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                            uint32_t cond = insn & 0xf;
                            gen_bf_16bitOffset_cond(ctx, offset, cond);
                            length = 4;
                            break;
                        }
                    }
                    break;
                case 0b0111:
                    break;
            }
            break;
        case 0b0110: //0110 COND CCCC CCCC SB 8bitOffset,COND
        {
            int16_t offset = insn & 0xff;
            if ((offset >> 7) == 1) {
                offset = offset | 0xff00;
            }
            uint32_t cond = (insn >> 8) & 0xf;
            gen_sb_8bitOffset_cond(ctx, offset, cond);
            break;
        }
        case 0b0111:
            switch((insn & 0x0f00) >> 8) {
                case 0b0110: //0111 0110 .... ....
                    if (((insn >> 7) & 0x1) == 1) { //0111 0110 1... .... MOVL XARn,#22bit 
                        uint32_t n = ((insn >> 6) & 0b11) + 6;//XAR6,XAR7
                        uint32_t imm = ((insn32 & 0x3f)<< 16) | translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                        gen_movl_xarn_22bit(ctx, n, imm);
                        length = 4;
                    }
                    else { // 0111 0110 0... ....
                        if (((insn >> 6) & 0x1) == 1) { //0111 0110 01.. .... LCR #22bit todo

                        }
                        else { //0111 0110 00.. ....
                            switch(insn & 0x3f) { 
                                case 0b011111: //0111 0110 0001 1111 MOVW DP,#16bit
                                {
                                    uint32_t imm = translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                                    gen_movw_dp_16bit(ctx, imm);
                                    length = 4;
                                    break;
                                }
                            }
                        }
                    }
                    break;
            }
            break;
        case 0b1000:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0101: /*1000 0101 .... .... MOV ACC, loc16<<0 */
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_acc_loc16_shift(ctx, mode, 0);
                }
                break;
                case 0b1101: /*1000 1101 .... .... MOVL XARn,#22bit */
                {
                    uint32_t n = (insn >> 6) & 0b11;
                    uint32_t imm = ((insn32 & 0x3f)<< 16) | translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                    gen_movl_xarn_22bit(ctx, n, imm);
                    length = 4;
                }
                break;
                case 0b1111:
                    switch((insn >> 7) & 1) {
                        case 0b0: //1000 1111 0.... .... MOVL XARn,#22bit
                        {
                            uint32_t n = ((insn >> 6) & 0b11) + 4;//XAR4,XAR5
                            uint32_t imm = ((insn32 & 0x3f)<< 16) | translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                            gen_movl_xarn_22bit(ctx, n, imm);
                            length = 4;
                            break;
                        }
                        case 0b1: //1000 1111 1.... ....
                            break;
                    }
                    break;
            }
            break;
        case 0b1001:
            switch ((insn >> 9) & 0b111) {
                case 0b001: //1001 001. .... ....  MOV AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    if (((insn >> 8) & 1) == 1){ //1001 0011 .... .... ah
                        gen_mov_ah_loc16(ctx, mode);
                    }
                    else { // al
                        gen_mov_al_loc16(ctx, mode);
                    }
                    break;
                }
                case 0b010: //1001 010. .... .... ADD AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    if (((insn >> 8) & 1) == 1){ //ah
                        gen_add_ah_loc16(ctx, mode);
                    }
                    else { // al
                        gen_add_al_loc16(ctx, mode);
                    }
                    break;
                }
                case 0b011: //1001 011. .... .... MOV loc16,AX
                {
                    uint32_t mode = insn & 0xff;
                    if (((insn >> 8) & 1) == 1){ // ah
                        gen_mov_loc16_ah(ctx, mode);
                    }
                    else { // al
                        gen_mov_loc16_al(ctx, mode);
                    }
                    break;
                }
            }
            break;
        case 0b1010:
            switch ((insn & 0xf00) >> 8) {
                case 0b0000: //1010 0000 LLLL LLLL MOVL loc32, XAR5
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 5);
                    break;
                }
                case 0b0010: //1010 0010 LLLL LLLL MOVL loc32, XAR3
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 3);
                    break;
                }
                case 0b1000: //1010 1000 LLLL LLLL MOVL loc32, XAR4
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 4);
                    break;
                }
                case 0b1010: //1010 1010 LLLL LLLL MOVL loc32, XAR2
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 2);
                    break;
                }
            }
            break;
        case 0b1011:
            switch ((insn & 0xf00) >> 8) {
                case 0b0010: //1011 0010 LLLL LLLL MOVL loc32, XAR1
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 1);
                    break;
                }
            }
            break;
        case 0b1100:
            switch ((insn & 0xf00) >> 8) {
                case 0b0010: //1100 0010 LLLL LLLL MOVL loc32, XAR6
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 6);
                    break;
                }
                case 0b0011: //1100 0011 LLLL LLLL MOVL loc32, XAR7
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 7);
                    break;
                }
            }
            break;
        case 0b1101:
            break;
        case 0b1110:
            break;
        case 0b1111:
            switch ((insn & 0x0f00) >> 8) { 
                case 0b1111://1111 1111 .... ....
                    switch ((insn & 0x00f0) >> 4) {
                        case 0b0010: /*1111 1111 0010 .... MOV ACC, #16bit<#0...15 */
                        {
                            uint32_t imm = translator_lduw_swap(&cpu->env, ctx->base.pc_next+2, true);
                            uint32_t shift = insn & 0x000f;
                            gen_mov_acc_16bit_shift(ctx, imm, shift);
                            length = 4;
                            break;
                        }
                    }
                    break;
            }
            break;
    }
    if (!set_repeat_counter) {
        ctx->repeat_counter = 0;
    }
    // 16bit -> 2; 32bit -> 4
    return length;
}

// Initialize the target-specific portions of DisasContext struct.
// The generic DisasContextBase has already been initialized.
static void tms320c28x_tr_init_disas_context(DisasContextBase *dcb, CPUState *cs)
{
    DisasContext *dc = container_of(dcb, DisasContext, base);
    // CPUTms320c28xState *env = cs->env_ptr;
    // int bound;
    dc->repeat_counter = 0;
}

// Emit any code required before the start of the main loop,
// after the generic gen_tb_start().
static void tms320c28x_tr_tb_start(DisasContextBase *db, CPUState *cs)
{
    // DisasContext *dc = container_of(db, DisasContext, base);
}

// Emit the tcg_gen_insn_start opcode.
static void tms320c28x_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    tcg_gen_insn_start(dc->base.pc_next, 0);
}


// When called, the breakpoint has already been checked to match the PC,
// but the target may decide the breakpoint missed the address
// (e.g., due to conditions encoded in their flags).  Return true to
// indicate that the breakpoint did hit, in which case no more breakpoints
// are checked.  If the breakpoint did hit, emit any code required to
// signal the exception, and set db->is_jmp as necessary to terminate
// the main loop.
static bool tms320c28x_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cs, const CPUBreakpoint *bp)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    tcg_gen_movi_tl(cpu_pc, dc->base.pc_next);
    // gen_exception(dc, EXCP_DEBUG);
    dc->base.is_jmp = DISAS_NORETURN;
    /* The address covered by the breakpoint must be included in
       [tb->pc, tb->pc + tb->size) in order to for it to be
       properly cleared -- thus we increment the PC here so that
       the logic setting tb->size below does the right thing.  */
    dc->base.pc_next += 2; // ?? 16bit op ==> 2
    return true;
}

static void tms320c28x_tr_translate_insn(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    // can not load 32bit here, "translator_ldl_swap" must load from 32bit aligned
    uint16_t insn = translator_lduw_swap(&cpu->env, dc->base.pc_next, true);

    int inc = decode(cpu, dc, insn);
    if (inc <= 0) {
        // gen_illegal_exception(dc);
    }
    else
        dc->base.pc_next += inc;
}

static void tms320c28x_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    target_ulong jmp_dest;

    /* If we have already exited the TB, nothing following has effect.  */
    if (dc->base.is_jmp == DISAS_NORETURN) {
        return;
    }

    if (dc->base.singlestep_enabled) {
        // gen_exception(dc, EXCP_DEBUG);
        return;
    }

    jmp_dest = dc->base.pc_next;

    switch (dc->base.is_jmp) {
    case DISAS_JUMP:
        tcg_gen_lookup_and_goto_ptr();
        break;

    case DISAS_TOO_MANY:
        if (unlikely(dc->base.singlestep_enabled)) {
            tcg_gen_movi_tl(cpu_pc, jmp_dest);
            // gen_exception(dc, EXCP_DEBUG);
        } else if ((dc->base.pc_first ^ jmp_dest) & TARGET_PAGE_MASK) {
            tcg_gen_movi_tl(cpu_pc, jmp_dest);
            tcg_gen_lookup_and_goto_ptr();
        } else {
            tcg_gen_goto_tb(0);
            tcg_gen_movi_tl(cpu_pc, jmp_dest);
            tcg_gen_exit_tb(dc->base.tb, 0);
        }
        break;

    case DISAS_EXIT:
        tcg_gen_exit_tb(NULL, 0);
        break;
    default:
        g_assert_not_reached();
    }
}

static void tms320c28x_tr_disas_log(const DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *s = container_of(dcbase, DisasContext, base);

    qemu_log("IN: %s\n", lookup_symbol(s->base.pc_first));
    log_target_disas(cs, s->base.pc_first, s->base.tb->size);
}

static const TranslatorOps tms320c28x_tr_ops = {
    .init_disas_context = tms320c28x_tr_init_disas_context,
    .tb_start           = tms320c28x_tr_tb_start,
    .insn_start         = tms320c28x_tr_insn_start,
    .breakpoint_check   = tms320c28x_tr_breakpoint_check,
    .translate_insn     = tms320c28x_tr_translate_insn,
    .tb_stop            = tms320c28x_tr_tb_stop,
    .disas_log          = tms320c28x_tr_disas_log,
};

void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    DisasContext ctx;

    translator_loop(&tms320c28x_tr_ops, &ctx.base, cs, tb, max_insns);
}

void tms320c28x_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    CPUTms320c28xState *env = &cpu->env;
    int i;

    qemu_fprintf(f, "PC =%08x RPC=%08x\n", env->pc, env->rpc);
    qemu_fprintf(f, "ACC=%08x AH =%04x AL=%04x\n", env->acc, env->acc >> 16, env->acc & 0xffff);
    for (i = 0; i < 8; ++i) {
        qemu_fprintf(f, "XAR%01d=%08x%c", i, env->xar[i], (i % 4) == 3 ? '\n' : ' ');
    }
    for (i = 0; i < 8; ++i) {
        qemu_fprintf(f, "AR%01d=%04x%c", i, env->xar[i] & 0xffff, (i % 4) == 3 ? '\n' : ' ');
    }
    qemu_fprintf(f, "DP =%04x IFR=%04x IER=%04x DBGIER=%04x\n", env->dp, env->ifr, env->ier, env->dbgier);
    qemu_fprintf(f, "P  =%08x PH =%04x PH =%04x\n", env->p, env->p >> 16, env->p & 0xffff);
    qemu_fprintf(f, "XT =%08x T  =%04x TL =%04x\n", env->xt, env->xt >> 16, env->xt & 0xffff);
    qemu_fprintf(f, "SP =%04x ST0=%04x ST1=%04x\n", env->sp, env->st0, env->st1);
    qemu_fprintf(f, "OVC=%x PM=%x V=%x N=%x Z=%x\n", cpu_get_ovm(env), cpu_get_pm(env), cpu_get_v(env), cpu_get_n(env), cpu_get_z(env));
    qemu_fprintf(f, "C=%x TC=%x OVM=%x SXM=%x\n", cpu_get_c(env), cpu_get_tc(env), cpu_get_ovm(env), cpu_get_sxm(env));
    qemu_fprintf(f, "ARP=%x XF=%x MOM1MAP=%x OBJMODE=%x\n", cpu_get_arp(env), cpu_get_xf(env), cpu_get_mom1map(env), cpu_get_objmode(env));
    qemu_fprintf(f, "AMODE=%x IDLESTAT=%x EALLOW=%x LOOP=%x\n", cpu_get_amode(env), cpu_get_idlestat(env), cpu_get_eallow(env), cpu_get_loop(env));
    qemu_fprintf(f, "SPA=%x VMAP=%x PAGE0=%x DBGM=%x INTM=%x\n", cpu_get_spa(env), cpu_get_vmap(env), cpu_get_page0(env), cpu_get_dbgm(env), cpu_get_intm(env));
}

void restore_state_to_opc(CPUTms320c28xState *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->pc = data[0];
    // env->dflag = data[1] & 1;
    // if (data[1] & 2) {
    //     env->ppc = env->pc - 4;
    // }
}
