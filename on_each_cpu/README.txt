An example of calling a function on each CPU.

The most interesting part is a call stack when CPU received an inter process interrupt to execute the function.

Thread 3 received signal SIGTRAP, Trace/breakpoint trap.
[Switching to Thread 4294967292]
0xfcb64022 in ?? ()
(gdb) bt
#0  0xfcb64022 in ?? ()
#1  0xc10f260a in flush_smp_call_function_queue (warn_cpu_offline=<optimised out>) at /build/linux-xK5wks/linux-4.4.0/kernel/smp.c:246
#2  0xc10f2e82 in generic_smp_call_function_single_interrupt () at /build/linux-xK5wks/linux-4.4.0/kernel/smp.c:195
#3  0xc104b9e5 in __smp_call_function_single_interrupt () at /build/linux-xK5wks/linux-4.4.0/arch/x86/kernel/smp.c:311
#4  smp_call_function_single_interrupt (regs=<optimised out>) at /build/linux-xK5wks/linux-4.4.0/arch/x86/kernel/smp.c:318
#5  0xc104ba0d in smp_call_function_interrupt () at /build/linux-xK5wks/linux-4.4.0/arch/x86/kernel/smp.c:320
#6  0xc17b2824 in call_function_interrupt () at /build/linux-xK5wks/linux-4.4.0/arch/x86/include/asm/entry_arch.h:14
#7  0x00000000 in ?? ()
(gdb) c
Continuing.

The second stack is for a CPU on which on_each_cpu() was called.

Thread 473 received signal SIGTRAP, Trace/breakpoint trap.
[Switching to Thread 8549]
0xfcb64022 in ?? ()
(gdb) bt
#0  0xfcb64022 in ?? ()
#1  0xc10f2d1c in on_each_cpu (func=0xfcb64000, info=0x0, wait=<optimised out>) at /build/linux-xK5wks/linux-4.4.0/kernel/smp.c:599
#2  0xf85d501e in ?? ()
#3  0xc100211a in do_one_initcall (fn=0xf85d5000) at /build/linux-xK5wks/linux-4.4.0/init/main.c:794
#4  0xc116fec9 in do_init_module (mod=0xfcb66000) at /build/linux-xK5wks/linux-4.4.0/kernel/module.c:3243
#5  0xc10f87f3 in load_module (info=0xe85c3f20, uargs=<optimised out>, flags=<optimised out>) at /build/linux-xK5wks/linux-4.4.0/kernel/module.c:3538
#6  0xc10f8f35 in SYSC_finit_module (flags=<optimised out>, uargs=<optimised out>, fd=<optimised out>) at /build/linux-xK5wks/linux-4.4.0/kernel/module.c:3627
#7  SyS_finit_module (fd=3, uargs=-2147162856, flags=0) at /build/linux-xK5wks/linux-4.4.0/kernel/module.c:3608
#8  0xc100393d in do_syscall_32_irqs_on (regs=<optimised out>) at /build/linux-xK5wks/linux-4.4.0/arch/x86/entry/common.c:391
#9  do_fast_syscall_32 (regs=0xe85c3fac) at /build/linux-xK5wks/linux-4.4.0/arch/x86/entry/common.c:458
#10 0xc17b1fdc in entry_SYSENTER_32 () at /build/linux-xK5wks/linux-4.4.0/arch/x86/entry/entry_32.S:310
#11 0x00000003 in ?? ()
#12 0x00000000 in ?? ()
