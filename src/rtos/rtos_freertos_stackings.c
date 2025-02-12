/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rtos_freertos_stackings.h"
#include "rtos_standard_stackings.h"
#include "target/target.h"
#include "helper/log.h"
#include "helper/binarybuffer.h"
#include <target/espressif/esp32.h>
#include <target/espressif/esp32s2.h>
#include <target/espressif/esp32s3.h>
#include <target/riscv/riscv.h>

static int rtos_freertos_esp_xtensa_stack_read_involuntary(struct target *target, int64_t stack_ptr, const struct rtos_register_stacking *stacking, uint8_t *stack_data);
static int rtos_freertos_esp_xtensa_stack_read_voluntary(struct target *target, int64_t stack_ptr, const struct rtos_register_stacking *stacking, uint8_t *stack_data);

/*
 The XTensa FreeRTOS implementation has *two* types of stack frames; one for
 involuntatrily swapped out tasks and another one for tasks which voluntarily yielded.
*/
static const struct stack_register_offset rtos_freertos_esp32_stack_offsets[] = {
	{ XT_REG_IDX_PC, 0x04, 32 },		/* PC */
	{ XT_REG_IDX_AR0, 0x0c, 32 },		/* A0 */
	{ XT_REG_IDX_AR1, 0x10, 32 },		/* A1 */
	{ XT_REG_IDX_AR2, 0x14, 32 },		/* A2 */
	{ XT_REG_IDX_AR3, 0x18, 32 },		/* A3 */
	{ XT_REG_IDX_AR4, 0x1c, 32 },		/* A4 */
	{ XT_REG_IDX_AR5, 0x20, 32 },		/* A5 */
	{ XT_REG_IDX_AR6, 0x24, 32 },		/* A6 */
	{ XT_REG_IDX_AR7, 0x28, 32 },		/* A7 */
	{ XT_REG_IDX_AR8, 0x2c, 32 },		/* A8 */
	{ XT_REG_IDX_AR9, 0x30, 32 },		/* A9 */
	{ XT_REG_IDX_AR10, 0x34, 32 },		/* A10 */
	{ XT_REG_IDX_AR11, 0x38, 32 },		/* A11 */
	{ XT_REG_IDX_AR12, 0x3c, 32 },		/* A12 */
	{ XT_REG_IDX_AR13, 0x40, 32 },		/* A13 */
	{ XT_REG_IDX_AR14, 0x44, 32 },		/* A14 */
	{ XT_REG_IDX_AR15, 0x48, 32 },		/* A15 */
	{ XT_REG_IDX_AR16, -1, 32 },		/* A16 */ /* A16-A63 aren't in the stack frame because they've been flushed to the stack earlier */
	{ XT_REG_IDX_AR17, -1, 32 },		/* A17 */
	{ XT_REG_IDX_AR18, -1, 32 },		/* A18 */
	{ XT_REG_IDX_AR19, -1, 32 },		/* A19 */
	{ XT_REG_IDX_AR20, -1, 32 },		/* A20 */
	{ XT_REG_IDX_AR21, -1, 32 },		/* A21 */
	{ XT_REG_IDX_AR22, -1, 32 },		/* A22 */
	{ XT_REG_IDX_AR23, -1, 32 },		/* A23 */
	{ XT_REG_IDX_AR24, -1, 32 },		/* A24 */
	{ XT_REG_IDX_AR25, -1, 32 },		/* A25 */
	{ XT_REG_IDX_AR26, -1, 32 },		/* A26 */
	{ XT_REG_IDX_AR27, -1, 32 },		/* A27 */
	{ XT_REG_IDX_AR28, -1, 32 },		/* A28 */
	{ XT_REG_IDX_AR29, -1, 32 },		/* A29 */
	{ XT_REG_IDX_AR30, -1, 32 },		/* A30 */
	{ XT_REG_IDX_AR31, -1, 32 },		/* A31 */
	{ XT_REG_IDX_AR32, -1, 32 },		/* A32 */
	{ XT_REG_IDX_AR33, -1, 32 },		/* A33 */
	{ XT_REG_IDX_AR34, -1, 32 },		/* A34 */
	{ XT_REG_IDX_AR35, -1, 32 },		/* A35 */
	{ XT_REG_IDX_AR36, -1, 32 },		/* A36 */
	{ XT_REG_IDX_AR37, -1, 32 },		/* A37 */
	{ XT_REG_IDX_AR38, -1, 32 },		/* A38 */
	{ XT_REG_IDX_AR39, -1, 32 },		/* A39 */
	{ XT_REG_IDX_AR40, -1, 32 },		/* A40 */
	{ XT_REG_IDX_AR41, -1, 32 },		/* A41 */
	{ XT_REG_IDX_AR42, -1, 32 },		/* A42 */
	{ XT_REG_IDX_AR43, -1, 32 },		/* A43 */
	{ XT_REG_IDX_AR44, -1, 32 },		/* A44 */
	{ XT_REG_IDX_AR45, -1, 32 },		/* A45 */
	{ XT_REG_IDX_AR46, -1, 32 },		/* A46 */
	{ XT_REG_IDX_AR47, -1, 32 },		/* A47 */
	{ XT_REG_IDX_AR48, -1, 32 },		/* A48 */
	{ XT_REG_IDX_AR49, -1, 32 },		/* A49 */
	{ XT_REG_IDX_AR50, -1, 32 },		/* A50 */
	{ XT_REG_IDX_AR51, -1, 32 },		/* A51 */
	{ XT_REG_IDX_AR52, -1, 32 },		/* A52 */
	{ XT_REG_IDX_AR53, -1, 32 },		/* A53 */
	{ XT_REG_IDX_AR54, -1, 32 },		/* A54 */
	{ XT_REG_IDX_AR55, -1, 32 },		/* A55 */
	{ XT_REG_IDX_AR56, -1, 32 },		/* A56 */
	{ XT_REG_IDX_AR57, -1, 32 },		/* A57 */
	{ XT_REG_IDX_AR58, -1, 32 },		/* A58 */
	{ XT_REG_IDX_AR59, -1, 32 },		/* A59 */
	{ XT_REG_IDX_AR60, -1, 32 },		/* A60 */
	{ XT_REG_IDX_AR61, -1, 32 },		/* A61 */
	{ XT_REG_IDX_AR62, -1, 32 },		/* A62 */
	{ XT_REG_IDX_AR63, -1, 32 },		/* A63 */
	{ XT_REG_IDX_LBEG, 0x58, 32 },		/* lbeg */
	{ XT_REG_IDX_LEND, 0x5c, 32 },		/* lend */
	{ XT_REG_IDX_LCOUNT, 0x60, 32 },		/* lcount */
	{ XT_REG_IDX_SAR, 0x4c, 32 },		/* sar */
	{ XT_REG_IDX_WINDOWBASE, -1, 32 },		/* windowbase */
	{ XT_REG_IDX_WINDOWSTART, -1, 32 },		/* windowstart */
	{ XT_REG_IDX_CONFIGID0, -1, 32 },		/* configid0 */
	{ XT_REG_IDX_CONFIGID1, -1, 32 },		/* configid1 */
	{ XT_REG_IDX_PS, 0x08, 32 },		/* ps */
	{ XT_REG_IDX_THREADPTR, 0x70, 32 },		/* threadptr */
	{ XT_REG_IDX_BR, 0x7c, 32 },		/* br */
	{ XT_REG_IDX_SCOMPARE1, 0x80, 32 },		/* scompare1 */
	{ XT_REG_IDX_ACCLO, 0x74, 32 },		/* acclo */
	{ XT_REG_IDX_ACCHI, 0x78, 32 },		/* acchi */
	{ XT_REG_IDX_M0, 0x84, 32 },		/* m0 */
	{ XT_REG_IDX_M1, 0x88, 32 },		/* m1 */
	{ XT_REG_IDX_M2, 0x8c, 32 },		/* m2 */
	{ XT_REG_IDX_M3, 0x90, 32 },		/* m3 */
	{ ESP32_REG_IDX_EXPSTATE, -1, 32 },		/* expstate */
	{ ESP32_REG_IDX_F64R_LO, 0x94, 32 },		/* f64r_lo */
	{ ESP32_REG_IDX_F64R_HI, 0x98, 32 },		/* f64r_hi */
	{ ESP32_REG_IDX_F64S, 0x9c, 32 },		/* f64s */
	{ XT_REG_IDX_F0, -1, 32 },		/* f0 */
	{ XT_REG_IDX_F1, -1, 32 },		/* f1 */
	{ XT_REG_IDX_F2, -1, 32 },		/* f2 */
	{ XT_REG_IDX_F3, -1, 32 },		/* f3 */
	{ XT_REG_IDX_F4, -1, 32 },		/* f4 */
	{ XT_REG_IDX_F5, -1, 32 },		/* f5 */
	{ XT_REG_IDX_F6, -1, 32 },		/* f6 */
	{ XT_REG_IDX_F7, -1, 32 },		/* f7 */
	{ XT_REG_IDX_F8, -1, 32 },		/* f8 */
	{ XT_REG_IDX_F9, -1, 32 },		/* f9 */
	{ XT_REG_IDX_F10, -1, 32 },		/* f10 */
	{ XT_REG_IDX_F11, -1, 32 },		/* f11 */
	{ XT_REG_IDX_F12, -1, 32 },		/* f12 */
	{ XT_REG_IDX_F13, -1, 32 },		/* f13 */
	{ XT_REG_IDX_F14, -1, 32 },		/* f14 */
	{ XT_REG_IDX_F15, -1, 32 },		/* f15 */
	{ XT_REG_IDX_FCR, -1, 32 },		/* fcr */
	{ XT_REG_IDX_FSR, -1, 32 },		/* fsr */
};

static const struct stack_register_offset rtos_freertos_esp32_s2_stack_offsets[] = {
	{ XT_REG_IDX_PC, 0x04, 32 },		/* PC */
	{ XT_REG_IDX_AR0, 0x0c, 32 },		/* A0 */
	{ XT_REG_IDX_AR1, 0x10, 32 },		/* A1 */
	{ XT_REG_IDX_AR2, 0x14, 32 },		/* A2 */
	{ XT_REG_IDX_AR3, 0x18, 32 },		/* A3 */
	{ XT_REG_IDX_AR4, 0x1c, 32 },		/* A4 */
	{ XT_REG_IDX_AR5, 0x20, 32 },		/* A5 */
	{ XT_REG_IDX_AR6, 0x24, 32 },		/* A6 */
	{ XT_REG_IDX_AR7, 0x28, 32 },		/* A7 */
	{ XT_REG_IDX_AR8, 0x2c, 32 },		/* A8 */
	{ XT_REG_IDX_AR9, 0x30, 32 },		/* A9 */
	{ XT_REG_IDX_AR10, 0x34, 32 },		/* A10 */
	{ XT_REG_IDX_AR11, 0x38, 32 },		/* A11 */
	{ XT_REG_IDX_AR12, 0x3c, 32 },		/* A12 */
	{ XT_REG_IDX_AR13, 0x40, 32 },		/* A13 */
	{ XT_REG_IDX_AR14, 0x44, 32 },		/* A14 */
	{ XT_REG_IDX_AR15, 0x48, 32 },		/* A15 */
	{ XT_REG_IDX_AR16, -1, 32 },		/* A16 */ /* A16-A63 aren't in the stack frame because they've been flushed to the stack earlier */
	{ XT_REG_IDX_AR17, -1, 32 },		/* A17 */
	{ XT_REG_IDX_AR18, -1, 32 },		/* A18 */
	{ XT_REG_IDX_AR19, -1, 32 },		/* A19 */
	{ XT_REG_IDX_AR20, -1, 32 },		/* A20 */
	{ XT_REG_IDX_AR21, -1, 32 },		/* A21 */
	{ XT_REG_IDX_AR22, -1, 32 },		/* A22 */
	{ XT_REG_IDX_AR23, -1, 32 },		/* A23 */
	{ XT_REG_IDX_AR24, -1, 32 },		/* A24 */
	{ XT_REG_IDX_AR25, -1, 32 },		/* A25 */
	{ XT_REG_IDX_AR26, -1, 32 },		/* A26 */
	{ XT_REG_IDX_AR27, -1, 32 },		/* A27 */
	{ XT_REG_IDX_AR28, -1, 32 },		/* A28 */
	{ XT_REG_IDX_AR29, -1, 32 },		/* A29 */
	{ XT_REG_IDX_AR30, -1, 32 },		/* A30 */
	{ XT_REG_IDX_AR31, -1, 32 },		/* A31 */
	{ XT_REG_IDX_AR32, -1, 32 },		/* A32 */
	{ XT_REG_IDX_AR33, -1, 32 },		/* A33 */
	{ XT_REG_IDX_AR34, -1, 32 },		/* A34 */
	{ XT_REG_IDX_AR35, -1, 32 },		/* A35 */
	{ XT_REG_IDX_AR36, -1, 32 },		/* A36 */
	{ XT_REG_IDX_AR37, -1, 32 },		/* A37 */
	{ XT_REG_IDX_AR38, -1, 32 },		/* A38 */
	{ XT_REG_IDX_AR39, -1, 32 },		/* A39 */
	{ XT_REG_IDX_AR40, -1, 32 },		/* A40 */
	{ XT_REG_IDX_AR41, -1, 32 },		/* A41 */
	{ XT_REG_IDX_AR42, -1, 32 },		/* A42 */
	{ XT_REG_IDX_AR43, -1, 32 },		/* A43 */
	{ XT_REG_IDX_AR44, -1, 32 },		/* A44 */
	{ XT_REG_IDX_AR45, -1, 32 },		/* A45 */
	{ XT_REG_IDX_AR46, -1, 32 },		/* A46 */
	{ XT_REG_IDX_AR47, -1, 32 },		/* A47 */
	{ XT_REG_IDX_AR48, -1, 32 },		/* A48 */
	{ XT_REG_IDX_AR49, -1, 32 },		/* A49 */
	{ XT_REG_IDX_AR50, -1, 32 },		/* A50 */
	{ XT_REG_IDX_AR51, -1, 32 },		/* A51 */
	{ XT_REG_IDX_AR52, -1, 32 },		/* A52 */
	{ XT_REG_IDX_AR53, -1, 32 },		/* A53 */
	{ XT_REG_IDX_AR54, -1, 32 },		/* A54 */
	{ XT_REG_IDX_AR55, -1, 32 },		/* A55 */
	{ XT_REG_IDX_AR56, -1, 32 },		/* A56 */
	{ XT_REG_IDX_AR57, -1, 32 },		/* A57 */
	{ XT_REG_IDX_AR58, -1, 32 },		/* A58 */
	{ XT_REG_IDX_AR59, -1, 32 },		/* A59 */
	{ XT_REG_IDX_AR60, -1, 32 },		/* A60 */
	{ XT_REG_IDX_AR61, -1, 32 },		/* A61 */
	{ XT_REG_IDX_AR62, -1, 32 },		/* A62 */
	{ XT_REG_IDX_AR63, -1, 32 },		/* A63 */
	{ XT_REG_IDX_SAR, 0x4c, 32 },		/* sar */
	{ XT_REG_IDX_WINDOWBASE, -1, 32 },		/* windowbase */
	{ XT_REG_IDX_WINDOWSTART, -1, 32 },		/* windowstart */
	{ XT_REG_IDX_CONFIGID0, -1, 32 },		/* configid0 */
	{ XT_REG_IDX_CONFIGID1, -1, 32 },		/* configid1 */
	{ XT_REG_IDX_PS, 0x08, 32 },		/* ps */
	{ XT_REG_IDX_THREADPTR, 0x64, 32 },		/* threadptr */
	{ ESP32_S2_REG_IDX_GPIOOUT, -1, 32 },		/* gpio_out */
};

static const struct stack_register_offset rtos_freertos_esp32_s3_stack_offsets[] = {
	{ XT_REG_IDX_PC, 0x04, 32 },		/* PC */
	{ XT_REG_IDX_AR0, 0x0c, 32 },		/* A0 */
	{ XT_REG_IDX_AR1, 0x10, 32 },		/* A1 */
	{ XT_REG_IDX_AR2, 0x14, 32 },		/* A2 */
	{ XT_REG_IDX_AR3, 0x18, 32 },		/* A3 */
	{ XT_REG_IDX_AR4, 0x1c, 32 },		/* A4 */
	{ XT_REG_IDX_AR5, 0x20, 32 },		/* A5 */
	{ XT_REG_IDX_AR6, 0x24, 32 },		/* A6 */
	{ XT_REG_IDX_AR7, 0x28, 32 },		/* A7 */
	{ XT_REG_IDX_AR8, 0x2c, 32 },		/* A8 */
	{ XT_REG_IDX_AR9, 0x30, 32 },		/* A9 */
	{ XT_REG_IDX_AR10, 0x34, 32 },		/* A10 */
	{ XT_REG_IDX_AR11, 0x38, 32 },		/* A11 */
	{ XT_REG_IDX_AR12, 0x3c, 32 },		/* A12 */
	{ XT_REG_IDX_AR13, 0x40, 32 },		/* A13 */
	{ XT_REG_IDX_AR14, 0x44, 32 },		/* A14 */
	{ XT_REG_IDX_AR15, 0x48, 32 },		/* A15 */
	{ XT_REG_IDX_AR16, -1, 32 },		/* A16 */ /* A16-A63 aren't in the stack frame because they've been flushed to the stack earlier */
	{ XT_REG_IDX_AR17, -1, 32 },		/* A17 */
	{ XT_REG_IDX_AR18, -1, 32 },		/* A18 */
	{ XT_REG_IDX_AR19, -1, 32 },		/* A19 */
	{ XT_REG_IDX_AR20, -1, 32 },		/* A20 */
	{ XT_REG_IDX_AR21, -1, 32 },		/* A21 */
	{ XT_REG_IDX_AR22, -1, 32 },		/* A22 */
	{ XT_REG_IDX_AR23, -1, 32 },		/* A23 */
	{ XT_REG_IDX_AR24, -1, 32 },		/* A24 */
	{ XT_REG_IDX_AR25, -1, 32 },		/* A25 */
	{ XT_REG_IDX_AR26, -1, 32 },		/* A26 */
	{ XT_REG_IDX_AR27, -1, 32 },		/* A27 */
	{ XT_REG_IDX_AR28, -1, 32 },		/* A28 */
	{ XT_REG_IDX_AR29, -1, 32 },		/* A29 */
	{ XT_REG_IDX_AR30, -1, 32 },		/* A30 */
	{ XT_REG_IDX_AR31, -1, 32 },		/* A31 */
	{ XT_REG_IDX_AR32, -1, 32 },		/* A32 */
	{ XT_REG_IDX_AR33, -1, 32 },		/* A33 */
	{ XT_REG_IDX_AR34, -1, 32 },		/* A34 */
	{ XT_REG_IDX_AR35, -1, 32 },		/* A35 */
	{ XT_REG_IDX_AR36, -1, 32 },		/* A36 */
	{ XT_REG_IDX_AR37, -1, 32 },		/* A37 */
	{ XT_REG_IDX_AR38, -1, 32 },		/* A38 */
	{ XT_REG_IDX_AR39, -1, 32 },		/* A39 */
	{ XT_REG_IDX_AR40, -1, 32 },		/* A40 */
	{ XT_REG_IDX_AR41, -1, 32 },		/* A41 */
	{ XT_REG_IDX_AR42, -1, 32 },		/* A42 */
	{ XT_REG_IDX_AR43, -1, 32 },		/* A43 */
	{ XT_REG_IDX_AR44, -1, 32 },		/* A44 */
	{ XT_REG_IDX_AR45, -1, 32 },		/* A45 */
	{ XT_REG_IDX_AR46, -1, 32 },		/* A46 */
	{ XT_REG_IDX_AR47, -1, 32 },		/* A47 */
	{ XT_REG_IDX_AR48, -1, 32 },		/* A48 */
	{ XT_REG_IDX_AR49, -1, 32 },		/* A49 */
	{ XT_REG_IDX_AR50, -1, 32 },		/* A50 */
	{ XT_REG_IDX_AR51, -1, 32 },		/* A51 */
	{ XT_REG_IDX_AR52, -1, 32 },		/* A52 */
	{ XT_REG_IDX_AR53, -1, 32 },		/* A53 */
	{ XT_REG_IDX_AR54, -1, 32 },		/* A54 */
	{ XT_REG_IDX_AR55, -1, 32 },		/* A55 */
	{ XT_REG_IDX_AR56, -1, 32 },		/* A56 */
	{ XT_REG_IDX_AR57, -1, 32 },		/* A57 */
	{ XT_REG_IDX_AR58, -1, 32 },		/* A58 */
	{ XT_REG_IDX_AR59, -1, 32 },		/* A59 */
	{ XT_REG_IDX_AR60, -1, 32 },		/* A60 */
	{ XT_REG_IDX_AR61, -1, 32 },		/* A61 */
	{ XT_REG_IDX_AR62, -1, 32 },		/* A62 */
	{ XT_REG_IDX_AR63, -1, 32 },		/* A63 */
	{ XT_REG_IDX_LBEG, 0x58, 32 },		/* lbeg */
	{ XT_REG_IDX_LEND, 0x5c, 32 },		/* lend */
	{ XT_REG_IDX_LCOUNT, 0x60, 32 },	/* lcount */
	{ XT_REG_IDX_SAR, 0x4c, 32 },		/* sar */
	{ XT_REG_IDX_WINDOWBASE, -1, 32 },		/* windowbase */
	{ XT_REG_IDX_WINDOWSTART, -1, 32 },		/* windowstart */
	{ XT_REG_IDX_CONFIGID0, -1, 32 },		/* configid0 */
	{ XT_REG_IDX_CONFIGID1, -1, 32 },		/* configid1 */
	{ XT_REG_IDX_PS, 0x08, 32 },		/* ps */
	{ XT_REG_IDX_THREADPTR, 0x70, 32 },		/* threadptr */
	{ XT_REG_IDX_BR, 0x7c, 32 },		/* br */
	{ XT_REG_IDX_SCOMPARE1, 0x80, 32 },		/* scompare1 */
	{ XT_REG_IDX_ACCLO, 0x74, 32 },		/* acclo */
	{ XT_REG_IDX_ACCHI, 0x78, 32 },		/* acchi */
	{ XT_REG_IDX_M0, 0x84, 32 },		/* m0 */
	{ XT_REG_IDX_M1, 0x88, 32 },		/* m1 */
	{ XT_REG_IDX_M2, 0x8c, 32 },		/* m2 */
	{ XT_REG_IDX_M3, 0x90, 32 },		/* m3 */
	{ ESP32_S3_REG_IDX_GPIOOUT, -1, 32 },		/* gpio_out */
	{ XT_REG_IDX_F0, -1, 32 },		/* f0 */
	{ XT_REG_IDX_F1, -1, 32 },		/* f1 */
	{ XT_REG_IDX_F2, -1, 32 },		/* f2 */
	{ XT_REG_IDX_F3, -1, 32 },		/* f3 */
	{ XT_REG_IDX_F4, -1, 32 },		/* f4 */
	{ XT_REG_IDX_F5, -1, 32 },		/* f5 */
	{ XT_REG_IDX_F6, -1, 32 },		/* f6 */
	{ XT_REG_IDX_F7, -1, 32 },		/* f7 */
	{ XT_REG_IDX_F8, -1, 32 },		/* f8 */
	{ XT_REG_IDX_F9, -1, 32 },		/* f9 */
	{ XT_REG_IDX_F10, -1, 32 },		/* f10 */
	{ XT_REG_IDX_F11, -1, 32 },		/* f11 */
	{ XT_REG_IDX_F12, -1, 32 },		/* f12 */
	{ XT_REG_IDX_F13, -1, 32 },		/* f13 */
	{ XT_REG_IDX_F14, -1, 32 },		/* f14 */
	{ XT_REG_IDX_F15, -1, 32 },		/* f15 */
	{ XT_REG_IDX_FCR, -1, 32 },		/* fcr */
	{ XT_REG_IDX_FSR, -1, 32 },		/* fsr */
	{ ESP32_S3_REG_IDX_ACCX_0, -1, 32 },		/* accx_0 */
	{ ESP32_S3_REG_IDX_ACCX_1, -1, 32 },		/* accx_1 */
	{ ESP32_S3_REG_IDX_QACC_H_0, -1, 32 },		/* qacc_h_0 */
	{ ESP32_S3_REG_IDX_QACC_H_1, -1, 32 },		/* qacc_h_1 */
	{ ESP32_S3_REG_IDX_QACC_H_2, -1, 32 },		/* qacc_h_2 */
	{ ESP32_S3_REG_IDX_QACC_H_3, -1, 32 },		/* qacc_h_3 */
	{ ESP32_S3_REG_IDX_QACC_H_4, -1, 32 },		/* qacc_h_4 */
	{ ESP32_S3_REG_IDX_QACC_L_0, -1, 32 },		/* qacc_l_0 */
	{ ESP32_S3_REG_IDX_QACC_L_1, -1, 32 },		/* qacc_l_1 */
	{ ESP32_S3_REG_IDX_QACC_L_2, -1, 32 },		/* qacc_l_2 */
	{ ESP32_S3_REG_IDX_QACC_L_3, -1, 32 },		/* qacc_l_3 */
	{ ESP32_S3_REG_IDX_QACC_L_4, -1, 32 },		/* qacc_l_4 */
	{ ESP32_S3_REG_IDX_SAR_BYTE, -1, 32 },			/* sar_byte */
	{ ESP32_S3_REG_IDX_FFT_BIT_WIDTH, -1, 32 },		/* fft_bit_width */
	{ ESP32_S3_REG_IDX_UA_STATE_0, -1, 32 },		/* ua_state_0 */
	{ ESP32_S3_REG_IDX_UA_STATE_1, -1, 32 },		/* ua_state_1 */
	{ ESP32_S3_REG_IDX_UA_STATE_2, -1, 32 },		/* ua_state_2 */
	{ ESP32_S3_REG_IDX_UA_STATE_3, -1, 32 },		/* ua_state_3 */
	{ ESP32_S3_REG_IDX_Q0, -1, 128 },		/* q0 */
	{ ESP32_S3_REG_IDX_Q1, -1, 128 },		/* q1 */
	{ ESP32_S3_REG_IDX_Q2, -1, 128 },		/* q2 */
	{ ESP32_S3_REG_IDX_Q3, -1, 128 },		/* q3 */
	{ ESP32_S3_REG_IDX_Q4, -1, 128 },		/* q4 */
	{ ESP32_S3_REG_IDX_Q5, -1, 128 },		/* q5 */
	{ ESP32_S3_REG_IDX_Q6, -1, 128 },		/* q6 */
	{ ESP32_S3_REG_IDX_Q7, -1, 128 },		/* q7 */
};

//WARNING: There's some deeper magic going on when reading this. Please
//refer to rtos_freertos_esp_xtensa_stack_read_voluntary for more info.

static const struct stack_register_offset rtos_freertos_esp32_voluntary_stack_offsets[] = {
	{ XT_REG_IDX_PC, 0x14, 32 },		/* PC */
	{ XT_REG_IDX_AR0, 0x00, 32 },		/* A0 */
	{ XT_REG_IDX_AR1, 0x04, 32 },		/* A1 */
	{ XT_REG_IDX_AR2, 0x08, 32 },		/* A2 */
	{ XT_REG_IDX_AR3, 0x0c, 32 },		/* A3 */
	{ XT_REG_IDX_AR4, 0x30, 32 },		/* A4 */
	{ XT_REG_IDX_AR5, 0x34, 32 },		/* A5 */
	{ XT_REG_IDX_AR6, 0x38, 32 },		/* A6 */
	{ XT_REG_IDX_AR7, 0x3c, 32 },		/* A7 */
	{ XT_REG_IDX_AR8, 0x40, 32 },		/* A8 */
	{ XT_REG_IDX_AR9, 0x44, 32 },		/* A9 */
	{ XT_REG_IDX_AR10, 0x48, 32 },		/* A10 */
	{ XT_REG_IDX_AR11, 0x4c, 32 },		/* A11 */
	{ XT_REG_IDX_AR12, -1, 32 },		/* A12 */ /* A12-A63 aren't in the stack frame because they've been flushed to the stack earlier or not relevant*/
	{ XT_REG_IDX_AR13, -1, 32 },		/* A13 */
	{ XT_REG_IDX_AR14, -1, 32 },		/* A14 */
	{ XT_REG_IDX_AR15, -1, 32 },		/* A15 */
	{ XT_REG_IDX_AR16, -1, 32 },		/* A16 */
	{ XT_REG_IDX_AR17, -1, 32 },		/* A17 */
	{ XT_REG_IDX_AR18, -1, 32 },		/* A18 */
	{ XT_REG_IDX_AR19, -1, 32 },		/* A19 */
	{ XT_REG_IDX_AR20, -1, 32 },		/* A20 */
	{ XT_REG_IDX_AR21, -1, 32 },		/* A21 */
	{ XT_REG_IDX_AR22, -1, 32 },		/* A22 */
	{ XT_REG_IDX_AR23, -1, 32 },		/* A23 */
	{ XT_REG_IDX_AR24, -1, 32 },		/* A24 */
	{ XT_REG_IDX_AR25, -1, 32 },		/* A25 */
	{ XT_REG_IDX_AR26, -1, 32 },		/* A26 */
	{ XT_REG_IDX_AR27, -1, 32 },		/* A27 */
	{ XT_REG_IDX_AR28, -1, 32 },		/* A28 */
	{ XT_REG_IDX_AR29, -1, 32 },		/* A29 */
	{ XT_REG_IDX_AR30, -1, 32 },		/* A30 */
	{ XT_REG_IDX_AR31, -1, 32 },		/* A31 */
	{ XT_REG_IDX_AR32, -1, 32 },		/* A32 */
	{ XT_REG_IDX_AR33, -1, 32 },		/* A33 */
	{ XT_REG_IDX_AR34, -1, 32 },		/* A34 */
	{ XT_REG_IDX_AR35, -1, 32 },		/* A35 */
	{ XT_REG_IDX_AR36, -1, 32 },		/* A36 */
	{ XT_REG_IDX_AR37, -1, 32 },		/* A37 */
	{ XT_REG_IDX_AR38, -1, 32 },		/* A38 */
	{ XT_REG_IDX_AR39, -1, 32 },		/* A39 */
	{ XT_REG_IDX_AR40, -1, 32 },		/* A40 */
	{ XT_REG_IDX_AR41, -1, 32 },		/* A41 */
	{ XT_REG_IDX_AR42, -1, 32 },		/* A42 */
	{ XT_REG_IDX_AR43, -1, 32 },		/* A43 */
	{ XT_REG_IDX_AR44, -1, 32 },		/* A44 */
	{ XT_REG_IDX_AR45, -1, 32 },		/* A45 */
	{ XT_REG_IDX_AR46, -1, 32 },		/* A46 */
	{ XT_REG_IDX_AR47, -1, 32 },		/* A47 */
	{ XT_REG_IDX_AR48, -1, 32 },		/* A48 */
	{ XT_REG_IDX_AR49, -1, 32 },		/* A49 */
	{ XT_REG_IDX_AR50, -1, 32 },		/* A50 */
	{ XT_REG_IDX_AR51, -1, 32 },		/* A51 */
	{ XT_REG_IDX_AR52, -1, 32 },		/* A52 */
	{ XT_REG_IDX_AR53, -1, 32 },		/* A53 */
	{ XT_REG_IDX_AR54, -1, 32 },		/* A54 */
	{ XT_REG_IDX_AR55, -1, 32 },		/* A55 */
	{ XT_REG_IDX_AR56, -1, 32 },		/* A56 */
	{ XT_REG_IDX_AR57, -1, 32 },		/* A57 */
	{ XT_REG_IDX_AR58, -1, 32 },		/* A58 */
	{ XT_REG_IDX_AR59, -1, 32 },		/* A59 */
	{ XT_REG_IDX_AR60, -1, 32 },		/* A60 */
	{ XT_REG_IDX_AR61, -1, 32 },		/* A61 */
	{ XT_REG_IDX_AR62, -1, 32 },		/* A62 */
	{ XT_REG_IDX_AR63, -1, 32 },		/* A63 */
	{ XT_REG_IDX_LBEG, -1, 32 },		/* lbeg */
	{ XT_REG_IDX_LEND, -1, 32 },		/* lend */
	{ XT_REG_IDX_LCOUNT, -1, 32 },		/* lcount */
	{ XT_REG_IDX_SAR, -1, 32 },		/* sar */
	{ XT_REG_IDX_WINDOWBASE, -1, 32 },		/* windowbase */
	{ XT_REG_IDX_WINDOWSTART, -1, 32 },		/* windowstart */
	{ XT_REG_IDX_CONFIGID0, -1, 32 },		/* configid0 */
	{ XT_REG_IDX_CONFIGID1, -1, 32 },		/* configid1 */
	{ XT_REG_IDX_PS, 0x18, 32 },		/* ps */
	{ XT_REG_IDX_THREADPTR, -1, 32 },		/* threadptr */
	{ XT_REG_IDX_BR, -1, 32 },		/* br */
	{ XT_REG_IDX_SCOMPARE1, -1, 32 },		/* scompare1 */
	{ XT_REG_IDX_ACCLO, -1, 32 },		/* acclo */
	{ XT_REG_IDX_ACCHI, -1, 32 },		/* acchi */
	{ XT_REG_IDX_M0, -1, 32 },		/* m0 */
	{ XT_REG_IDX_M1, -1, 32 },		/* m1 */
	{ XT_REG_IDX_M2, -1, 32 },		/* m2 */
	{ XT_REG_IDX_M3, -1, 32 },		/* m3 */
	{ ESP32_REG_IDX_EXPSTATE, -1, 32 },		/* expstate */
	{ ESP32_REG_IDX_F64R_LO, -1, 32 },		/* f64r_lo */
	{ ESP32_REG_IDX_F64R_HI, -1, 32 },		/* f64r_hi */
	{ ESP32_REG_IDX_F64S, -1, 32 },		/* f64s */
	{ XT_REG_IDX_F0, -1, 32 },		/* f0 */
	{ XT_REG_IDX_F1, -1, 32 },		/* f1 */
	{ XT_REG_IDX_F2, -1, 32 },		/* f2 */
	{ XT_REG_IDX_F3, -1, 32 },		/* f3 */
	{ XT_REG_IDX_F4, -1, 32 },		/* f4 */
	{ XT_REG_IDX_F5, -1, 32 },		/* f5 */
	{ XT_REG_IDX_F6, -1, 32 },		/* f6 */
	{ XT_REG_IDX_F7, -1, 32 },		/* f7 */
	{ XT_REG_IDX_F8, -1, 32 },		/* f8 */
	{ XT_REG_IDX_F9, -1, 32 },		/* f9 */
	{ XT_REG_IDX_F10, -1, 32 },		/* f10 */
	{ XT_REG_IDX_F11, -1, 32 },		/* f11 */
	{ XT_REG_IDX_F12, -1, 32 },		/* f12 */
	{ XT_REG_IDX_F13, -1, 32 },		/* f13 */
	{ XT_REG_IDX_F14, -1, 32 },		/* f14 */
	{ XT_REG_IDX_F15, -1, 32 },		/* f15 */
	{ XT_REG_IDX_FCR, -1, 32 },		/* fcr */
	{ XT_REG_IDX_FSR, -1, 32 },		/* fsr */
};


static const struct stack_register_offset rtos_freertos_esp32_s2_voluntary_stack_offsets[] = {
	{ XT_REG_IDX_PC, 0x14, 32 },		/* PC */
	{ XT_REG_IDX_AR0, 0x00, 32 },		/* A0 */
	{ XT_REG_IDX_AR1, 0x04, 32 },		/* A1 */
	{ XT_REG_IDX_AR2, 0x08, 32 },		/* A2 */
	{ XT_REG_IDX_AR3, 0x0c, 32 },		/* A3 */
	{ XT_REG_IDX_AR4, 0x30, 32 },		/* A4 */
	{ XT_REG_IDX_AR5, 0x34, 32 },		/* A5 */
	{ XT_REG_IDX_AR6, 0x38, 32 },		/* A6 */
	{ XT_REG_IDX_AR7, 0x3c, 32 },		/* A7 */
	{ XT_REG_IDX_AR8, 0x40, 32 },		/* A8 */
	{ XT_REG_IDX_AR9, 0x44, 32 },		/* A9 */
	{ XT_REG_IDX_AR10, 0x48, 32 },		/* A10 */
	{ XT_REG_IDX_AR11, 0x4c, 32 },		/* A11 */
	{ XT_REG_IDX_AR12, -1, 32 },		/* A12 */ /* A12-A63 aren't in the stack frame because they've been flushed to the stack earlier or not relevant*/
	{ XT_REG_IDX_AR13, -1, 32 },		/* A13 */
	{ XT_REG_IDX_AR14, -1, 32 },		/* A14 */
	{ XT_REG_IDX_AR15, -1, 32 },		/* A15 */
	{ XT_REG_IDX_AR16, -1, 32 },		/* A16 */
	{ XT_REG_IDX_AR17, -1, 32 },		/* A17 */
	{ XT_REG_IDX_AR18, -1, 32 },		/* A18 */
	{ XT_REG_IDX_AR19, -1, 32 },		/* A19 */
	{ XT_REG_IDX_AR20, -1, 32 },		/* A20 */
	{ XT_REG_IDX_AR21, -1, 32 },		/* A21 */
	{ XT_REG_IDX_AR22, -1, 32 },		/* A22 */
	{ XT_REG_IDX_AR23, -1, 32 },		/* A23 */
	{ XT_REG_IDX_AR24, -1, 32 },		/* A24 */
	{ XT_REG_IDX_AR25, -1, 32 },		/* A25 */
	{ XT_REG_IDX_AR26, -1, 32 },		/* A26 */
	{ XT_REG_IDX_AR27, -1, 32 },		/* A27 */
	{ XT_REG_IDX_AR28, -1, 32 },		/* A28 */
	{ XT_REG_IDX_AR29, -1, 32 },		/* A29 */
	{ XT_REG_IDX_AR30, -1, 32 },		/* A30 */
	{ XT_REG_IDX_AR31, -1, 32 },		/* A31 */
	{ XT_REG_IDX_AR32, -1, 32 },		/* A32 */
	{ XT_REG_IDX_AR33, -1, 32 },		/* A33 */
	{ XT_REG_IDX_AR34, -1, 32 },		/* A34 */
	{ XT_REG_IDX_AR35, -1, 32 },		/* A35 */
	{ XT_REG_IDX_AR36, -1, 32 },		/* A36 */
	{ XT_REG_IDX_AR37, -1, 32 },		/* A37 */
	{ XT_REG_IDX_AR38, -1, 32 },		/* A38 */
	{ XT_REG_IDX_AR39, -1, 32 },		/* A39 */
	{ XT_REG_IDX_AR40, -1, 32 },		/* A40 */
	{ XT_REG_IDX_AR41, -1, 32 },		/* A41 */
	{ XT_REG_IDX_AR42, -1, 32 },		/* A42 */
	{ XT_REG_IDX_AR43, -1, 32 },		/* A43 */
	{ XT_REG_IDX_AR44, -1, 32 },		/* A44 */
	{ XT_REG_IDX_AR45, -1, 32 },		/* A45 */
	{ XT_REG_IDX_AR46, -1, 32 },		/* A46 */
	{ XT_REG_IDX_AR47, -1, 32 },		/* A47 */
	{ XT_REG_IDX_AR48, -1, 32 },		/* A48 */
	{ XT_REG_IDX_AR49, -1, 32 },		/* A49 */
	{ XT_REG_IDX_AR50, -1, 32 },		/* A50 */
	{ XT_REG_IDX_AR51, -1, 32 },		/* A51 */
	{ XT_REG_IDX_AR52, -1, 32 },		/* A52 */
	{ XT_REG_IDX_AR53, -1, 32 },		/* A53 */
	{ XT_REG_IDX_AR54, -1, 32 },		/* A54 */
	{ XT_REG_IDX_AR55, -1, 32 },		/* A55 */
	{ XT_REG_IDX_AR56, -1, 32 },		/* A56 */
	{ XT_REG_IDX_AR57, -1, 32 },		/* A57 */
	{ XT_REG_IDX_AR58, -1, 32 },		/* A58 */
	{ XT_REG_IDX_AR59, -1, 32 },		/* A59 */
	{ XT_REG_IDX_AR60, -1, 32 },		/* A60 */
	{ XT_REG_IDX_AR61, -1, 32 },		/* A61 */
	{ XT_REG_IDX_AR62, -1, 32 },		/* A62 */
	{ XT_REG_IDX_AR63, -1, 32 },		/* A63 */
	{ XT_REG_IDX_SAR, -1, 32 },		/* sar */
	{ XT_REG_IDX_WINDOWBASE, -1, 32 },		/* windowbase */
	{ XT_REG_IDX_WINDOWSTART, -1, 32 },		/* windowstart */
	{ XT_REG_IDX_CONFIGID0, -1, 32 },		/* configid0 */
	{ XT_REG_IDX_CONFIGID1, -1, 32 },		/* configid1 */
	{ XT_REG_IDX_PS, 0x18, 32 },		/* ps */
	{ XT_REG_IDX_THREADPTR, -1, 32 },		/* threadptr */
	{ ESP32_S2_REG_IDX_GPIOOUT, -1, 32 },		/* gpio_out */
};

static const struct stack_register_offset rtos_freertos_esp32_s3_voluntary_stack_offsets[] = {
	{ XT_REG_IDX_PC, 0x14, 32 },		/* PC */
	{ XT_REG_IDX_AR0, 0x00, 32 },		/* A0 */
	{ XT_REG_IDX_AR1, 0x04, 32 },		/* A1 */
	{ XT_REG_IDX_AR2, 0x08, 32 },		/* A2 */
	{ XT_REG_IDX_AR3, 0x0c, 32 },		/* A3 */
	{ XT_REG_IDX_AR4, 0x30, 32 },		/* A4 */
	{ XT_REG_IDX_AR5, 0x34, 32 },		/* A5 */
	{ XT_REG_IDX_AR6, 0x38, 32 },		/* A6 */
	{ XT_REG_IDX_AR7, 0x3c, 32 },		/* A7 */
	{ XT_REG_IDX_AR8, 0x40, 32 },		/* A8 */
	{ XT_REG_IDX_AR9, 0x44, 32 },		/* A9 */
	{ XT_REG_IDX_AR10, 0x48, 32 },		/* A10 */
	{ XT_REG_IDX_AR11, 0x4c, 32 },		/* A11 */
	{ XT_REG_IDX_AR12, -1, 32 },		/* A12 */ /* A12-A63 aren't in the stack frame because they've been flushed to the stack earlier or not relevant*/
	{ XT_REG_IDX_AR13, -1, 32 },		/* A13 */
	{ XT_REG_IDX_AR14, -1, 32 },		/* A14 */
	{ XT_REG_IDX_AR15, -1, 32 },		/* A15 */
	{ XT_REG_IDX_AR16, -1, 32 },		/* A16 */
	{ XT_REG_IDX_AR17, -1, 32 },		/* A17 */
	{ XT_REG_IDX_AR18, -1, 32 },		/* A18 */
	{ XT_REG_IDX_AR19, -1, 32 },		/* A19 */
	{ XT_REG_IDX_AR20, -1, 32 },		/* A20 */
	{ XT_REG_IDX_AR21, -1, 32 },		/* A21 */
	{ XT_REG_IDX_AR22, -1, 32 },		/* A22 */
	{ XT_REG_IDX_AR23, -1, 32 },		/* A23 */
	{ XT_REG_IDX_AR24, -1, 32 },		/* A24 */
	{ XT_REG_IDX_AR25, -1, 32 },		/* A25 */
	{ XT_REG_IDX_AR26, -1, 32 },		/* A26 */
	{ XT_REG_IDX_AR27, -1, 32 },		/* A27 */
	{ XT_REG_IDX_AR28, -1, 32 },		/* A28 */
	{ XT_REG_IDX_AR29, -1, 32 },		/* A29 */
	{ XT_REG_IDX_AR30, -1, 32 },		/* A30 */
	{ XT_REG_IDX_AR31, -1, 32 },		/* A31 */
	{ XT_REG_IDX_AR32, -1, 32 },		/* A32 */
	{ XT_REG_IDX_AR33, -1, 32 },		/* A33 */
	{ XT_REG_IDX_AR34, -1, 32 },		/* A34 */
	{ XT_REG_IDX_AR35, -1, 32 },		/* A35 */
	{ XT_REG_IDX_AR36, -1, 32 },		/* A36 */
	{ XT_REG_IDX_AR37, -1, 32 },		/* A37 */
	{ XT_REG_IDX_AR38, -1, 32 },		/* A38 */
	{ XT_REG_IDX_AR39, -1, 32 },		/* A39 */
	{ XT_REG_IDX_AR40, -1, 32 },		/* A40 */
	{ XT_REG_IDX_AR41, -1, 32 },		/* A41 */
	{ XT_REG_IDX_AR42, -1, 32 },		/* A42 */
	{ XT_REG_IDX_AR43, -1, 32 },		/* A43 */
	{ XT_REG_IDX_AR44, -1, 32 },		/* A44 */
	{ XT_REG_IDX_AR45, -1, 32 },		/* A45 */
	{ XT_REG_IDX_AR46, -1, 32 },		/* A46 */
	{ XT_REG_IDX_AR47, -1, 32 },		/* A47 */
	{ XT_REG_IDX_AR48, -1, 32 },		/* A48 */
	{ XT_REG_IDX_AR49, -1, 32 },		/* A49 */
	{ XT_REG_IDX_AR50, -1, 32 },		/* A50 */
	{ XT_REG_IDX_AR51, -1, 32 },		/* A51 */
	{ XT_REG_IDX_AR52, -1, 32 },		/* A52 */
	{ XT_REG_IDX_AR53, -1, 32 },		/* A53 */
	{ XT_REG_IDX_AR54, -1, 32 },		/* A54 */
	{ XT_REG_IDX_AR55, -1, 32 },		/* A55 */
	{ XT_REG_IDX_AR56, -1, 32 },		/* A56 */
	{ XT_REG_IDX_AR57, -1, 32 },		/* A57 */
	{ XT_REG_IDX_AR58, -1, 32 },		/* A58 */
	{ XT_REG_IDX_AR59, -1, 32 },		/* A59 */
	{ XT_REG_IDX_AR60, -1, 32 },		/* A60 */
	{ XT_REG_IDX_AR61, -1, 32 },		/* A61 */
	{ XT_REG_IDX_AR62, -1, 32 },		/* A62 */
	{ XT_REG_IDX_AR63, -1, 32 },		/* A63 */
	{ XT_REG_IDX_LBEG, -1, 32 },		/* lbeg */
	{ XT_REG_IDX_LEND, -1, 32 },		/* lend */
	{ XT_REG_IDX_LCOUNT, -1, 32 },		/* lcount */
	{ XT_REG_IDX_SAR, -1, 32 },		/* sar */
	{ XT_REG_IDX_WINDOWBASE, -1, 32 },		/* windowbase */
	{ XT_REG_IDX_WINDOWSTART, -1, 32 },		/* windowstart */
	{ XT_REG_IDX_CONFIGID0, -1, 32 },		/* configid0 */
	{ XT_REG_IDX_CONFIGID1, -1, 32 },		/* configid1 */
	{ XT_REG_IDX_PS, 0x18, 32 },		/* ps */
	{ XT_REG_IDX_THREADPTR, -1, 32 },		/* threadptr */
	{ XT_REG_IDX_BR, -1, 32 },		/* br */
	{ XT_REG_IDX_SCOMPARE1, -1, 32 },		/* scompare1 */
	{ XT_REG_IDX_ACCLO, -1, 32 },		/* acclo */
	{ XT_REG_IDX_ACCHI, -1, 32 },		/* acchi */
	{ XT_REG_IDX_M0, -1, 32 },		/* m0 */
	{ XT_REG_IDX_M1, -1, 32 },		/* m1 */
	{ XT_REG_IDX_M2, -1, 32 },		/* m2 */
	{ XT_REG_IDX_M3, -1, 32 },		/* m3 */
	{ ESP32_S3_REG_IDX_GPIOOUT, -1, 32 },		/* gpio_out */
	{ XT_REG_IDX_F0, -1, 32 },		/* f0 */
	{ XT_REG_IDX_F1, -1, 32 },		/* f1 */
	{ XT_REG_IDX_F2, -1, 32 },		/* f2 */
	{ XT_REG_IDX_F3, -1, 32 },		/* f3 */
	{ XT_REG_IDX_F4, -1, 32 },		/* f4 */
	{ XT_REG_IDX_F5, -1, 32 },		/* f5 */
	{ XT_REG_IDX_F6, -1, 32 },		/* f6 */
	{ XT_REG_IDX_F7, -1, 32 },		/* f7 */
	{ XT_REG_IDX_F8, -1, 32 },		/* f8 */
	{ XT_REG_IDX_F9, -1, 32 },		/* f9 */
	{ XT_REG_IDX_F10, -1, 32 },		/* f10 */
	{ XT_REG_IDX_F11, -1, 32 },		/* f11 */
	{ XT_REG_IDX_F12, -1, 32 },		/* f12 */
	{ XT_REG_IDX_F13, -1, 32 },		/* f13 */
	{ XT_REG_IDX_F14, -1, 32 },		/* f14 */
	{ XT_REG_IDX_F15, -1, 32 },		/* f15 */
	{ XT_REG_IDX_FCR, -1, 32 },		/* fcr */
	{ XT_REG_IDX_FSR, -1, 32 },		/* fsr */
	{ ESP32_S3_REG_IDX_ACCX_0, -1, 32 },		/* accx_0 */
	{ ESP32_S3_REG_IDX_ACCX_1, -1, 32 },		/* accx_1 */
	{ ESP32_S3_REG_IDX_QACC_H_0, -1, 32 },		/* qacc_h_0 */
	{ ESP32_S3_REG_IDX_QACC_H_1, -1, 32 },		/* qacc_h_1 */
	{ ESP32_S3_REG_IDX_QACC_H_2, -1, 32 },		/* qacc_h_2 */
	{ ESP32_S3_REG_IDX_QACC_H_3, -1, 32 },		/* qacc_h_3 */
	{ ESP32_S3_REG_IDX_QACC_H_4, -1, 32 },		/* qacc_h_4 */
	{ ESP32_S3_REG_IDX_QACC_L_0, -1, 32 },		/* qacc_l_0 */
	{ ESP32_S3_REG_IDX_QACC_L_1, -1, 32 },		/* qacc_l_1 */
	{ ESP32_S3_REG_IDX_QACC_L_2, -1, 32 },		/* qacc_l_2 */
	{ ESP32_S3_REG_IDX_QACC_L_3, -1, 32 },		/* qacc_l_3 */
	{ ESP32_S3_REG_IDX_QACC_L_4, -1, 32 },		/* qacc_l_4 */
	{ ESP32_S3_REG_IDX_SAR_BYTE, -1, 32 },			/* sar_byte */
	{ ESP32_S3_REG_IDX_FFT_BIT_WIDTH, -1, 32 },		/* fft_bit_width */
	{ ESP32_S3_REG_IDX_UA_STATE_0, -1, 32 },		/* ua_state_0 */
	{ ESP32_S3_REG_IDX_UA_STATE_1, -1, 32 },		/* ua_state_1 */
	{ ESP32_S3_REG_IDX_UA_STATE_2, -1, 32 },		/* ua_state_2 */
	{ ESP32_S3_REG_IDX_UA_STATE_3, -1, 32 },		/* ua_state_3 */
	{ ESP32_S3_REG_IDX_Q0, -1, 128 },		/* q0 */
	{ ESP32_S3_REG_IDX_Q1, -1, 128 },		/* q1 */
	{ ESP32_S3_REG_IDX_Q2, -1, 128 },		/* q2 */
	{ ESP32_S3_REG_IDX_Q3, -1, 128 },		/* q3 */
	{ ESP32_S3_REG_IDX_Q4, -1, 128 },		/* q4 */
	{ ESP32_S3_REG_IDX_Q5, -1, 128 },		/* q5 */
	{ ESP32_S3_REG_IDX_Q6, -1, 128 },		/* q6 */
	{ ESP32_S3_REG_IDX_Q7, -1, 128 },		/* q7 */
};

const struct rtos_register_stacking rtos_freertos_esp32_stacking = {
	30*4,				/* stack_registers_size */
	-1,					/* stack_growth_direction */
	ESP32_NUM_REGS_G_COMMAND,					/* num_output_registers */
	rtos_generic_stack_align8,	/* stack_alignment */
	rtos_freertos_esp32_stack_offsets,		/* register_offsets */
	rtos_freertos_esp_xtensa_stack_read_involuntary		/* Custom stack frame read function */
};

const struct rtos_register_stacking rtos_freertos_esp32_s2_stacking = {
	30*4,				/* stack_registers_size */
	-1,					/* stack_growth_direction */
	ESP32_S2_NUM_REGS_G_COMMAND,					/* num_output_registers */
	rtos_generic_stack_align8,	/* stack_alignment */
	rtos_freertos_esp32_s2_stack_offsets,		/* register_offsets */
	rtos_freertos_esp_xtensa_stack_read_involuntary		/* Custom stack frame read function */
};

const struct rtos_register_stacking rtos_freertos_esp32_s3_stacking = {
	30*4,				/* stack_registers_size */
	-1,					/* stack_growth_direction */
	ESP32_S3_NUM_REGS_G_COMMAND,					/* num_output_registers */
	rtos_generic_stack_align8,	/* stack_alignment */
	rtos_freertos_esp32_s3_stack_offsets,		/* register_offsets */
	rtos_freertos_esp_xtensa_stack_read_involuntary		/* Custom stack frame read function */
};

const struct rtos_register_stacking rtos_freertos_voluntary_esp32_stacking = {
	0x50,				/* stack_registers_size, including 'faked' stack values */
	-1,					/* stack_growth_direction */
	ESP32_NUM_REGS_G_COMMAND,					/* num_output_registers */
	rtos_generic_stack_align8,	/* stack_alignment */
	rtos_freertos_esp32_voluntary_stack_offsets,	/* register_offsets */
	rtos_freertos_esp_xtensa_stack_read_voluntary		/* Custom stack frame read function */
};

const struct rtos_register_stacking rtos_freertos_voluntary_esp32_s2_stacking = {
	0x50,				/* stack_registers_size, including 'faked' stack values */
	-1,					/* stack_growth_direction */
	ESP32_S2_NUM_REGS_G_COMMAND,					/* num_output_registers */
	rtos_generic_stack_align8,	/* stack_alignment */
	rtos_freertos_esp32_s2_voluntary_stack_offsets,	/* register_offsets */
	rtos_freertos_esp_xtensa_stack_read_voluntary		/* Custom stack frame read function */
};

const struct rtos_register_stacking rtos_freertos_voluntary_esp32_s3_stacking = {
	0x50,				/* stack_registers_size, including 'faked' stack values */
	-1,					/* stack_growth_direction */
	ESP32_S3_NUM_REGS_G_COMMAND,					/* num_output_registers */
	rtos_generic_stack_align8,	/* stack_alignment */
	rtos_freertos_esp32_s3_voluntary_stack_offsets,	/* register_offsets */
	rtos_freertos_esp_xtensa_stack_read_voluntary		/* Custom stack frame read function */
};

static const struct stack_register_offset rtos_freertos_riscv_stack_offsets[] = {
	{ GDB_REGNO_ZERO, -1, 32 },
	{ GDB_REGNO_RA, 0x04, 32 },
	{ GDB_REGNO_A0, 0x08, 32 },
	{ GDB_REGNO_A1, 0x0C, 32 },
	{ GDB_REGNO_A2, 0x10, 32 },
	{ GDB_REGNO_A3, 0x14, 32 },
	{ GDB_REGNO_A4, 0x18, 32 },
	{ GDB_REGNO_A5, 0x1C, 32 },
	{ GDB_REGNO_A6, 0x20, 32 },
	{ GDB_REGNO_A7, 0x24, 32 },
	{ GDB_REGNO_T0, 0x28, 32 },
	{ GDB_REGNO_T1, 0x2C, 32 },
	{ GDB_REGNO_T2, 0x30, 32 },
	{ GDB_REGNO_T3, 0x34, 32 },
	{ GDB_REGNO_T4, 0x38, 32 },
	{ GDB_REGNO_T5, 0x3C, 32 },
	{ GDB_REGNO_T6, 0x40, 32 },
	{ GDB_REGNO_SP, 0x44, 32 },
	{ GDB_REGNO_GP, 0x48, 32 },
	{ GDB_REGNO_TP, 0x4C, 32 },
	{ GDB_REGNO_FP, 0x50, 32 },
	{ GDB_REGNO_S1, 0x54, 32 },
	{ GDB_REGNO_S2, 0x58, 32 },
	{ GDB_REGNO_S3, 0x5C, 32 },
	{ GDB_REGNO_S4, 0x60, 32 },
	{ GDB_REGNO_S5, 0x64, 32 },
	{ GDB_REGNO_S6, 0x68, 32 },
	{ GDB_REGNO_S7, 0x6C, 32 },
	{ GDB_REGNO_S8, 0x70, 32 },
	{ GDB_REGNO_S9, 0x74, 32 },
	{ GDB_REGNO_S10, 0x78, 32 },
	{ GDB_REGNO_S11, 0x7C, 32 },
	{ GDB_REGNO_PC, 0x00, 32 },
};

const struct rtos_register_stacking rtos_freertos_riscv_stacking = {
	32*4,				/* stack_registers_size */
	-1,					/* stack_growth_direction */
	33,					/* num_output_registers */
	rtos_generic_stack_align8,	/* stack_alignment */
	rtos_freertos_riscv_stack_offsets,		/* register_offsets */
	NULL		/* Custom stack frame read function */
};

/*
 This function uses the first word of the stack memory to see if the stack frame is from a
 voluntary or unvoluntary yield, and returns the correct stack frame info.
*/
static const struct rtos_register_stacking *rtos_freertos_esp_xtensa_pick_stacking_info(struct rtos *rtos, int64_t thread_id, int64_t stack_addr,
												const struct rtos_register_stacking *stacking, const struct rtos_register_stacking *voluntary_stacking)
{
	int retval;
	uint32_t stack_ptr = 0;
	uint32_t stk_exit;

	if (rtos == NULL)
		return stacking;

	if (thread_id == 0)
		return stacking;

	/* Read the stack pointer */
	retval = target_read_buffer(rtos->target,
			stack_addr,
			4,
			(uint8_t *)&stack_ptr);

	/* Read the XT_STK_EXIT variable */
	if (retval != ERROR_OK) {
		return stacking;
	}
	retval = target_read_buffer(rtos->target,
			stack_ptr,
			4,
			(uint8_t *)&stk_exit);

	if (stk_exit) {
		return stacking;
	} else {
		return voluntary_stacking;
	}
}

const struct rtos_register_stacking *rtos_freertos_esp32_pick_stacking_info(struct rtos *rtos, int64_t thread_id, int64_t stack_addr)
{
	return rtos_freertos_esp_xtensa_pick_stacking_info(rtos, thread_id, stack_addr, &rtos_freertos_esp32_stacking, &rtos_freertos_voluntary_esp32_stacking);
}

const struct rtos_register_stacking *rtos_freertos_esp32_s2_pick_stacking_info(struct rtos *rtos, int64_t thread_id, int64_t stack_addr)
{
	return rtos_freertos_esp_xtensa_pick_stacking_info(rtos, thread_id, stack_addr, &rtos_freertos_esp32_s2_stacking, &rtos_freertos_voluntary_esp32_s2_stacking);
}

const struct rtos_register_stacking *rtos_freertos_esp32_s3_pick_stacking_info(struct rtos *rtos, int64_t thread_id, int64_t stack_addr)
{
	return rtos_freertos_esp_xtensa_pick_stacking_info(rtos, thread_id, stack_addr, &rtos_freertos_esp32_s3_stacking, &rtos_freertos_voluntary_esp32_s3_stacking);
}

/*
 Xtensa stacks are saved with the exception bit set, while the call structure is more like a normal
 windowed call. We get rid of the exception bit so gdb can parse it into a working backtrace.
*/
static int rtos_freertos_esp_xtensa_stack_read_involuntary(struct target *target, int64_t stack_ptr, const struct rtos_register_stacking *stacking, uint8_t *stack_data)
{
	int retval;
	retval = target_read_buffer(target, stack_ptr, stacking->stack_registers_size, stack_data);
	if (retval!=ERROR_OK) return retval;

	stack_data[8]&=~0x10; //clear exception bit in PS

	return retval;
}

/*
 Voluntary stacks also have the exception bit set, but they spill the registers on entry and only save the
 PC and PS, as they are inside the vPortYield function.

 Also, because the vPortYield function is called using a window call, the top 2 bits of PC are killed (because
 they indicate the call size). We hard-set those to 0x4 now. ToDo: glance a correct address from a symbol
 somewhere.

 To get to the original function, we have to restore the callers stack frame. On Xtensa, the amount of
 registers to restore are stored in the top of A0 on the callees (which is XT_SOL_PC now). The
 registers A0-A4 are saved on the callees stack, in the 4 bytes above the SP. The

 Because of this madness, we read not only the stack frame, but also the 4 words *above* it (that is, the
 previous stack frame. That's the reason the stack frame definition above looks wonky. It's like this:
 (XT_SOL_xxx entries are part of the FreeRTOS solicited yield stack frame)

0x00 - A0 of orig fn
0x04 - A1 of orig fn
0x08 - A2 of orig fn
0x0c - A3 of orig fn
0x10 - XT_SOL_EXIT - 0 for a voluntary yield <- SP actually is here
0x14 - XT_SOL_PC   - A0 on call to vPortYield
0x18 - XT_SOL_PS   - PS on call to vPortYield
0x1c - XT_SOL_NEXT - Unused
0x20 - XT_SOL_A0   - Unused
0x24 - XT_SOL_A1   - Unused
0x28 - XT_SOL_A2   - Unused
0x2C - XT_SOL_A3   - Unused
0x30 - A4 of orig fn
...
0x4C - A11 of orig fn

 If we're called using CALL8/CALL12, register A4-A7 and in the case of CALL12 A8-A11 are stored in the
 *callers* stack frame. We need some extra logic to dump them in the stack_data.
*/
static int rtos_freertos_esp_xtensa_stack_read_voluntary(struct target *target, int64_t stack_ptr, const struct rtos_register_stacking *stacking, uint8_t *stack_data)
{
	int retval;
	int callno;
	int i;
	uint32_t prevsp;
	uint32_t xt_sol_pc_reg;

	retval = target_read_buffer(target, stack_ptr-0x10, 4*8, stack_data);
	if (retval!=ERROR_OK) return retval;

	stack_data[0x18]&=~0x10; //clear exception bit in PS
	xt_sol_pc_reg = le_to_h_u32(stack_data + 5 * sizeof(uint32_t));
	callno = xt_sol_pc_reg >> 30;
	xt_sol_pc_reg = (xt_sol_pc_reg & 0x3FFFFFFF) | 0x40000000; //Hardcoded for now.
	h_u32_to_le(stack_data + 5 * sizeof(uint32_t), xt_sol_pc_reg);
	prevsp = le_to_h_u32(stack_data + 1 * sizeof(uint32_t));

	//Fill unknown regs with dummy value
	for (i = 12; i < 20; i++) {
		h_u32_to_le(stack_data + i * sizeof(uint32_t), 0xdeadbeef);
	}

	if (callno == 2) { //call8
		retval = target_read_buffer(target, prevsp-32, 4*4, stack_data+0x30);
	} else if (callno == 3) { //call12
		retval = target_read_buffer(target, prevsp-48, 8*4, stack_data+0x30);
	}
	return retval;
}

const struct rtos_register_stacking *rtos_freertos_riscv_pick_stacking_info(struct rtos *rtos, int64_t thread_id, int64_t stack_addr)
{
	return &rtos_freertos_riscv_stacking;
}

// Chip-specific data for calculating Thread Local Storage (tls) address

static const struct freertos_tls_info s_xtensa_tls = {
	XT_REG_IDX_THREADPTR,	/* tls_reg */
	16,			/* tls_align */
};

static const struct freertos_tls_info s_riscv_tls = {
	GDB_REGNO_TP,	/* tls_reg */
	0,		/* tls_align */
};

const struct freertos_tls_info *rtos_freertos_get_tls_info(struct target *target)
{
	if (strncmp(target_get_gdb_arch(target), "riscv", 5) == 0) {
		return &s_riscv_tls;
	} else if (strncmp(target_get_gdb_arch(target), "xtensa", 6) == 0) {
		return &s_xtensa_tls;
	}
	return NULL;
}
