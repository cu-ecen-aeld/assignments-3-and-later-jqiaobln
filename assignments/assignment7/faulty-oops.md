# The Dump

[  114.243495] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
[  114.243979] Mem abort info:
[  114.244180]   ESR = 0x0000000096000045
[  114.244321]   EC = 0x25: DABT (current EL), IL = 32 bits
[  114.244628]   SET = 0, FnV = 0
[  114.244952]   EA = 0, S1PTW = 0
[  114.245158]   FSC = 0x05: level 1 translation fault
[  114.245455] Data abort info:
[  114.245663]   ISV = 0, ISS = 0x00000045
[  114.246245]   CM = 0, WnR = 1
[  114.251139] user pgtable: 4k pages, 39-bit VAs, pgdp=0000000043dc8000
[  114.251335] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
[  114.251893] Internal error: Oops: 96000045 [#1] PREEMPT SMP
[  114.252192] Modules linked in: faulty(O) hello(O) scull(O)
[  114.252588] CPU: 2 PID: 440 Comm: sh Tainted: G           O      5.15.124-yocto-standard #1
[  114.252851] Hardware name: linux,dummy-virt (DT)
[  114.253116] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[  114.253336] pc : faulty_write+0x18/0x20 [faulty]
[  114.253772] lr : vfs_write+0xf8/0x29c
[  114.253902] sp : ffffffc009733d80
[  114.254008] x29: ffffffc009733d80 x28: ffffff8003688000 x27: 0000000000000000
[  114.254586] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
[  114.254868] x23: 0000000000000000 x22: ffffffc009733dc0 x21: 00000055656f7590
[  114.255149] x20: ffffff80037bd400 x19: 000000000000000c x18: 0000000000000000
[  114.255366] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
[  114.255598] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
[  114.255806] x11: 0000000000000000 x10: 0000000000000000 x9 : ffffffc008268e3c
[  114.256049] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
[  114.256256] x5 : 0000000000000001 x4 : ffffffc000b7c000 x3 : ffffffc009733dc0
[  114.256517] x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
[  114.256825] Call trace:
[  114.256929]  faulty_write+0x18/0x20 [faulty]
[  114.257094]  ksys_write+0x74/0x10c
[  114.257201]  __arm64_sys_write+0x24/0x30
[  114.257318]  invoke_syscall+0x5c/0x130
[  114.257457]  el0_svc_common.constprop.0+0x4c/0x100
[  114.257598]  do_el0_svc+0x4c/0xb4
[  114.257699]  el0_svc+0x28/0x80
[  114.257797]  el0t_64_sync_handler+0xa4/0x130
[  114.257923]  el0t_64_sync+0x1a0/0x1a4
[  114.258115] Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
[  114.258467] ---[ end trace 49563cc657b50bcf ]---
/bin/start_getty: line 20:   440 Segmentation fault      ${setsid:-} ${getty} -L $1 $2 $3

# The Analysis
1. faulty_write is the function in the faulty module where the error occurred. The +0x18/0x20 part indicates that the error occurred 0x18 bytes into a function that is 0x20 bytes long.
2. ksys_write+0x74/0x10c, __arm64_sys_write+0x24/0x30, invoke_syscall+0x5c/0x130, etc.: These are the functions that were called leading up to the error.
3. Code: d2800001 d2800000 d503233f d50323bf (b900003f): This is the machine code of the instruction that caused the error.

