# 0.xv6系统调用


## 任务一（trace）
首先，需要知道的是，用户态的代码都在 `user` 文件夹内，内核态的代码都在 `kernel` 文件夹内。在上一个实验中，主要是在用户态内实现进程之间的创建和通信等功能，因此只需要在 `user` 文件夹内写代码。在这个任务中，`user/trace.c` 文件已经给出，重点是内核态部分。
## 第一步，Makefile
根据实验指导书，首先需要将 `$U/_trace` 添加到 `Makefile` 的 `UPROGS` 字段中。
## 第二步，添加声明
然后需要添加一些声明才能进行编译，进而启动`qemu。需要以下几步： 
1. 可以看到，在 `user/trace.c` 文件中，使用了 `trace` 函数，因此需要在 `user/user.h` 文件中加入函数声明：`int trace(int)`;
2. 同时，为了生成进入中断的汇编文件，需要在 `user/usys.pl` 添加进入内核态的入口函数的声明：`entry("trace")`，以便使用 `ecall` 中断指令进入内核态；  
3. 同时在 `kernel/syscall.h` 中添加系统调用的指令码，这样就可以编译成功了。 

说明：
   在生成的 `user/usys.S` 文件中可以看到，汇编语言 `li a7, SYS_trace` 将指令码放到了 `a7` 寄存器中。在内核态 `kernel/syscall.c` 的 `syscall` 函数中，使用 `p->trapframe->a7` 取出寄存器中的指令码，然后调用对应的函数。
## 第三步，实现 sys_trace() 函数
最主要的部分是实现 `sys_trace()` 函数，在 `kernel/sysproc.c` 文件中。目的是实现内核态的 `trace()` 函数。我们的目的是跟踪程序调用了哪些系统调用函数，因此需要在每个 `trace` 进程中，添加一个 `mask` 字段，用来识别是否执行了 `mask` 标记的系统调用。在执行 `trace` 进程时，如果进程调用了 `mask `所包括的系统调用，就打印到标准输出中。

首先在 `kernel/proc.h` 文件中，为 `proc` 结构体添加 `mask` 字段：`int mask;`。

然后在 `sys_trace()` 函数中，为该字段进行赋值，赋值的 `mask` 为系统调用传过来的参数，放在了 `a0 `寄存器中。使用 `argint()` 函数可以从对应的寄存器中取出参数并转成 `int` 类型。

`argint()` 的函数声明在 `kernel/defs.h` 中，具体实现在 `kernel/syscall.c` 中。除了取出 `int` 型的数据函数外，还有取出地址参数的 `argaddr` 函数，就是将参数以地址的形式取出来，第二个任务需要使用。

`sys_trace()` 函数的实现：
```
uint64 sys_trace(void)
{
  int mask;
  if(argint(0, &mask) < 0) 
    return -1;
  myproc()->mask = mask;
  return 0;
}
```
## 第四步，跟踪子进程
需要跟踪所有 `trace` 进程下的子进程，在`kernel/proc.c` 的 `fork()` 代码中，添加子进程的 `mask`：
```
np->mask = p->mask;
```
## 第五步，打印信息
所有的系统调用都需要通过 `kernel/syscall.c` 中的 `syscall()` 函数来执行，因此在这里添加判断：
```
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();
    if ((1 << num) & p->mask) {
      printf("%d: syscall %s -> %d\n", p->pid, syscalls_name[num], p->trapframe->a0);
    }
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
```

由于系统调用的函数名称没办法直接获取，因此创建了一个数组，用于存储系统调用的名称，`syscalls_name` 代码如下：

```
char* syscalls_name[] = {
[SYS_fork]    "fork",
[SYS_exit]    "exit",
[SYS_wait]    "wait",
[SYS_pipe]    "pipe",
[SYS_read]    "read",
[SYS_kill]    "kill",
[SYS_exec]    "exec",
[SYS_fstat]   "fstat",
[SYS_chdir]   "chdir",
[SYS_dup]     "dup",
[SYS_getpid]  "getpid",
[SYS_sbrk]    "sbrk",
[SYS_sleep]   "sleep",
[SYS_uptime]  "uptime",
[SYS_open]    "open",
[SYS_write]   "write",
[SYS_mknod]   "mknod",
[SYS_unlink]  "unlink",
[SYS_link]    "link",
[SYS_mkdir]   "mkdir",
[SYS_close]   "close",
[SYS_trace]   "trace",
};
```
此外，还需添加系统调用入口 `extern uint64 sys_trace(void)`; 和 `[SYS_trace]   sys_trace`,到 `kernel/syscall.c` 中。其中的 `p->trapframe->a0` 存储的是函数调用的返回值。

## 任务二（sysinfo）
与任务一相似，也是实现一个系统调用。
## 第一步，添加 Makefile
根据实验指导书，首先需要将 `$U/_sysinfotest` 添加到 `Makefile` 的 `UPROGS` 字段中。
## 第二步，添加声明
同样需要添加一些声明才能进行编译，启动 `qemu`。需要以下几步： 
1. 在 `user/user.h `文件中加入函数声明：`int sysinfo(struct sysinfo*)`;，同时添加结构体声明 `struct sysinfo;`；
2. 在 `user/usys.pl` 添加进入内核态的入口函数的声明：`entry("sysinfo");`；
3. 同时在` kernel/syscall.h` 中添加系统调用的指令码。

## 第三步，获取内存信息
可以在 `kernel/sysinfo.h` 中查看结构体 `struct sysinfo`， 其中只有两个字段，一个是保存空闲内存信息，一个是保存正在运行的进程数目。

两个字段的信息都需要自己写函数调用来获取，先来获取内存信息。内存信息的处理都写在 `kernel/kalloc.c` 文件中了，内存信息以链表的形式存储，每个节点存储一个物理内存页。

从 `kfree` 函数中可以发现，每次创建一个页时，将其内容初始化为`1`，然后将它的下一个节点指向当前节点的 `freelist`，更新 `freelist` 为这个新创建的页。也就是说，`freelist` 指向最后一个可以使用的内存页，它的 `next` 指向上一个可用的内存页。

因此，我们可以通过遍历所有的 `freelist` 来获取可用内存页数，然后乘上页大小即可。添加获取内存中剩余空闲内存的函数：
```
uint64 free_mem(void) 
{
  struct run *r = kmem.freelist;
  uint64 n = 0;
  while (r) {
    n++;
    r = r->next;
  }
  return n * PGSIZE;
}
```
## 第四步，获取进程数目
所有的进程有关的操作都保存在 `/kernel/proc.c`文件中，其中的 `proc` 数组保存了所有进程。进程有五种状态，我们只需要遍历 `proc` 数组，计算不为 `UNUSED` 状态的进程数目即可，函数为：
```
int n_proc(void)
{
  struct proc *p;
  int n = 0;
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state != UNUSED)
      n++;
  }
  return n;
}
```
## 第五步，声明和调用
在 `kernel/defs.h` 中添加上面这两个函数的声明：
```
uint64             free_mem(void);
int             n_proc(void);
```
然后在 `kernel/sysproc.c` 中的 `sys_sysinfo` 函数进行调用：
```
uint64 sys_sysinfo(void)
{
  struct sysinfo info;
  uint64 addr;
  struct proc* p = myproc();
  if(argaddr(0, &addr) < 0) {
    return -1;
  }
  info.freemem = free_mem();
  info.nproc = n_proc();
  if (copyout(p->pagetable, addr, (char*)&info, sizeof(info)) < 0) {
    return -1;
  }
  return 0;
}
```
需要注意的是，这里使用 `copyout` 方法将内核空间中，相应的地址内容复制到用户空间中。这里就是将 `info` 的内容复制到进程的虚拟地址内，具体是哪个虚拟地址，由函数传入的参数决定（`addr` 读取第一个参数并转成地址的形式）。

## 总结
1. 做完实验，对xv6系统的启动和系统调用的理解更深刻了。
2. 本次作业阅读源码的时间较长，要理清楚每个模块负责干什么。
