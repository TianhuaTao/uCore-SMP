#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ucore.h>
#include <ucore.h>
char *
fmtname(char *path)
{
  static char buf[DIRSIZ + 1];
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if (strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
  return buf;
}

void ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0)
  {
    printf("ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0)
  {
    printf("ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type)
  {
  case T_FILE:
    printf("%s FIL %d %p\n", fmtname(path), st.ino, st.size);
    break;
  case T_DEVICE:
    printf("%s DEV %d %p\n", fmtname(path), st.ino, st.size);
    break;
  case T_DIR:
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
    {
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
      if (de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if (stat(buf, &st) < 0)
      {
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s ", fmtname(buf));
      if (st.type == T_DIR)
      {
        printf("DIR ");
      }
      else if (st.type == T_FILE)
      {
        printf("FIL ");
      }
      else if (st.type == T_DEVICE)
      {
        printf("DEV ");
      }
      else
      {
        printf("UNK ");
      }
      printf("%d %p\n", st.ino, st.size);
    }
    break;
  default:
    printf("unknown type\n");

  }
  close(fd);
}

int main(int argc, char *argv[])
{
  int i;

  if (argc < 2)
  {
    ls(".");
    exit(0);
  }
  for (i = 1; i < argc; i++)
    ls(argv[i]);
  exit(0);
}
