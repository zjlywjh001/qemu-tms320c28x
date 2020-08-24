#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "exception.h"
#include "fpu/softfloat.h"

uint32_t HELPER(fpu_absf)(CPUTms320c28xState *env, uint32_t value)
{
    float32 ret = float32_abs(make_float32(value));
    //NF=0
    cpu_set_stf(env, 0, NF_BIT, NF_MASK);
    // ZF = 0;
    // if ( RaH[30:23] == 0) ZF = 1;
    if((ret & 0x7f800000) == 0)
    {
        cpu_set_stf(env, 1, ZF_BIT, ZF_MASK);
    }
    else
    {
        cpu_set_stf(env, 0, ZF_BIT, ZF_MASK);
    }
    return ret;
}

uint32_t HELPER(fpu_addf)(CPUTms320c28xState *env, uint32_t a, uint32_t b)
{
    //set round mode
    if (cpu_get_stf(env, RND32_BIT, RND32_MASK) == 0) //truncate
    {
        env->fp_status.float_rounding_mode = float_round_to_zero;
    }
    else
    {
        env->fp_status.float_rounding_mode = float_round_nearest_even;
    }
    float32 ret = float32_add(a, b, &env->fp_status);
    //LVF
    if (env->fp_status.float_exception_flags & float_flag_overflow)
    {
        cpu_set_stf(env, 1, LVF_BIT, LVF_MASK);
    }
    //LUF
    if (env->fp_status.float_exception_flags & float_flag_underflow)
    {
        cpu_set_stf(env, 1, LUF_BIT, LUF_MASK);
    }
    return ret;
}

void HELPER(fpu_cmpf)(CPUTms320c28xState *env, uint32_t a, uint32_t b)
{
    //Negative zero will be treated as positive zero. ---------checked
    //A denormalized value will be treated as positive zero.
    env->fp_status.flush_inputs_to_zero = 1;
    //Not-a-Number (NaN) will be treated as infinity.
    if (a == 0x7FBFFFFF)
        a = 0x7F800000;
    if (b == 0x7FBFFFFF)
        b = 0x7F800000;
    //compare
    int flag = float32_compare(a, b, &env->fp_status);
    switch(flag)
    {
        case float_relation_less://If(RaH < RbH) {ZF=0, NF=1}
            cpu_set_stf(env, 0, ZF_BIT, ZF_MASK);
            cpu_set_stf(env, 1, NF_BIT, NF_MASK);
            break;
        case float_relation_equal://If(RaH == RbH) {ZF=1, NF=0}
            cpu_set_stf(env, 1, ZF_BIT, ZF_MASK);
            cpu_set_stf(env, 0, NF_BIT, NF_MASK);
            break;
        case float_relation_greater://If(RaH > RbH) {ZF=0, NF=0}
            cpu_set_stf(env, 0, ZF_BIT, ZF_MASK);
            cpu_set_stf(env, 0, NF_BIT, NF_MASK);
            break;
        case float_relation_unordered:
        default:
            g_assert_not_reached();
    }
    //restore status
    env->fp_status.flush_inputs_to_zero = 0;
}

uint32_t HELPER(fpu_f32toi16)(CPUTms320c28xState *env, uint32_t value)
{
    env->fp_status.float_rounding_mode = float_round_to_zero;
    int ret = float32_to_int16(value, &env->fp_status);
    return ret;
}

uint32_t HELPER(fpu_f32toi16r)(CPUTms320c28xState *env, uint32_t value)
{
    env->fp_status.float_rounding_mode = float_round_nearest_even;
    int ret = float32_to_int16(value, &env->fp_status);
    return ret;
}