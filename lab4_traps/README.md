## lab4
## 前置知识
1. trap处理链路
2. 一些寄存器的作用
3. 函数栈帧
4. alarm

## 陷阱trap
1. 用户级别（系统调用）：uservec->usertrap->usertrapret->userret
2. kernel级别
3. 异常
4. 设备中断
5. 计时器中断

## 相关寄存器
1. stvec：核将trap handler（在用户空间下的trap，是trampoline的uservec；在内核空间下的trap，是kernel/kernelvec.S中的kernelvec）写到stvec中。当trap发生时，RISC-V就会跳转到stvec中的地址，准备处理trap
2. sepc：当trap发生时，RISC-V就将当前pc的值保存在sepc中（例如，指令ecall就会做这个工作），因为稍后RISC-V将使用stvec中的值来覆盖pc，从而开始执行trap handler。稍后在userret中，sret指令将sepc的值复制到pc中，内核可以设置sepc来控制sret返回到哪里
3. scause：RISC-V在这里存放一个数字，代表引发trap的原因
4. sscratch：一个特别的寄存器，通常在用户空间发生trap，并进入到uservec之后，sscratch就装载着指向进程trapframe的指针（该进程的trapframe，在进程被创建，并从userret返回的时候，就已经被内核设置好并且放置到sscratch中）。RISC-V还提供了一条交换指令（csrrw），可以将任意寄存器与sscratch进行值的交换。sscratch的这些特性，便于在uservec中进行一些寄存器的保存、恢复工作
5. sstatus：位于该寄存器中的SIE位，控制设备中断是否开启，如果SIE被清0，RISC-V会推迟期间的设备中断，直到SIE被再次置位；SPP位指示一个trap是来自用户模式下还是监管者模式下的，因此也决定了sret要返回到哪个模式下。

**当trap确实发生了，`RISC-V CPU硬件`按以下步骤处理所有类型的trap（除了计时器中断）
**
1. 如果造成trap的是设备中断，将sstatus中的SIE位清0，然后跳过以下步骤。
2. （如果trap的原因不是设备中断）将sstatus中的SIE位清0，关闭设备中断。
3. 将pc的值复制到sepc中。
4. 将发生trap的当前模式（用户模式或监管者模式）写入sstatus中的SPP位。
5. 设置scause的内容，反映trap的起因。
6. 设置模式为监管者模式。
7. 将stvec的值复制到pc中。
8. 从新的pc值开始执行。

## 相关kernel函数


## RISC-V assembly (easy)
## Backtrace
## Alarm
### 1. test0: invoke handler
### 2. test1/test2(): resume interrupted code
正确返回到调用handler前的状态
注意：有一个handler函数正在执行，就不能让第二个handler函数继续执行
