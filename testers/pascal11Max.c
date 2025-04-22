#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

/*****************************************************************************
 *  Choose Function Tester
 *
 *  11th level of Pascal's Triangle:
 *  1 10 45 120 210 *252* 210 120 45 10 1
 *  Calculate the largest number at the 11th level of the Pascal's Triangle,
 *  which is the middle number of the level, calculated using 10 choose 5
 * 
 *  Written by Khoa Ho & Hieu Tran
 */

/* iterative choose function */
int choose(int n, int k) {
    int res = 1;
    int i;
    for (i = 1; i <= k; i++) {
        res = res * (n - k + i) / i;
    }
    return res;
}

void main() {
    int val = choose(10, 5);
    print(WRITETERMINAL, "Pascal's Triangle Test starts\n");
    if (val == 252) {
        print(WRITETERMINAL, "\nPASS: C(10,5) == 252\n\n");
    } else {
        print(WRITETERMINAL, "\nFAIL: C(10,5) != 252\n\n");
    }
    SYSCALL(TERMINATE, 0, 0, 0);
}
