#include <ucore.h>
#include <stdio.h>
int main(int argc, char *argv[])
{
  int i;

  if (argc < 2)
  {
    printf("Usage: makedir files...\n");
    exit(1);
  }

  for (i = 1; i < argc; i++)
  {
    if (mkdir(argv[i]) < 0)
    {
      printf("makedir: %s failed to create\n", argv[i]);
      break;
    }
  }

  return 0;
}
