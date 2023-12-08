## 0.xv6系统待用
添加系统调用主要有以下几步：

1. 在`user/user.h`中添加系统调用函数的定义。
2. 在`user/usys.pl`中添加入口，这个文件将会在`make`后生成`user/usys.S`文件，在该汇编文件中，每个函数就只有三行，将系统调用号通过`li(load imm)`存入`a7`寄存器，(通过设置寄存器`a7`的值，表明调用哪个`system call`)之后使用`ecall`进入内核态，最后返回。

```
fork:
    li a7, SYS_fork
    ecall   //ecall表示一种特殊的trap，转到kernel/syscall.c:syscall执行
    ret
```
3. 在`kernel/syscall.h`中定义系统调用号。
4. 在`kernel/syscall.c`的`syscalls`函数指针数组中添加对应的函数。在`syscall`函数中，先读取`trapframe->a7`获取系统调用号，之后根据该系统调用号查找`syscalls`数组中的对应的处理函数并调用。



## 1.trace
```
int trace(int mask);
```
trace系统调用控制跟踪
- 参数：mask的比特位指定要跟踪的系统调用
- 调用时输出：进程id、系统调用的名称和返回值

添加系统调用流程：

0. 在Makefile的UPROGS中添加$U/_trace
```
    $U/_pingpong\
    $U/_primes\
    $U/_find\
    $U/_xargs\
    $U/_trace\
```
1. 将系统调用的原型添加到`user/user.h`，存根添加到`user/usys.pl`，以及将系统调用编号添加到`kernel/syscall.h`
   ```
    //user/user.h
    ...
    int sleep(int);
    int uptime(void);
    int trace(int);
   ```
   ```
    //user/usys.pl
    ...
    entry("sleep");
    entry("uptime");
    entry("trace");
   ```
   ```
    //kernel/syscall.h
    ...
    #define SYS_link   19
    #define SYS_mkdir  20
    #define SYS_close  21
    #define SYS_trace  22
   ```
2. 先在`proc`结构体中添加一个`tracemask`字段，之后在`fork`函数中复制该字段到新进程。
```
    //proc.h
    ...
    struct inode *cwd;           // Current directory
    char name[16];               // Process name (debugging)
    int tracemask;               // Trace maek
```
```
    //proc.c
    ...
    //将父进程的跟踪掩码拷贝到子进程
    np->tracemask=p->tracemask;
```
3. 在系统调用`sys_trace`中就只要通过`argint`函数读取参数，然后设置给`tracemask`字段就行了。
```
    //syscall.c
    ...
    void trace(int mask)
    {
    //获取进程当前状态
    struct proc*p=myproc();
    p->tracemask=mask;

    }
    ...
    //sys_trace函数
    uint64
    sys_trace(void)
    {
    uint64 mask;
    if(argaddr(0,&mask)<0)
        return -1;
    trace(mask);
    return 0;
    }
```
4. 最后修改`syscall`，当系统调用号和`tracemask`匹配时就打印相关信息。
```
    if (num > 0 && num < NELEM(syscalls) && syscalls[num])
    {
        ///系统调用的返回值保存到a0寄存器
        p->trapframe->a0 = syscalls[num]();
        for(int i=1,t=p->tracemask;i<=t;i++)
        {
        //遍历位为 1 的位置，表示要追踪的系统调用
        if(t&(1<<i))
        {
            //1.遍历的是追踪掩码位为1的位置
            //2.该位位置代表的系统调用的编号等于当前系统调用编号 num，则输出系统调用的相关信息
            if(i==num)
            printf("%d: syscall %s -> %d\n",p->pid,syscall_name[num],p->trapframe->a0);
            else if(i>num)break;
        }
    }
```