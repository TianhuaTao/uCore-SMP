#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <fcntl.h>
#include <string.h>


int smash(int x){
    x++;
    smash(x*x);
    return x*2;
}


int main(int argc, char* argv[]){

    smash(0);
    // should be killed
    return 0;
}