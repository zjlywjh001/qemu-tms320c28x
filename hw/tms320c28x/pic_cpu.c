/*
 * TMS320C28X Programmable Interrupt Controller support.
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
#include "hw/irq.h"
#include "cpu.h"

/* pic handler */
static void tms320c28x_pic_cpu_handler(void *opaque, int irq, int level)
{
    Tms320c28xCPU *cpu = (Tms320c28xCPU *)opaque;
    CPUState *cs = CPU(cpu);
    // uint32_t irq_bit;

    //todo irq priority, mask, ... 

    if (irq < 15 && irq >=0) {
        cs->interrupt_request = CPU_INTERRUPT_INT; //trigger interrupt
        cs->exception_index = irq + 100;
    }
}

void cpu_tms320c28x_pic_init(Tms320c28xCPU *cpu)
{
    int i;
    qemu_irq *qi;
    qi = qemu_allocate_irqs(tms320c28x_pic_cpu_handler, cpu, NR_IRQS);
    for (i = 0; i < NR_IRQS; i++) {
        cpu->env.irq[i] = qi[i];
    }
}
