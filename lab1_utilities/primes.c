#include "kernel/types.h"
#include "user.h"

#define RD 0
#define WR 1

const uint LEN = sizeof(int);

/*
    功能：获取左管道的第一个数字
    返回值：成功返回0，失败返回-1
*/
int GetLfdFirst(int lfd[2], int *first)
{
    if (read(lfd[RD], first, LEN) == LEN)
    {
        printf("prime %d\n", *first);
        return 0;
    }
    return -1;
}

/*
    功能：判断当前是否被第一个数整除，不能就写入右管道
*/
void check(int lfd[2], int rfd[2], int first)
{
    int data;
    while (read(lfd[RD], &data, LEN) == LEN)
    {
        if (data % first)
            write(rfd[WR], &data, LEN);
    }
    close(lfd[RD]);
    close(rfd[WR]);
}

/*
    功能：判断素数的主函数
*/
void prime(int lfd[2])
{
    close(lfd[WR]);
    int first = 0;

    // 按照给的算法提示，获取第一个数后，开始判断剩余的数字，
    // 如果当前数字不能被第一个数整除，写入右管道
    if (GetLfdFirst(lfd, &first) == 0)
    {
        int rfd[2];
        pipe(rfd);
        // 判断剩余数字
        check(lfd, rfd, first);

        int pid = fork();
        if (pid == 0)
        {
            prime(rfd); // 递归调用，下一次右管道即为左管道
        }
        else
        {
            close(rfd[RD]);
            wait(0);
        }
    }
    exit(0);
}
int main()
{
    int fd[2];
    pipe(fd);

    for (int i = 2; i <= 35; ++i)
        write(fd[WR], &i, LEN);
    int pid = fork();
    if (pid == 0)
    {
        prime(fd);
    }
    else
    {
        // 父进程等所有儿孙进程退出后再退出
        close(fd[RD]);
        close(fd[WR]);
        wait(0);
    }
    exit(0);
}