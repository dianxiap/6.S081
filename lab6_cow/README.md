## lab6
## 前置知识
## 遇到的错误
1. qemu启动后所有指令都卡住了，没有结果打印到终端上，排查后发现忘记切换分支了，直接在lab5基础上做的，切换分支后就可以运行了
2. 并行：在修改物理页的引用计数时，没有加锁，比如进程a，进程b映射到同一物理页上时，两个进程同时更改引用计数，会出现引用计数不一致的情况，应该原子化的修改引用计数