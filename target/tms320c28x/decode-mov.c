// DMOV loc16
static void gen_dmov_loc16(DisasContext *ctx, uint32_t mode)
{
    //mode = reg addressing is illegal
    if (mode >= 160 && mode <=173) {
        gen_exception(ctx, EXCP_INTERRUPT_ILLEGAL);
    }
    else
    {
        TCGv a = tcg_temp_local_new();
        TCGv addr = tcg_temp_local_new();
        TCGLabel *begin = gen_new_label();
        TCGLabel *end = gen_new_label();
        gen_set_label(begin);

        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr);
        tcg_gen_addi_i32(addr, addr, 1);
        gen_st16u_swap(a, addr);

        tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
        tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
        tcg_gen_br(begin);
        gen_set_label(end);

        tcg_temp_free(a);
        tcg_temp_free(addr);
    }
}

// MOV *(0:16bit),loc16
static void gen_16bit_loc16(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv imm_tcg = tcg_const_i32(imm);
    TCGv mode_tcg = tcg_const_i32(mode);
    TCGv is_rpt_tcg = tcg_const_i32(ctx->rpt_set);
    gen_helper_mov_16bit_loc16(cpu_env,mode_tcg,imm_tcg,is_rpt_tcg);
    tcg_temp_free(imm_tcg);
    tcg_temp_free(mode_tcg);
    tcg_temp_free(is_rpt_tcg);
}

// MOV ACC, loc16<<#0...15
static void gen_mov_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift) 
{
    TCGv oprand = tcg_temp_new();
    gen_ld_loc16(oprand, mode);
    gen_helper_extend_low_sxm(oprand, cpu_env, oprand);
    tcg_gen_shli_i32(cpu_acc, oprand, shift); // shift left, save to acc

    // set N,Z
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(oprand);
}

// MOV ACC,loc16<<T
static void gen_mov_acc_loc16_t(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_helper_extend_low_sxm(a, cpu_env, a);

    TCGv shift = tcg_temp_new();
    tcg_gen_shri_i32(shift, cpu_xt, 16);
    tcg_gen_andi_i32(shift, shift, 0x7);//T(3:0)
    tcg_gen_shl_i32(cpu_acc, a, shift);//acc = a<<T
    
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(a);
    tcg_temp_free(shift);
}

// MOV ACC, #16bit<<#0...15
static void gen_mov_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift) 
{
    TCGv oprand = tcg_const_i32(imm);
    gen_helper_extend_low_sxm(oprand, cpu_env, oprand);//16bit signed extend with sxm

    tcg_gen_shli_i32(cpu_acc, oprand, shift); // shift left, save to acc
    // set N,Z
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(oprand);
}

// MOV ARn,loc16
static void gen_mov_arn_loc16(DisasContext *ctx, uint32_t mode, uint32_t n)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_st_reg_low_half(cpu_xar[n], a);

    tcg_temp_free(a);
}

// MOV AX, loc16
static void gen_mov_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH) {
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(ax, mode);

    if (is_AH) {
        gen_st_reg_high_half(cpu_acc, ax);
    }
    else {
        gen_st_reg_low_half(cpu_acc, ax);
    }
    gen_helper_test_N_Z_16(cpu_env, ax);

    tcg_temp_free_i32(ax);
}

// MOV DP,#10bit
static void gen_mov_dp_10bit(DisasContext *ctx, uint32_t imm)
{
    TCGv a = tcg_const_i32(imm);
    tcg_gen_andi_i32(cpu_dp, cpu_dp, 0xfc00);
    tcg_gen_or_i32(cpu_dp, cpu_dp, a);
    tcg_temp_free(a);
}

// MOV IER,loc16
static void gen_mov_ier_loc16(DisasContext *ctx, uint32_t mode)
{
    gen_ld_loc16(cpu_ier, mode);
}

// MOV loc16,#16bit
static void gen_mov_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm) 
{
    TCGv imm_tcg = tcg_const_local_i32(imm);

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_st_loc16(mode, imm_tcg);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_test_ax_N_Z(mode);

    tcg_temp_free(imm_tcg);
}

// MOV loc16,*(0:16bit)
static void gen_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv imm_tcg = tcg_const_i32(imm);
    TCGv mode_tcg = tcg_const_i32(mode);
    TCGv is_rpt_tcg = tcg_const_i32(ctx->rpt_set);
    gen_helper_mov_loc16_16bit(cpu_env,mode_tcg,imm_tcg,is_rpt_tcg);
    tcg_temp_free(imm_tcg);
    tcg_temp_free(mode_tcg);
    tcg_temp_free(is_rpt_tcg);

    gen_test_ax_N_Z(mode);
}

// MOV loc16,#0
static void gen_mov_loc16_0(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_const_local_i32(0);

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_st_loc16(mode, a);
    gen_test_ax_N_Z(mode);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free(a);
}

// MOV loc16,ACC<<1...8
static void gen_mov_loc16_acc_shift(DisasContext *ctx, uint32_t mode, uint32_t shift, uint32_t insn_len)
{
    TCGv a = tcg_temp_local_new();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_shli_i32(a, cpu_acc, shift);
    tcg_gen_andi_i32(a, a, 0xffff);
    gen_st_loc16(mode, a);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_test_ax_N_Z(mode);

    tcg_temp_free(a);
}

// MOV loc16,ARn
static void gen_mov_loc16_arn(DisasContext *ctx, uint32_t mode, uint32_t n)
{
    TCGv a = tcg_temp_new();
    tcg_gen_andi_i32(a, cpu_xar[n], 0xffff);
    gen_st_loc16(mode, a);

    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

// MOV loc16,AX
static void gen_mov_loc16_ax(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv ax = tcg_temp_new();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_reg_half(ax, cpu_acc, is_AH);

    gen_st_loc16(mode, ax);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_test_ax_N_Z(mode);
    tcg_temp_free(ax);
}

// MOV loc16,AX,COND
static void gen_mov_loc16_ax_cond(DisasContext *ctx, uint32_t mode, uint32_t cond, bool is_AH)
{
    TCGLabel *loc16_modification = gen_new_label();
    TCGLabel *done = gen_new_label();
    TCGv cond_tcg = tcg_const_i32(cond);
    TCGv test = tcg_temp_new();
    gen_helper_test_cond(test, cpu_env, cond_tcg);
    tcg_gen_brcondi_i32(TCG_COND_EQ, test, 0, loc16_modification);
    
    TCGv ax = tcg_temp_new();
    gen_ld_reg_half(ax, cpu_acc, is_AH);
    gen_st_loc16(mode, ax);
    gen_test_ax_N_Z(mode);
    tcg_gen_br(done);

    //pre post modification addr mode
    gen_set_label(loc16_modification);

    // gen_ld_loc16(test,mode);//pre and post mod for addr mode, by doing a load loc16
    gen_get_loc_addr(test, mode, LOC16);//pre and post mod for addr mode, by doing a addr loc16
    
    gen_set_label(done);

    tcg_temp_free(cond_tcg);
    tcg_temp_free(test);
    tcg_temp_free(ax);
}

//MOV loc16,IER
static void gen_mov_loc16_ier(DisasContext *ctx, uint32_t mode)
{
    gen_st_loc16(mode, cpu_ier);
    gen_test_ax_N_Z(mode);
}

//MOV loc16,OVC
static void gen_mov_loc16_ovc(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    tcg_gen_andi_i32(a, cpu_st0, 0xfc00);//6bit of ovc,low bit are 0
    gen_st_loc16(mode, a);
    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

//MOV loc16,P
static void gen_mov_loc16_p(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_shift_by_pm(a, cpu_p);
    gen_st_loc16(mode, a);
    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

//MOV loc16,T
static void gen_mov_loc16_t(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_xt, 1);
    gen_st_loc16(mode, a);
    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

//MOV OVC,loc16
static void gen_mov_ovc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    tcg_gen_andi_i32(a, a, OVC_MASK);//15:10 = ovc
    tcg_gen_andi_i32(cpu_st0, cpu_st0, ~OVM_MASK);//clear ovc
    tcg_gen_or_i32(cpu_st0, cpu_st0, a);
    tcg_temp_free(a);
}

//MOV PH,loc16
static void gen_mov_ph_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_st_reg_high_half(cpu_p, a);
    tcg_temp_free(a);
}

//MOV PL,loc16
static void gen_mov_pl_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_st_reg_low_half(cpu_p, a);
    tcg_temp_free(a);
}

//MOV PM,AX
static void gen_mov_pm_ax(DisasContext *ctx, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);
    gen_set_bit(cpu_st0, PM_BIT, PM_MASK, a);
    tcg_temp_free(a);
}

//MOV T,loc16
static void gen_mov_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_st_reg_high_half(cpu_xt, a);
    tcg_temp_free(a);
}

// MOV TL,#0
static void gen_mov_tl_0(DisasContext *ctx)
{
    tcg_gen_andi_i32(cpu_xt, cpu_xt, 0xffff0000);
}

// MOV XARn,PC
static void gen_mov_xarn_pc(DisasContext *ctx, uint32_t n)
{
    tcg_gen_movi_i32(cpu_xar[n], ctx->base.pc_next >> 1);
}

// MOVA T,loc16
static void gen_mova_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv t = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGv a = tcg_temp_local_new();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_loc16(t, mode);
    gen_st_reg_high_half(cpu_xt, t);

    gen_shift_by_pm(b, cpu_p);//b = P>>PM

    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(t);
}

//MOVAD T,loc16
static void gen_movad_t_loc16(DisasContext *ctx, uint32_t mode)
{
    //mode = reg addressing is illegal
    if (mode >= 160 && mode <=173) {
        gen_exception(ctx, EXCP_INTERRUPT_ILLEGAL);
    }
    else
    {
        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);

        // T = loc16
        TCGv t = tcg_temp_local_new();
        gen_ld16u_swap(t, addr);
        gen_st_reg_high_half(cpu_xt, t);

        //loc16+1 = T
        tcg_gen_addi_i32(addr, addr, 1);
        gen_st16u_swap(t, addr);
        tcg_temp_free_i32(addr);

        //acc = acc + p<<pm
        TCGv b = tcg_temp_local_new();
        gen_shift_by_pm(b, cpu_p);//b = P>>PM

        TCGv a = tcg_temp_local_new();
        tcg_gen_mov_i32(a, cpu_acc);
        tcg_gen_add_i32(cpu_acc, a, b);

        gen_helper_test_N_Z_32(cpu_env, cpu_acc);
        gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

        tcg_temp_free_i32(a);
        tcg_temp_free_i32(b);
        tcg_temp_free_i32(t);
    }
}

// MOVB ACC,#8bit
static void gen_movb_acc_8bit(DisasContext *ctx, uint32_t imm)
{
    imm = imm & 0xff;
    tcg_gen_movi_i32(cpu_acc, imm);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// MOVB ARn,#8bit
static void gen_movb_arn_8bit(DisasContext *ctx, uint32_t imm, uint32_t n)
{
    imm = imm & 0xff;
    tcg_gen_andi_i32(cpu_xar[n], cpu_xar[n], 0xffff0000);//clear arn
    tcg_gen_ori_i32(cpu_xar[n], cpu_xar[n], imm);//set imm
}

// MOVB AX,#8bit
static void gen_movb_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    TCGv a = tcg_const_i32(imm);
    if (is_AH)
    {
        gen_st_reg_high_half(cpu_acc, a);
    }
    else {
        gen_st_reg_low_half(cpu_acc, a);
    }
    gen_helper_test_N_Z_16(cpu_env, a);
    tcg_temp_free(a);
}

// MOVB AH.LSB,loc16
static void gen_movb_ah_lsb_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16_byte_addressing(a, mode);
    tcg_gen_shli_i32(a, a, 16);
    tcg_gen_andi_i32(cpu_acc, cpu_acc, 0xff00ffff);//clear ah.lsb
    tcg_gen_or_i32(cpu_acc, cpu_acc, a);//set ah.lsb
    gen_ld_reg_half(a, cpu_acc, 1);//load new ah
    gen_helper_test_N_Z_16(cpu_env, a);//test ah for n,z
    tcg_temp_free(a);
}

// MOVB AL.LSB,loc16
static void gen_movb_al_lsb_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16_byte_addressing(a, mode);
    tcg_gen_andi_i32(cpu_acc, cpu_acc, 0xffffff00);//clear al.lsb
    tcg_gen_or_i32(cpu_acc, cpu_acc, a);//set al.lsb
    gen_ld_reg_half(a, cpu_acc, 0);//load new al
    gen_helper_test_N_Z_16(cpu_env, a);//test al for n,z
    tcg_temp_free(a);
}

// MOVB AH.MSB,loc16
static void gen_movb_ah_msb_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16_byte_addressing(a, mode);
    tcg_gen_shli_i32(a, a, 24);
    tcg_gen_andi_i32(cpu_acc, cpu_acc, 0x00ffffff);//clear ah.msb
    tcg_gen_or_i32(cpu_acc, cpu_acc, a);//set ah.msb
    gen_ld_reg_half(a, cpu_acc, 1);//load new ah
    gen_helper_test_N_Z_16(cpu_env, a);//test ah for n,z
    tcg_temp_free(a);
}

// MOVB AL.MSB,loc16
static void gen_movb_al_msb_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16_byte_addressing(a, mode);
    tcg_gen_shli_i32(a, a, 8);
    tcg_gen_andi_i32(cpu_acc, cpu_acc, 0xffff00ff);//clear al.msb
    tcg_gen_or_i32(cpu_acc, cpu_acc, a);//set al.msb
    gen_ld_reg_half(a, cpu_acc, 0);//load new al
    gen_helper_test_N_Z_16(cpu_env, a);//test al for n,z
    tcg_temp_free(a);
}

// MOVB loc16,#8bit,COND
static void gen_movb_loc16_8bit_cond(DisasContext *ctx, uint32_t mode, uint32_t imm, uint32_t cond)
{
    TCGLabel *loc16_modification = gen_new_label();
    TCGLabel *done = gen_new_label();
    TCGv cond_tcg = tcg_const_i32(cond);
    TCGv test = tcg_temp_new();
    gen_helper_test_cond(test, cpu_env, cond_tcg);
    tcg_gen_brcondi_i32(TCG_COND_EQ, test, 0, loc16_modification);
    
    TCGv a = tcg_const_i32(imm);
    gen_st_loc16(mode, a);
    gen_test_ax_N_Z(mode);
    tcg_gen_br(done);

    //pre post modification addr mode
    gen_set_label(loc16_modification);

    gen_ld_loc16(test,mode);//pre and post mod for addr mode, by doing a load loc16
    
    gen_set_label(done);

    tcg_temp_free(cond_tcg);
    tcg_temp_free(test);
    tcg_temp_free(a);
}

//MOVB loc16,AX.LSB
static void gen_movb_loc16_ax_lsb(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);
    tcg_gen_andi_i32(a, a, 0xff);
    gen_st_loc16_byte_addressing(mode, a);
    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

//MOVB loc16,AX.MSB
static void gen_movb_loc16_ax_msb(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);
    tcg_gen_shri_i32(a, a, 8);
    gen_st_loc16_byte_addressing(mode, a);
    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

// MOVB XARn,#8bit
static void gen_movb_xarn_8bit(DisasContext *ctx, uint32_t imm, uint32_t n)
{
    imm = imm & 0xff;
    tcg_gen_movi_i32(cpu_xar[n], imm);
}

// MOVDL XT,loc32
static void gen_movdl_xt_loc32(DisasContext *ctx, uint32_t mode)
{
    //mode = reg addressing is illegal
    if (is_reg_addressing_mode(mode, LOC32)) {
        gen_exception(ctx, EXCP_INTERRUPT_ILLEGAL);
    }
    else
    {
        TCGv_i32 addr = tcg_temp_local_new();

        TCGLabel *begin = gen_new_label();
        TCGLabel *end = gen_new_label();
        gen_set_label(begin);

        gen_get_loc_addr(addr, mode, LOC32);

        //XT = [loc32]
        gen_ld32u_swap(cpu_xt, addr);
        //[loc32+2] = XT
        tcg_gen_addi_i32(addr, addr, 2);
        gen_st32u_swap(cpu_xt, addr);

        tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
        tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
        tcg_gen_br(begin);
        gen_set_label(end);

        tcg_temp_free_i32(addr);
    }
}

// MOVH loc16,ACC<<1...8
static void gen_movh_loc16_acc_shift(DisasContext *ctx, uint32_t mode, uint32_t shift, uint32_t insn_len)
{
    TCGv a = tcg_temp_local_new();
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_shli_i32(a, cpu_acc, shift);
    tcg_gen_shri_i32(a, cpu_acc, 16);
    tcg_gen_andi_i32(a, a, 0xffff);
    gen_st_loc16(mode, a);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_test_ax_N_Z(mode);

    tcg_temp_free(a);
}

//MOVH loc16,P
static void gen_movh_loc16_p(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_shift_by_pm(a, cpu_p);
    tcg_gen_shri_i32(a, a, 16);
    gen_st_loc16(mode, a);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

// MOVL ACC,loc32
static void gen_movl_acc_loc32(DisasContext *ctx, uint32_t mode) {
    gen_ld_loc32(cpu_acc, mode);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// MOVL loc32,ACC
static void gen_movl_loc32_acc(DisasContext *ctx, uint32_t mode) {
    gen_st_loc32(mode,cpu_acc);
    gen_test_acc_N_Z(mode);
}

// MOVL loc32,ACC,COND
static void gen_movl_loc32_acc_cond(DisasContext *ctx, uint32_t mode, uint32_t cond)
{
    TCGLabel *loc32_modification = gen_new_label();
    TCGLabel *done = gen_new_label();
    TCGv cond_tcg = tcg_const_i32(cond);
    TCGv test = tcg_temp_new();
    gen_helper_test_cond(test, cpu_env, cond_tcg);
    tcg_gen_brcondi_i32(TCG_COND_EQ, test, 0, loc32_modification);
    
    gen_st_loc32(mode, cpu_acc);
    gen_test_acc_N_Z(mode);
    tcg_gen_br(done);

    //pre post modification addr mode
    gen_set_label(loc32_modification);

    gen_get_loc_addr(test, mode, LOC32);//pre and post mod for addr mode, by doing a addr loc32
    
    gen_set_label(done);

    tcg_temp_free(cond_tcg);
    tcg_temp_free(test);
}

// MOVL loc32,P
static void gen_movl_loc32_p(DisasContext *ctx, uint32_t mode)
{
    gen_st_loc32(mode, cpu_p);
    gen_test_acc_N_Z(mode);
}

// MOVL loc32,XARn
static void gen_movl_loc32_xarn(DisasContext *ctx, uint32_t mode, uint32_t n) {
    gen_st_loc32(mode, cpu_xar[n]);
    gen_test_acc_N_Z(mode);
}

// MOVL loc32,XT
static void gen_movl_loc32_xt(DisasContext *ctx, uint32_t mode)
{
    gen_st_loc32(mode, cpu_xt);
    gen_test_acc_N_Z(mode);
}

// MOVL P,ACC
static void gen_movl_p_acc(DisasContext *ctx)
{
    tcg_gen_mov_i32(cpu_p, cpu_acc);
}

// MOVL P,loc32
static void gen_movl_p_loc32(DisasContext *ctx, uint32_t mode)
{
    gen_ld_loc32(cpu_p, mode);
}

// MOVL XARn,loc32
static void gen_movl_xarn_loc32(DisasContext *ctx, uint32_t mode, uint32_t n)
{
    gen_ld_loc32(cpu_xar[n], mode);
}

// MOVL XARn,#22bit
static void gen_movl_xarn_22bit(DisasContext *ctx, uint32_t n, uint32_t imm) {
    tcg_gen_movi_i32(cpu_xar[n], imm);
}

// MOVL XT,loc32
static void gen_movl_xt_loc32(DisasContext *ctx, uint32_t mode)
{
    gen_ld_loc32(cpu_xt, mode);
}

//MOVP T,loc16
static void gen_movp_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_st_reg_high_half(cpu_xt, a); //T = [loc16]
    gen_shift_by_pm(cpu_acc, cpu_p);//ACC = P << PM
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free(a);
}

// MOVS T,loc16
static void gen_movs_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);
    
    gen_ld_loc16(a, mode);
    gen_st_reg_high_half(cpu_xt, a); //T = [loc16]

    tcg_gen_mov_i32(a, cpu_acc);
    gen_shift_by_pm(b, cpu_p);//b = P << PM
    tcg_gen_sub_i32(cpu_acc, a, b);
    gen_helper_test_sub_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// MOVU ACC,loc16
static void gen_movu_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    tcg_gen_andi_i32(a, a, 0xffff);
    tcg_gen_mov_i32(cpu_acc, a);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free(a);
}

// MOVU loc16,OVC
static void gen_movu_loc16_ovc(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_get_bit(a, cpu_st0, OVC_BIT, OVC_MASK);//get ovc bit 
    gen_st_loc16(mode, a);
    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

// MOVU OVC,loc16
static void gen_movu_ovc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_set_bit(cpu_st0, OVC_BIT, OVC_MASK, a);
    tcg_temp_free(a);
}

// MOVW DP,#16bit
static void gen_movw_dp_16bit(DisasContext *ctx, uint32_t imm) {
    tcg_gen_movi_tl(cpu_dp, imm);
}

// MOVX TL,loc16
static void gen_movx_tl_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    tcg_gen_ext16s_tl(a, a);
    tcg_gen_mov_i32(cpu_xt, a);
    tcg_temp_free(a);
}

// MOVZ ARn,loc16
static void gen_movz_arn_loc16(DisasContext *ctx, uint32_t mode, uint32_t n)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    tcg_gen_andi_i32(a, a, 0xffff);
    tcg_gen_mov_i32(cpu_xar[n], a);
    tcg_temp_free(a);
}

// MOVZ DP,#10bit
static void gen_movz_dp_10bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_movi_tl(cpu_dp, imm);
}

// // POP ACC
// static void gen_pop_acc(DisasContext *ctx)
// {
//     TCGv sp = tcg_temp_new();
//     tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
//     tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);//align to 32bit
//     gen_ld32u_swap(cpu_acc, sp);
//     tcg_temp_free(sp);
// }

// POP ARn:ARm
static void gen_pop_arn_arm(DisasContext *ctx, uint32_t n, uint32_t m)
{
    TCGv tmp = tcg_temp_new();
    TCGv tmp2 = tcg_temp_new();
    TCGv sp = tcg_temp_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_ld32u_swap(tmp, sp);
    tcg_gen_shri_i32(tmp2, tmp, 16);//arm
    tcg_gen_andi_i32(tmp, tmp, 0xffff);//arn

    gen_st_reg_low_half(cpu_xar[m], tmp);
    gen_st_reg_low_half(cpu_xar[n], tmp2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp2);
    tcg_temp_free(sp);
}

// POP AR1H:AR0H
static void gen_pop_ar1h_ar0h(DisasContext *ctx) {
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv sp = tcg_temp_local_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_ld32u_swap(tmp, sp);
    tcg_gen_shri_i32(tmp2, tmp, 16);//ar1h
    tcg_gen_andi_i32(tmp, tmp, 0xffff);//ar0h

    gen_st_reg_high_half(cpu_xar[0], tmp);
    gen_st_reg_high_half(cpu_xar[1], tmp2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp2);
    tcg_temp_free(sp);
}

// POP DBGIER
static void gen_pop_dbgier(DisasContext *ctx) {
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(cpu_dbgier, cpu_sp);
}

// POP DP
static void gen_pop_dp(DisasContext *ctx) {
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(cpu_dp, cpu_sp);
}

// POP DP:ST1
static void gen_pop_dp_st1(DisasContext *ctx) {
    TCGv tmp = tcg_temp_new();
    TCGv sp = tcg_temp_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_ld32u_swap(tmp, sp);
    tcg_gen_shri_i32(cpu_dp, tmp, 16);//dp
    tcg_gen_andi_i32(cpu_st1, tmp, 0xffff);//st1
    //todo: in ccs, some bit of st1 cannot be set

    tcg_temp_free(tmp);
    tcg_temp_free(sp);
}

// POP IFR
static void gen_pop_ifr(DisasContext *ctx) {
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(cpu_ifr, cpu_sp);
}

// POP loc16
static void gen_pop_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv tmp = tcg_temp_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(tmp, cpu_sp);
    gen_st_loc16(mode, tmp);
    gen_test_ax_N_Z(mode);
    tcg_temp_free(tmp);
}

// POP P
static void gen_pop_p(DisasContext *ctx)
{
    TCGv sp = tcg_temp_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);//align to 32bit
    gen_ld32u_swap(cpu_p, sp);
    tcg_temp_free(sp);
}

// POP RPC
static void gen_pop_rpc(DisasContext *ctx)
{
    TCGv sp = tcg_temp_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);

    gen_ld32u_swap(cpu_rpc, sp);
    tcg_gen_andi_i32(cpu_rpc, cpu_rpc, 0x3fffff);//rpc is 22bit 
    tcg_temp_free(sp);
}

// POP ST0
static void gen_pop_st0(DisasContext *ctx)
{
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(cpu_st0, cpu_sp);
}

// POP ST1
static void gen_pop_st1(DisasContext *ctx)
{
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(cpu_st1, cpu_sp);
}

// POP T:ST0
static void gen_pop_t_st0(DisasContext *ctx) {
    TCGv tmp = tcg_temp_new();
    TCGv sp = tcg_temp_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_ld32u_swap(tmp, sp);
    tcg_gen_andi_i32(cpu_st0, tmp, 0xffff);//st0
    tcg_gen_shri_i32(tmp, tmp, 16);
    gen_st_reg_high_half(cpu_xt, tmp);//T

    tcg_temp_free(tmp);
    tcg_temp_free(sp);
}

// // POP XARn
// static void gen_pop_xarn(DisasContext *ctx, uint32_t n)
// {
//     TCGv sp = tcg_temp_new();
//     tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
//     tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);

//     gen_ld32u_swap(cpu_xar[n], sp);
//     tcg_temp_free(sp);
// }

// PREAD loc16,*XAR7
static void gen_pread_loc16_xar7(DisasContext *ctx, uint32_t mode)
{
    TCGv tmp = tcg_temp_local_new_i32();
    tcg_gen_mov_i32(cpu_tmp[7], cpu_xar[7]);

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld16u_swap(tmp, cpu_tmp[7]);
    gen_st_loc16(mode, tmp);
    gen_test_ax_N_Z(mode);
    tcg_gen_addi_i32(cpu_tmp[7], cpu_tmp[7], 1);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free(tmp);
}

// PUSH ARn:ARm
static void gen_push_arn_arm(DisasContext *ctx, uint32_t n, uint32_t m)
{
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv sp = tcg_temp_local_new();
    tcg_gen_andi_i32(tmp, cpu_xar[m], 0xffff);
    tcg_gen_shli_i32(tmp2, cpu_xar[n], 16);
    tcg_gen_or_i32(tmp, tmp, tmp2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_st32u_swap(tmp, sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp2);
    tcg_temp_free(sp);
}

// PUSH AR1H:AR0H
static void gen_push_ar1h_ar0h(DisasContext *ctx)
{
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv sp = tcg_temp_local_new();
    tcg_gen_andi_i32(tmp, cpu_xar[1], 0xffff0000);
    tcg_gen_shri_i32(tmp2, cpu_xar[0], 16);
    tcg_gen_or_i32(tmp, tmp, tmp2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_st32u_swap(tmp, sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp2);
    tcg_temp_free(sp);
}

// PUSH DBGIER
static void gen_push_dbgier(DisasContext *ctx)
{
    gen_st16u_swap(cpu_dbgier, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 1);
}

// PUSH DP
static void gen_push_dp(DisasContext *ctx)
{
    gen_st16u_swap(cpu_dp, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 1);
}

// PUSH DP:ST1
static void gen_push_dp_st1(DisasContext *ctx)
{
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv sp = tcg_temp_local_new();
    tcg_gen_andi_i32(tmp, cpu_st1, 0xffff);
    tcg_gen_shli_i32(tmp2, cpu_dp, 16);
    tcg_gen_or_i32(tmp, tmp, tmp2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_st32u_swap(tmp, sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp2);
    tcg_temp_free(sp);
}

// PUSH IFR
static void gen_push_ifr(DisasContext *ctx)
{
    gen_st16u_swap(cpu_dp, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 1);
}

// PUSH loc16
static void gen_push_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv tmp = tcg_temp_new();
    gen_ld_loc16(tmp, mode);
    gen_st16u_swap(tmp, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 1);
    tcg_temp_free(tmp);
}

// PUSH P
static void gen_push_p(DisasContext *ctx)
{
    TCGv sp = tcg_temp_local_new();
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_st32u_swap(cpu_p, sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);
    tcg_temp_free(sp);
}

// PUSH RPC
static void gen_push_rpc(DisasContext *ctx)
{
    TCGv sp = tcg_temp_local_new();
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_st32u_swap(cpu_rpc, sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);
    tcg_temp_free(sp);
}

// PUSH ST0
static void gen_push_st0(DisasContext *ctx)
{
    gen_st16u_swap(cpu_st0, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 1);
}

// PUSH ST1
static void gen_push_st1(DisasContext *ctx)
{
    gen_st16u_swap(cpu_st1, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 1);
}

// PUSH T:ST0
static void gen_push_t_st0(DisasContext *ctx)
{
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv sp = tcg_temp_local_new();
    tcg_gen_andi_i32(tmp, cpu_st0, 0xffff);
    tcg_gen_andi_i32(tmp2, cpu_xt, 0xffff0000);
    tcg_gen_or_i32(tmp, tmp, tmp2);
    tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
    gen_st32u_swap(tmp, sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp2);
    tcg_temp_free(sp);
}

// // PUSH XT
// static void gen_push_xt(DisasContext *ctx)
// {
//     TCGv sp = tcg_temp_local_new();
//     tcg_gen_andi_i32(sp, cpu_sp, 0xfffffffe);
//     gen_st32u_swap(cpu_xt, sp);
//     tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);
//     tcg_temp_free(sp);
// }

// PWRITE *XAR7,loc16
static void gen_pwrite_xar7_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv tmp = tcg_temp_local_new();
    if (mode == 0b10000111)//*XAR7++
    {
        tcg_gen_addi_i32(cpu_tmp[7], cpu_xar[7], 1);
    }
    else if (mode == 0b10001111)//*--XAR7
    {
        tcg_gen_subi_i32(cpu_tmp[7], cpu_xar[7], 1);
    }
    else
    {
        tcg_gen_mov_i32(cpu_tmp[7], cpu_xar[7]);
    }

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_loc16(tmp, mode);
    gen_st16u_swap(tmp, cpu_tmp[7]);

    tcg_gen_addi_i32(cpu_tmp[7], cpu_tmp[7], 1);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free(tmp);
}

// XPREAD loc16,*(pma)
static void gen_xpwread_loc16_pma(DisasContext *ctx, uint32_t mode, uint32_t pma)
{
    TCGv addr = cpu_tmp[0];
    TCGv value = cpu_tmp[1];
    tcg_gen_movi_i32(value, 0x3f0000 | pma);

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld16u_swap(value, addr);
    gen_st_loc16(mode, value);
    gen_test_ax_N_Z(mode);
    tcg_gen_addi_i32(addr, addr, 1);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
}

// XPREAD loc16,*al
static void gen_xpwread_loc16_al(DisasContext *ctx, uint32_t mode)
{
    TCGv addr = cpu_tmp[0];
    TCGv value = cpu_tmp[1];
    TCGv al = cpu_tmp[2];
    gen_ld_reg_half(al, cpu_acc, false);
    tcg_gen_xori_i32(addr, al, 0x3f0000);

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld16u_swap(value, addr);
    gen_st_loc16(mode, value);
    gen_test_ax_N_Z(mode);
    tcg_gen_addi_i32(addr, addr, 1);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
}

//XPWRITE *AL,loc16
static void gen_xpwrite_al_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv addr = cpu_tmp[0];
    TCGv loc16 = cpu_tmp[1];
    TCGv al = cpu_tmp[2];
    gen_ld_reg_half(al, cpu_acc, false);
    tcg_gen_xori_i32(addr, al, 0x3f0000);

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_loc16(loc16, mode);
    gen_st16u_swap(loc16, addr);
    tcg_gen_addi_i32(addr, addr, 1);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
}

