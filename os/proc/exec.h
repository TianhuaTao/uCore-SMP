#if !defined(EXEC_H)
#define EXEC_H

#define MAX_EXEC_ARG_COUNT 8
#define MAX_EXEC_ARG_LENGTH 32

#define MAX_EXEC_ENVP_COUNT 8
#define MAX_EXEC_ENVP_LENGTH 32


int exec(char *name, int argc, const char **argv,int envc,const char ** envp);

#endif // EXEC_H
