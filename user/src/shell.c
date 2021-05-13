#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
const unsigned char LF = 0x0a;
const unsigned char CR = 0x0d;
const unsigned char DL = 0x7f;
const unsigned char BS = 0x08;

char line[128] = {};

int top = 0;

void push(char c) {
    line[top++] = c;
}

void pop() {
    --top;
}

int is_empty() {
    return top == 0;
}

void clear() {
    top = 0;
}

void print_help(){
    printf(
        "C Shell Help\n"
        "exit: Exit shell and shutdown.\n"
        "help: Print this help message.\n"
    );
}

char args_buffer[1024];
char *argv[10]; 
char *args_buffer_cur;
void parse_line(){
    args_buffer_cur = args_buffer;
    int argc = 0;
    char *start;
    char *end;

    start = line;
    end = line;

    while (*end)
    {
        while (*start == ' ') {
            start++;
        }

        end = start;
        while (*end != ' ' && *end != '\0') {
            end++;
        }

        int arg_len = end - start;
        if(arg_len==0)
            break;
        // printf("arg_len %d\n", arg_len);
        argv[argc] = args_buffer_cur;
        stpncpy(args_buffer_cur, start, arg_len);
        args_buffer_cur += arg_len;
        *args_buffer_cur = 0;
        args_buffer_cur++;
        argc++;
        start = end;
    }
    argv[argc] = 0; // NULL terminate
    // printf("argc %d\n", argc);
    // for (int i = 0; i < argc; i++)
    // {
    //     printf("argv[%d]=\"%s\"\n", i, argv[i]);
    // }
    
    if (argc == 0){
        return;
    }

    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(argv[0], "help") == 0) {
        print_help();
    } else {
        int pid = fork();
        if (pid == 0) {
            // child process
            if (execv(argv[0], argv) < 0) {
                printf("Shell: %s: No such file\n", argv[0]);
                exit(-9);
            }
            panic("unreachable!");
        } else {
            int xstate = 0;
            int exit_pid = 0;
            exit_pid = wait(pid, &xstate);
            assert(pid == exit_pid, -1);
            printf("Shell: Process %d exited with code %d\n", pid, xstate);
        }
    }
}


int main() {

    if (open("console", O_RDWR, 0) < 0) {
        mknod("console", 1, 0);
        open("console", O_RDWR, 0);
    }
    dup(0); // stdout
    dup(0); // stderr

    printf("C user shell\n");
    printf(">> ");
    while (1) {
        char c = getchar();
        switch (c) {
            case LF:
            case CR:
                printf("\n");
                if (!is_empty()) {
                    push('\0');
                    parse_line();

                    clear();
                }
                printf(">> ");
                break;
            case BS:
            case DL:
                if (!is_empty()) {
                    putchar(BS);
                    printf(" ");
                    putchar(BS);
                    pop();
                }
                break;
            default:
                putchar(c);
                push(c);
                break;
        }
    }
    return 0;
}
