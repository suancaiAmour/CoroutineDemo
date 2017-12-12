//
//  Coroutine.s
//  协程 Demo
//
//  Created by 张鸿 on 2017/11/27.
//  Copyright © 2017年 酸菜Amour. All rights reserved.
//

.text
.align 4
.globl _pushCoroutineEnv
.globl _popCoroutineEnv
.globl _getSP
.globl _getFP

_pushCoroutineEnv:
    stp    x21, x30, [x0]
    mov    x21, x0
    bl     openSVC
    mov    x0, x21
    ldp    x21, x30, [x0]
    mov    x1, sp
    stp    x19, x20, [x0]
    stp    x21, x22, [x0, #0x10]
    stp    x23, x24, [x0, #0x20]
    stp    x25, x26, [x0, #0x30]
    stp    x27, x28, [x0, #0x40]
    stp    x29, x30, [x0, #0x50]
    stp    x29, x1, [x0, #0x60]
    stp    d8, d9, [x0, #0x70]
    stp    d12, d13, [x0, #0x90]
    stp    d14, d15, [x0, #0xa0]
    mov    x0, #0x0
    ret

_popCoroutineEnv:
    sub    sp, sp, #0x10
    mov    x21, x0
    ldr    x0, [x21, #0xb0]
    str    x0, [sp, #0x8]
    add    x1, sp, #0x8
    orr    w0, wzr, #0x3
    mov    x2, #0x0
    bl     openSVC
    mov    x0, x21
    add    sp, sp, #0x10
    ldp    x19, x20, [x0]
    ldp    x21, x22, [x0, #0x10]
    ldp    x23, x24, [x0, #0x20]
    ldp    x25, x26, [x0, #0x30]
    ldp    x27, x28, [x0, #0x40]
    ldp    x29, x30, [x0, #0x50]
    ldp    x29, x2, [x0, #0x60]
    ldp    d8, d9, [x0, #0x70]
    ldp    d10, d11, [x0, #0x80]
    ldp    d12, d13, [x0, #0x90]
    ldp    d14, d15, [x0, #0xa0]
    mov    sp, x2
    ret

_getSP:
    mov   x0, sp
    ret

_getFP:
    mov   x0, x29
    ret

openSVC:
    mov    x16, #0x30
    svc    #0x80
    stp    x29, x30, [sp, #-0x10]!
    mov    x29, sp
    mov    sp, x29
    ldp    x29, x30, [sp], #0x10
    ret
