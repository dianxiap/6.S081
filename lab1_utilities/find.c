#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de; // 这个指的是目录项这一结构体（在kernel/fs.h中定义），其实目录也是一种文件，里面就是存放了一系列的目录项
    struct stat st;   // 这个指的是文件的统计信息（在kernel/stat.h中定义），包含文件类型（目录或文件）/inode/文件引用nlink/文件大小/存放fs的disk dev

    // 打开文件，第二个参数指示的是打开方式，0代表的是O_RDONLY只读的形式。返回值是file descriptor >=0，<0说明open失败
    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // fstat用来返回“相关文件状态信息”，传入的fd是需要用open打开的
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 参数错误，find的第一个参数必须是目录
    if (st.type != T_DIR)
    {
        fprintf(2, "Usage: find <directory> <filename>\n");
        return;
    }

    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) // 检查缓存有没有溢出
    {
        fprintf(2, "find: path too long\n");
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/'; // p指向了buf的下一个位置

    // 访问目录内容。每次read只是read一个de的大小（也就是一个目录项），只有read到最后一个目录项的下一次read才会返回0，也就不满足while循环条件退出循环，
    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0) // 此文件夹无文件，continue操作后进行下一次read
            continue;

        // memmove, 把de.name信息复制p,其中de.name是char name[255],代表文件名
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        // 通过文件名buf获取文件信息，并保存在st所指的结构体stat中,一般是文件没有打开的时候这样操作。
        if (stat(buf, &st) < 0)
        {
            fprintf(2, "find: cannot stat %s\n", p);
            continue;
        }

        // 在进行下一步操作前，需要先更新st保存的文件信息

        // 递归查找，此时buf已经拼接上了当前目录路径, 不递归 . 和 ..
        if (st.type == T_DIR && strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0)
            find(buf, filename);
        else if (strcmp(p, filename) == 0)
            printf("%s\n", buf);
    }

    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(2, "Usage: find <directory> <filename>\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
