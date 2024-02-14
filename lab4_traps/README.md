# Lab4: traps

## 一. RISC-V assembly (easy)
## 二. Backtrace
内核为每个进程分配了一段栈帧内存页，用于存放栈。函数调用就是在该位置处进行的。需要记得的是：**fp为当前函数的栈顶指针，sp为栈指针。fp-8 存放返回地址，fp-16 存放原栈帧（调用函数的fp）**。因此，我们可以通过当前函数的栈顶指针 fp 来找到调用该函数的函数栈帧，然后递归地进行下去。直到到达当前页的开始地址。

(1). 首先，在 `kernel/defs.h` 中添加 `backtrace` 的声明：`void backtrace(void)`;

(2). 读取 `fp`，由于我们需要先获取到当前函数的栈帧 fp 的值，该值存放在 s0 寄存器中，因此需要写一个能够读取 s0 寄存器值得函数。按照实验指导书给的方法，在 kernel/riscv.h 添加读取 s0 寄存器的函数：

```c
// read the current frame pointer
static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r"(x));
  return x;
}
```

(3). 在 kernel/printf.c 中实现一个 backtrace() 函数：

```c
void backtrace(void) {
  uint64 fp = r_fp(), top = PGROUNDUP(fp);
  printf("backtrace:\n");
  for(; fp < top; fp = *((uint64*)(fp-16))) {
    printf("%p\n", *((uint64*)(fp-8)));
  }
}
```

迭代方法，不断循环，输出当前函数的返回地址，直到到达该页表起始地址为止。

(4). 添加函数调用
在 kernel/printf.c 文件中的 panic 函数里添加 backtrace 的函数调用；在 sys_sleep 代码中也添加同样的函数调用。

panic
```
printf("\n");
  backtrace();
  panicked = 1; // freeze uart output from other CPUs
```
sys_sleep
```
release(&tickslock);
  backtrace();
  return 0;
```

最后执行测试命令：bttest。（不在 panic 中添加也能通过。）


## 三. Alarm
主要是实现一个定时调用的函数，每过一定数目的 cpu 的切片时间，调用一个用户函数，同时，在调用完成后，需要恢复到之前没调用时的状态。

这里需要注意的是：

- 在当前进程，已经有一个要调用的函数正在运行时，不能再运行第二个；
- 注意寄存器值的保存方式，在返回时需要保存寄存器的值；
- 系统调用的声明和书写方式。

(1). 声明系统调用
- 在 user/user.h 中添加声明：
```c
int sigalarm(int ticks, void (*handler)());
int sigreturn(void);
```
- 在 user/usys.pl 添加 entry，用于生成汇编代码：
```c
entry("sigalarm");
entry("sigreturn");
```

- 在 kernel/syscall.h 中添加函数调用码：
```c
#define SYS_sigalarm  22
#define SYS_sigreturn 23
```

- 在 kernel/syscall.c 添加函数调用代码：
```c
extern uint64 sys_sigalarm(void);
extern uint64 sys_sigreturn(void);
[SYS_sigalarm]  sys_sigalarm,
[SYS_sigreturn] sys_sigreturn,
```

(2). 实现 test0
可以查看 alarmtest.c 的代码，能够发现 test0 只需要进入内核，并执行至少一次即可。不需要正确返回也可以通过测试。

- 首先，写一个 sys_sigreturn 的代码，直接返回 0即可（后面再添加）：

```c
uint64
sys_sigreturn(void)
{
  return 0;
}
```

- 然后，在 kernel/proc.h 中的 proc 结构体添加字段，用于记录时间间隔，经过的时钟数和调用的函数信息：

```c
int interval;
uint64 handler;
int ticks;
```

- 编写 sys_sigalarm() 函数，给 proc 结构体赋值：

```c
uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;
  struct proc * p;
  if(argint(0, &interval) < 0 || argaddr(1, &handler) < 0 || interval < 0) {
    return -1;
  }
  p = myproc();
  p->interval = interval;
  p->handler = handler;
  p->ticks = 0;
  return 0;
}
```

- 在进程初始化时，给初始化这些新添加的字段（kernel/proc.c 的 allocproc 函数）：

```c
p->interval = 0;
  p->handler = 0;
  p->ticks = 0;
```

- 在进程结束后释放内存（freeproc 函数）：

```c
p->interval = 0;
  p->handler = 0;
  p->ticks = 0;
```

- 最后一步，在时钟中断时，添加相应的处理代码：

```c
if(which_dev == 2) {
    if(p->interval) {
      if(p->ticks == p->interval) {
        p->ticks = 0;  // 待会儿需要删掉这一行
        p->trapframe->epc = p->handler;
      }
      p->ticks++;
    }
    yield();
  }
```

到这里，test0 就可以顺利通过了。值得注意的是，现在还不能正确返回到调用前的状态，因此test1 和 test2 还不能正常通过。

这里为啥把要调用的函数直接赋给 epc 呢，原因是函数在返回时，调用 ret 指令，使用 trapframe 内事先保存的寄存器的值进行恢复。这里我们更改 epc 寄存器的值，在返回后，就直接调用的是 handler 处的指令，即执行 handler 函数。

handler 函数是用户态的代码，使用的是用户页表的虚拟地址，因此只是在内核态进行赋值，在返回到用户态后才进行执行，并没有在内核态执行handler代码。

(3). 实现 test1/test2
在这里需要实现正确返回到调用前的状态。由于在 ecall 之后的 trampoline 处已经将所有寄存器保存在 trapframe 中，为此，需要添加一个字段，用于保存 trapframe 调用前的所有寄存器的值。

什么不直接将返回的 epc 进行保存，再赋值呢？或者说为什么需要保存 trapframe 的值呢？为什么需要两个系统调用函数才能实现一个功能呢？之前的系统调用函数都是一个就完成了。

这是一些值得思考的一些问题。首先需要了解系统调用的过程：

- 在调用 sys_sigalarm 前，已经把调用前所有的寄存器信息保存在了 trapframe 中。然后进入内核中执行 sys_sigalarm 函数。执行的过程中，只需要做一件事：为 ticks 等字段进行赋值。赋值完成后，该系统调用函数就完成了，trapframe 中的寄存器的值恢复，返回到了用户态。此时的 trapframe 没有保存的必要。
- 在用户态中，当执行了一定数量的 cpu 时间中断后，我们将返回地址更改为 handler 函数，这样，在 ret 之后便开始执行 handler 函数。在 cpu 中断时，也是进入的 trap.c 调用了相应的中断处理函数。
- 在执行好 handler 后，我们希望的是回到用户调用 handler 前的状态。但那时的状态已经被用来调用 handler 函数了，现在的 trapframe 中存放的是执行 sys_sigreturn 前的 trapframe，如果直接返回到用户态，则找不到之前的状态，无法实现我们的预期。
- 在 alarmtest 代码中可以看到，每个 handler 函数最后都会调用 sigreturn 函数，用于恢复之前的状态。由于每次使用 ecall 进入中断处理前，都会使用 trapframe 存储当时的寄存器信息，包括时钟中断。因此 trapframe 在每次中断前后都会产生变换，如果要恢复状态，需要额外存储 handler 执行前的 trapframe（即更改返回值为 handler 前的 trapframe），这样，无论中间发生多少次时钟中断或是其他中断，保存的值都不会变。
- 因此，在 sigreturn 只需要使用存储的状态覆盖调用 sigreturn 时的 trapframe，就可以在 sigreturn 系统调用后恢复到调用 handler 之前的状态。再使用 ret 返回时，就可以返回到执行 handler 之前的用户代码部分。

所以，其实只需要增加一个字段，用于保存调用 handler 之前的 trapframe 即可：

- 在 kernel/proc.h 中添加一个指向 trapframe 结构体的指针：

```c
struct trapframe *pretrapframe;
```

- 在进程初始化时，为该指针进行赋值：

```c
p->interval = 0;
  p->handler = 0;
  p->ticks = 0;
  if((p->pretrapframe = (struct trapframe *)kalloc()) == 0){
    release(&p->lock);
    return 0;
  }
```

- 进程结束后，释放该指针：

```c
p->interval = 0;
  p->handler = 0;
  p->ticks = 0;
  if(p->pretrapframe)
    kfree((void*)p->pretrapframe);
```

- 在每次时钟中断处理时，判断是否调用 handler，如果调用了，就存储当前的 trapframe，用于调用之后的恢复：

```c
if(which_dev == 2) {
    if(p->interval) {
      if(p->ticks == p->interval) {
        //p->ticks = 0;
        //memmove(p->pretrapframe, p->trapframe, sizeof(struct trapframe));
        *p->pretrapframe = *p->trapframe;
        p->trapframe->epc = p->handler;
      }// else {
        p->ticks++;
      //}
    }
    yield();
  }
```

- 最后，实现 sys_sigreturn 恢复执行 handler 之前的状态：

```c
uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  *p->trapframe = *p->pretrapframe;
  //memmove(p->trapframe, p->pretrapframe, sizeof(struct trapframe));
  p->ticks = 0;
  return 0;
}
```

到这里就都完成了。需要注意的是，如果有一个handler函数正在执行，就不能让第二个handler函数继续执行。为此，可以再次添加一个字段，用于标记是否有 handler 在执行。我第一次通过的时候就增加了一个字段，但想来想去，感觉有点多余。

其实可以直接在 sigreturn 中设置 ticks 为 0，而取消 trap.c 中的 ticks 置 0 操作。这样，即使第一个 handler 还没执行完，由于 ticks 一直是递增的，第二个 handler 始终无法执行。只有当 sigreturn 执行完成后，ticks 才置为 0，这样就可以等待下一个 handler 执行了。

---

## 总结


移步至飞书：https://d19ce7nuq0g.feishu.cn/docx/V7oWdjLCdojoQFx8d4YcTGejn8X?from=from_copylink

