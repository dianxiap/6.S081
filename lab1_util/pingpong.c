#include "kernel/types.h"
#include "user.h"

#define RD 0
#define WR 1

int main()
{
    int ptoc[2] = {0};
    int ctop[2] = {0};
    char buf = 'P';

    pipe(ptoc);
    pipe(ctop);

    int pid = fork();

    int exit_status = 0;
    if (pid < 0)
    {
        fprintf(2, "fork error\n");
        close(ptoc[RD]);
        close(ptoc[WR]);
        close(ctop[RD]);
        close(ctop[WR]);
        exit(1);
    }
    else if (pid == 0)
    {
        // 用不着的先关闭，防止阻塞read
        close(ptoc[WR]);
        close(ctop[RD]);

        if (read(ptoc[RD], &buf, sizeof(char)) != sizeof(char))
        {
            fprintf(2, "child read error\n");
            exit_status = 1;
        }
        else
        {
            printf("%d: received ping\n", getpid());
        }

        write(ctop[WR], &buf, sizeof(char));

        close(ptoc[RD]);
        close(ctop[WR]);

        exit(exit_status);
    }
    else
    {
        close(ctop[WR]);
        close(ptoc[RD]);

        // 注意：必须有一方先写入，另一方等待读取，本题要求父进程新写入
        write(ptoc[WR], &buf, sizeof(char));

        if (read(ctop[RD], &buf, sizeof(char)) != sizeof(char))
        {
            fprintf(2, "parent read error\n");
            exit_status = 1;
        }
        else
        {
            printf("%d: received pong\n", getpid());
        }

        close(ctop[RD]);
        close(ptoc[WR]);

        exit(exit_status);
    }
    exit(0);
}