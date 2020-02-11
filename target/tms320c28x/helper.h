/*
 * TMS320C28x helper defines
 *
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

DEF_HELPER_FLAGS_2(exception, TCG_CALL_NO_WG, void, env, i32)
DEF_HELPER_3(addressing_mode, i32, env, i32, i32)

DEF_HELPER_4(branch_cond,void,env,i32,i32,i32)

DEF_HELPER_3(test_N, void, env, i32, i32)
DEF_HELPER_3(test_Z, void, env, i32, i32)

DEF_HELPER_2(test_C_V_16, void, env, i32)
DEF_HELPER_3(test_C_V_32, void, env, i32, i32)
DEF_HELPER_3(test_sub_C_V_32, void, env, i32, i32)

//affect acc value
DEF_HELPER_3(test_OVC_32_set_acc, void, env, i32, i32)


DEF_HELPER_2(ld_high_sxm, i32, env, i32)
DEF_HELPER_2(ld_low_sxm, i32, env, i32)

DEF_HELPER_2(print, void, env, i32)
DEF_HELPER_1(print_env, void, env)

//interrupt
DEF_HELPER_1(aborti, void, env)