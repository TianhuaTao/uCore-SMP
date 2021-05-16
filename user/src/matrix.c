#include <stdio.h>
#include <ucore.h>

int main() {
    const int MATSIZE = 10;
    int mata[MATSIZE][MATSIZE];
    int matb[MATSIZE][MATSIZE];
    int matc[MATSIZE][MATSIZE];
    int i, j, k, size = MATSIZE;
    for (i = 0; i < size; i ++) {
        for (j = 0; j < size; j ++) {
            mata[i][j] = matb[i][j] = 1;
        }
    }

    int times = 200;

    while(times -- > 0) {
        for (i = 0; i < size; i++) {
            for (j = 0; j < size; j++) {
                matc[i][j] = 0;
                for (k = 0; k < size; k++) {
                    matc[i][j] += mata[i][k] * matb[k][j];
                }
            }
        }
        for (i = 0; i < size; i++) {
            for (j = 0; j < size; j++) {
                mata[i][j] = matb[i][j] = matc[i][j];
            }
        }
    }

    return 0;
}
