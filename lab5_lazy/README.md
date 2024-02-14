# Lab5: xv6 lazy page allocation

## 一. Eliminate allocation from sbrk() (easy)

任务一的目的就是更改 kernel/sysproc.c 中的 sys_sbrk() 函数，把原来只要申请就分配的逻辑改成申请时仅进行标注，即更改进程的 sz 字段。

```c
uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  struct proc *p = myproc();
  addr = p->sz;
  //if (addr + n < 0) return -1;
  if (addr + n >= MAXVA || addr + n <= 0)
    return addr;
  p->sz = addr + n;
  //if(growproc(n) < 0)
  //  return -1;
  if(n < 0)
    uvmdealloc(p->pagetable, addr, p->sz);
  return addr;
}
```

这里有三点需要注意的：

- 将 p->sz 增加 n，这也是任务一的要求。
- 如果 n 是负数，则对其对应的内存进行释放（仿照 proc.c 中的 growproc 函数写就行）。
- 判断堆的空间大小。不能超过 MAXVA，也不能释放小于 0 的地址空间。

如果直接运行 echo hi 命令会报错，因为我们还没写分配内存的逻辑，这是下面任务的内容

## 二. Lazy allocation (moderate)

(1). 处理中断，分配内存

仿照上一个实验中的中断处理操作，在 kernel/trap.c 中，找到中断处理的逻辑进行更改。

系统调用的中断码是 8，**page fault 的中断码是 13 和 15**。因此，这里我们对 r_scause() 中断原因进行判断，如果是 13 或是 15，则说明没有找到地址。**错误的虚拟地址被保存在了 STVAL 寄存器中**，我们取出该地址进行分配。如果**申请物理地址没成功或者虚拟地址超出范围了，那么杀掉进程**，代码如下：

```c
else if((which_dev = devintr()) != 0){
    // ok
  } else if(r_scause() == 13 || r_scause() == 15) {
    uint64 va = r_stval();
    uint64 pa = (uint64)kalloc();
    if (pa == 0) {
      p->killed = 1;
    } else if (va >= p->sz || va <= PGROUNDDOWN(p->trapframe->sp)) {
      kfree((void*)pa);
      p->killed = 1;
    } else {
      va = PGROUNDDOWN(va);
      memset((void*)pa, 0, PGSIZE);
      if (mappages(p->pagetable, va, PGSIZE, pa, PTE_W | PTE_U | PTE_R) != 0) {
        kfree((void*)pa);
        p->killed = 1;
      }
    }
    // lazyalloc(va);
  } else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }
```

需要注意的几点：

1. 如果申请内存成功了，如果虚拟地址不合法，需要再释放掉这块内存；
2. va >= p->sz 是指虚拟地址不能超过堆实际分配的大小；
3. p->trapframe->sp 是指栈指针的位置，所以 PGROUNDDOWN(p->trapframe->sp) 是指栈顶最大值，是 guard 页的最大地址，用于防止栈溢出；
4. 中断判断时，如果出错（虚拟地址不合法或者没有成功映射到物理地址），就杀死进程。

由于在接下来的 write 等函数出现找不到地址时，也需要分配内存。本来打算写一个函数进行代码复用，但实际上不太行，因为只有在这里才需要杀死进程。

(2). 处理 uvmunmap 的报错

uvmunmap 是在释放内存时调用的，由于释放内存时，页表内有些地址并没有实际分配内存，因此没有进行映射。如果在 uvmunmap 中发现了没有映射的地址，直接跳过就行，不需要 panic：

```c
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      //panic("uvmunmap: walk");
      continue;
    if((*pte & PTE_V) == 0)
      continue;
      //panic("uvmunmap: not mapped");
    if(PTE_FLAGS(*pte) == PTE_V)
      panic("uvmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
  }
}
```

这样，任务二就算完成了，注意添加 spinlock.h 和 proc.h 的头部引用 #include。到目前为止，已经可以成功运行 echo hi 命令了。已经算是写完主要的逻辑了。

## 三. Lazytests and Usertests (moderate)

任务三是需要通过所有的测试用例，这个任务的代码很少，前面很多已经都写好了。主要是两个函数的异常处理，一个是 uvmcopy，另一个是 walkaddr。

(1). uvmcopy

fork 函数在创建进程时会调用 uvmcopy 函数。由于没有实际分配内存，因此，在这里，忽略 pte 无效，继续执行代码。

kernel/vm.c
```c
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      continue;
      //panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      continue;
      //panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
      kfree(mem);
      goto err;
    }
  }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}
```

(2). walkaddr

这里，由于 read/write 等系统调用时，由于进程利用系统调用已经到了内核中，页表已经切换为内核页表，无法直接访问虚拟地址。因此，需要通过 walkaddr 将虚拟地址翻译为物理地址。这里如果没找到对应的物理地址，就分配一个：

kernel/vm.c
```c
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return 0;

  pte = walk(pagetable, va, 0);
  //if(pte == 0)
  //  return 0;
  //if((*pte & PTE_V) == 0)
  //  return 0;
  if (pte == 0 || (*pte & PTE_V) == 0) {
    //pa = lazyalloc(va);
    struct proc *p = myproc();
    if(va >= p->sz || va < PGROUNDUP(p->trapframe->sp)) return 0;
    pa = (uint64)kalloc();
    if (pa == 0) return 0;
    if (mappages(p->pagetable, va, PGSIZE, pa, PTE_W|PTE_R|PTE_U|PTE_X) != 0) {
      kfree((void*)pa);
      return 0;
    }
    return pa;
  }
  if((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  return pa;
}
```

需要注意的是，这里的分配代码并不杀死进程。

## 总结
