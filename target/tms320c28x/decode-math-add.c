// ABS ACC
static void gen_abs_acc(DisasContext *ctx)
{
    gen_helper_abs_acc(cpu_env);
}

// ABSTC ACC
static void gen_abstc_acc(DisasContext *ctx)
{
    gen_helper_abstc_acc(cpu_env);
}

// ADD ACC,#16bit<<#0...15
static void gen_add_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    TCGv b = tcg_const_i32(imm);
    gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
    tcg_gen_shli_i32(b, b, shift);
    
    TCGv a = tcg_const_i32(0);
    tcg_gen_mov_i32(a, cpu_acc);

    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// ADD ACC,loc16<<T
static void gen_add_acc_loc16_t(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGv shift = tcg_temp_new();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);
        tcg_gen_mov_i32(a, cpu_acc);//get a
        gen_ld_loc16(b, mode); //get b
        gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
        tcg_gen_shri_i32(shift, cpu_xt, 16);
        tcg_gen_andi_i32(shift, shift, 0x7);//T(3:0)
        tcg_gen_shl_i32(b, b, shift);//b = b<<T

        tcg_gen_add_i32(cpu_acc, a, b);//add

        gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);

    gen_set_label(end);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(shift);
}

// ADD ACC,loc16<<#0...16
static void gen_add_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift_imm)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);
        tcg_gen_mov_i32(a, cpu_acc);//get a
        gen_ld_loc16(b, mode); //get b
        gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
        tcg_gen_shli_i32(b, b, shift_imm);//b = b<<shift

        tcg_gen_add_i32(cpu_acc, a, b);//add

        gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

        if (shift_imm == 16)
            gen_helper_test_C_32_shift16(cpu_env, a, b, cpu_acc);
        else
            gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}


// ADD AX,loc16
static void gen_add_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH) {
    TCGv b = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    TCGv a = tcg_temp_new();
    gen_ld_loc16(b, mode);

    gen_ld_reg_half(a, cpu_acc, is_AH);

    tcg_gen_add_i32(ax, a, b);//add

    gen_helper_test_N_Z_16(cpu_env, ax);
    gen_helper_test_C_V_16(cpu_env, a, b, ax);

    if (is_AH) {
        gen_st_reg_high_half(cpu_acc, ax);
    }
    else {
        gen_st_reg_low_half(cpu_acc, ax);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(ax);
}

// ADD loc16,AX
static void gen_add_loc16_ax(DisasContext *ctx, uint32_t mode, bool is_AH) 
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    TCGv result = tcg_temp_new();

    if (is_reg_addressing_mode(mode, LOC16)) {
        gen_ld_loc16(a, mode);
        gen_ld_reg_half(b, cpu_acc, is_AH);
        tcg_gen_add_i32(result, a, b);//add
        gen_st_loc16(mode, result);//store
        gen_helper_test_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
    }
    else {
        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr);

        gen_ld_reg_half(b, cpu_acc, is_AH);

        tcg_gen_add_i32(result, a, b);//add

        gen_helper_test_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
        
        gen_st16u_swap(result, addr); //store

        tcg_temp_free_i32(addr);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(result);
}

// ADD loc16,#16bitSigned
static void gen_add_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_const_i32(imm);
    TCGv result = tcg_temp_new();

    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(a, mode);
        tcg_gen_add_i32(result, a, b);//add

        gen_helper_test_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);

        gen_st_loc16(mode, result);
    }
    else
    {
        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr);

        tcg_gen_add_i32(result, a, b);//add


        gen_helper_test_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
        
        gen_st16u_swap(result, addr);//store
        
        tcg_temp_free_i32(addr);
    }
    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(result);
}

// ADDB ACC,#8bit
static void gen_addb_acc_8bit(DisasContext *ctx, uint32_t imm)
{
    TCGv a = tcg_temp_new();
    tcg_gen_mov_i32(a, cpu_acc);
    TCGv b = tcg_const_i32(imm);

    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// ADDB AX,#8bitSigned
static void gen_addb_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    if ((imm >> 7) == 1) {
        imm = imm | 0xff00;
    }
    TCGv b = tcg_const_i32(imm);

    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);


    TCGv tmp = tcg_temp_new();
    tcg_gen_add_i32(tmp, a, b);

    gen_helper_test_N_Z_16(cpu_env, tmp);
    gen_helper_test_C_V_16(cpu_env, a, b, tmp);

    if (is_AH) {
        gen_st_reg_high_half(cpu_acc, tmp);
    }
    else {
        gen_st_reg_low_half(cpu_acc, tmp);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(tmp);
}

// ADDB SP,#7bit
static void gen_addb_sp_7bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_addi_i32(cpu_sp, cpu_sp, imm);
}

// ADDB XARn,#7bit
static void gen_addb_xarn_7bit(DisasContext *ctx, uint32_t n, uint32_t imm)
{
    tcg_gen_addi_i32(cpu_xar[n], cpu_xar[n], imm);
}

// ADDCL ACC,loc32
static void gen_addcl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);

    TCGv c = tcg_temp_local_new();
    gen_get_bit(c, cpu_st0, C_BIT, C_MASK);

    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_i32(tmp, a, b);
    tcg_gen_add_i32(cpu_acc, tmp, c);

    gen_helper_test2_C_V_OVC_OVM_32(cpu_env, a, b,c, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(tmp);
}

// ADDCU ACC,loc16
static void gen_addcu_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc16(b, mode);

    TCGv c = tcg_temp_local_new();
    gen_get_bit(c, cpu_st0, C_BIT, C_MASK);

    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_i32(tmp, a, b);
    tcg_gen_add_i32(cpu_acc, tmp, c);

    gen_helper_test2_C_V_OVC_OVM_32(cpu_env, a, b,c, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(c);
    tcg_temp_free_i32(tmp);
}

// ADDL ACC,loc32
static void gen_addl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);

    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// ADDL loc32,ACC
static void gen_addl_loc32_acc(DisasContext *ctx, uint32_t mode)
{
    if (is_reg_addressing_mode(mode, LOC32))
    {
        TCGv b = tcg_temp_new();
        gen_ld_loc32(b, mode);

        TCGv tmp = tcg_temp_new();
        tcg_gen_add_i32(tmp, cpu_acc, b);

        gen_helper_test_N_Z_32(cpu_env, cpu_acc);
        gen_helper_test_C_V_32(cpu_env, cpu_acc, b, tmp);
        gen_helper_test_OVC_OVM_32(cpu_env, cpu_acc, b, tmp);

        gen_st_loc32(mode, tmp);
        tcg_temp_free_i32(tmp);
        tcg_temp_free_i32(b);
    }
    else 
    {
        TCGv b = tcg_temp_new();

        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC32);
        gen_ld32u_swap(b, addr);

        TCGv tmp = tcg_temp_new();
        tcg_gen_add_i32(tmp, cpu_acc, b);

        gen_helper_test_N_Z_32(cpu_env, cpu_acc);
        gen_helper_test_C_V_32(cpu_env, cpu_acc, b, tmp);
        gen_helper_test_OVC_OVM_32(cpu_env, cpu_acc, b, tmp);

        gen_st32u_swap(tmp, addr);

        tcg_temp_free_i32(tmp);
        tcg_temp_free_i32(b);
        tcg_temp_free(addr);
    }
}

// ADDU ACC,loc16
static void gen_addu_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();

    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();

    gen_set_label(begin);

    tcg_gen_mov_i32(a, cpu_acc);
    gen_ld_loc16(b, mode);

    tcg_gen_add_i32(cpu_acc, a, b);
    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// ADDUL P,loc32
static void gen_addul_p_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc32(a, mode);

    TCGv b = tcg_temp_new();
    tcg_gen_mov_i32(b, cpu_p);

    tcg_gen_add_i32(cpu_p, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_p);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_p);
    gen_helper_test_OVCU_32(cpu_env, a, b, cpu_p);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// ADDUL ACC,loc32
static void gen_addul_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_loc32(a, mode);
    tcg_gen_mov_i32(b, cpu_acc);
    tcg_gen_add_i32(cpu_acc, a, b);
    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVCU_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// ADRK #8bit
static void gen_adrk_8bit(DisasContext *ctx, uint32_t imm)
{
    TCGv a = tcg_temp_new();
    gen_helper_ld_xar_arp(a, cpu_env);
    tcg_gen_addi_i32(a, a, imm);
    gen_helper_st_xar_arp(cpu_env, a);
    tcg_temp_free(a);
}

// INC loc16
static void gen_inc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_const_i32(1);
    TCGv result = tcg_temp_new();

    if (is_reg_addressing_mode(mode, LOC16)) {
        gen_ld_loc16(a, mode);
        tcg_gen_add_i32(result, a, b);//add
        gen_st_loc16(mode, result);//store
        gen_helper_test_sub_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
    }
    else {
        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr);

        tcg_gen_add_i32(result, a, b);//add

        gen_helper_test_sub_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
        
        gen_st16u_swap(result, addr); //store

        tcg_temp_free_i32(addr);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(result);
}