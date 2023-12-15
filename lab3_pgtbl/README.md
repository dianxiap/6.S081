## lab3
## 前置知识
1. 多级页表
2. 物理内存与虚拟内存的映射

## 相关kernel函数

### main.c
1. 调用函数kinit()在内核启动时对物理内存分配器进行初始化
2. 调用函数kvminit()构建内核页表

### kalloc.c
1. extern char end[]:标明了自由物理内存的开始位置,自由物理内存的范围[end,PHYSTOP]
2. keme结构体全局的管理空闲页面，freelist指向空闲链表的首节点，run中next指向下一个节点，run结构存在于空闲页面本身
3. kinit() 在内核启动时调用freeerange()初始化内存分配器
4. freerange() 以页为单位回收[ps_start,ps_end]范围内的内存
5. kfree  回收一页内存
6. kalloc 分配一页内存


### vm.c
1. kernel_pagetable：指向内核页表的根目录页
2. kvminit()
3. kvminithart()
4. walk()
5. walkaddr()
6. kvmmap()
7. kvmpa()
8. mappages()
9. uvmunmap()
10. uvmcreate()
11. uvminit()
12. uvmalloc()
13. uvmdealloc()
14. freewalk()
15. uvmfree()
16. uvmcopy()
17. uvmclear()
18. copyout()
19. copyin()
20. copyinstr()


