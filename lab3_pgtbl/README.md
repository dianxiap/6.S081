## lab3
## 前置知识
1. 分段与分页
2. 多级页表
3. 物理内存与虚拟内存的映射

## 相关宏函数
### riscv.h
1. PGROUNDUP：将 sz（大小）进行页面上取整操作，即向上对齐到下一个页面的边界
2. PGROUNDDOWN：将 a（地址）进行页面下取整操作，即向下对齐到当前页面的边界
3. PA2PTE：将物理地址调整到PTE的正确位置
4. PTE2PA：根据pte虚拟地址提取对应的物理地址
5. PX：根据页表级别 level 和虚拟地址 va, 从虚拟地址中提取出所需的9位页表索引
   
## 相关kernel函数
### main.c
1. kinit()
2. kvminit()
3. procinit()：为每个用户进程分配一个内核栈，该内核栈将被映射到内核虚拟地址空间的高地址部分，位于trampoline下方
4. trapinit()

### kalloc.c
1. extern char end[]：标明了自由物理内存的开始位置,自由物理内存的范围[end,PHYSTOP]
2. keme：结构体全局的管理空闲页面，freelist指向空闲链表的首节点，run中next指向下一个节点，run结构存在于空闲页面本身
3. kinit()： 在内核启动时调用freeerange()初始化内存分配器
4. freerange() ：以页为单位回收[ps_start,ps_end]范围内的内存，将它们挂到free-list上
5. kfree：回收一页内存
6. kalloc：分配一页内存


### vm.c
1. pagetable_t：指向存放RISC-V根页表的一页，它可以是内核的根页表，也可以是用户进程的根页表，存放在stap寄存器中
2. kvminit()：创建内核页表
3. kvminithart()：装在内核页表，将内核根页表的物理地址写入到satp寄存器中
4. walk()：为给定的虚拟地址，找到其相应的最低级PTE，如果PTE不存在则新分配一页使之有效
5. walkaddr()
6. kvmmap()：在即将装载的内核页表上建立一系列的直接映射，是对mappages的封装，只在启动阶段初始化时使用
7. kvmpa()
8. mappages()：为给定输入的映射建立PTE，更新页表
9. uvmunmap()：使用walk找到相应的PTE，并且调用kfree回收相应的物理帧
10. uvmcreate()
11. uvminit()
12. uvmalloc()：向内核申请更多的内存，分配PTE和物理内存来将分配给用户的内存大小从oldsz提升到newsz，并调用mappages来更新页表，并设置PTE的5个标志位都置位
13. uvmdealloc()：调用uvmunmap来回收已分配的物理内存，使得进程的内存大小从oldsz变为newsz
14. freewalk()
15. uvmfree()
16. uvmcopy()：给定一个父进程页表，将其内存拷贝到子进程页表中，同时拷贝页表和对应的物理内存
17. uvmclear()
18. copyout()：从内核空间向用户空间拷贝数据
19. copyin()：从用户空间向内核空间拷贝数据
20. copyinstr()：从用户空间拷贝一个以空字符结尾的字符串到内核空间

### sysproc.c
sys_sbrk()：用户进程调用它以增加或减少自己拥有的物理内存

### proc.c
1. growproc()：根据增加或减少内存的需要，分别调用uvmmalloc和uvmdealloc来满足请求
2. procinit()：在启动时初始化进程表,为每个进程分配一个内核栈
3. allocproc()：在进程表中查找一个未使用的进程，初始化进程状态
4. scheduler()：用于每个CPU的进程调度器，每个CPU在设置完成后调用scheduler()函数。通过上下文切换（context switch）来实现进程之间的切换
5. freeproc()：释放一个进程（proc）结构和与之相关的数据，包括用户页面，内核页表

## Print a page table (easy)
模拟MMU，打印页表
## A kernel page table per process (hard)
为每一个进程维护一个内核页表，然后在该进程进入到内核态时，不使用公用的内核态页表，而是使用进程的内核态页表
## Simplify copyin/copyinstr（hard）
衔接任务二，主要目的是将用户进程页表的所有内容都复制到内核页表中，这样的话，就完成了内核态直接转换虚拟地址的方法
