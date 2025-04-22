#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"
#define MAXLEN 128

/*****************************************************************************
 * Sting Reversal Tester
 * 
 * This program reads a string from the terminal, 
 * reverses it, and prints it out.
 * 
 * Written by Khoa Ho & Hieu Tran
 */

void main(void) {
    char buf[MAXLEN];
    char tmp[2];
    int len = 0;
    int i;

    print(WRITETERMINAL, "reverseString Test starts\n\n");
    print(WRITETERMINAL, "Enter word with 128 characters or less: ");
    SYSCALL(READTERMINAL, (int)buf, MAXLEN - 1, 0);

    /* calculate length */
    while (buf[len] != EOS && buf[len] != '\n') len++;
    buf[len] = EOS;

    print(WRITETERMINAL, "\nReversed: ");
    for (i = len - 1; i >= 0; i--) {
        tmp[0] = buf[i];
        tmp[1] = EOS;
        print(WRITETERMINAL, tmp);
    }

    print(WRITETERMINAL, "\n\nreverseString concluded\n");

    SYSCALL(TERMINATE, 0, 0, 0);
}
