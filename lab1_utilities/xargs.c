#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

// xargs.c程序的命令行参数从xargs开始，比如：
//  echo hello | xargs echo nihao
//  argv是 xargs echo nihao (echo hello) ,其中(echo hello)经由管道“|”由标准输入变成命令行参数拼接到argv后面

// 1.循环读取标准输入
// 2.拼接到argv后面
// 3.fork并在子进程中exec

char *getline(int fd)
{
    // 静态字符数组
    // buf 被声明为静态变量，它的存储位置在数据段，而不是在栈上，所以函数返回后，指针p仍然可以指向这块内存空间，以读取下一行的字符
    static char buf[MAXARG];
    // 字符指针p指向该数组的起始位置，每次调用函数时，buf会被下一行数据覆盖，保证只读取一行的数据
    char *p = buf;
    while (1)
    {
        int len = read(fd, p, sizeof(char));
        // 读到0字节表示文件描述符已没有数据
        if (len <= 0)
            return 0;
        // 读到'\n'表示读完了一行
        if (*p == '\n')
            break;
        p++;
    }
    *p = 0;
    // 返回当前读到的数据
    return buf;
}
int main(int argc, char *argv[])
{
    if (argc < 1)
    {
        fprintf(2, "Usage: xargs <>\n");
        exit(1);
    }
    while (1)
    {
        // 按行读取
        char *p = getline(0);
        if (p == 0)
            break;

        if (fork() == 0)
        {
            // 向前挪移一个，过滤掉xargs参数
            char *_argv[MAXARG];
            for (int i = 0; i + 1 < argc; i++)
                _argv[i] = argv[i + 1];

            // 将读取的数据拼接到命令行参数后面argv
            _argv[argc - 1] = p;
            _argv[argc] = 0;
            // for(int i=0;i<argc+1;i++)
            //     printf("%s ",_argv[i]);
            exec(_argv[0], _argv);
            fprintf(2, "exec %s error\n", argv[1]);
            exit(2);
        }
        else
            wait(0);
    }
    exit(0);
}