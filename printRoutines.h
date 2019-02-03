/* This file contains the prototypes and constants needed to use the
   routines defined in printRoutines.c
*/

#ifndef _PRINTROUTINES_H_
#define _PRINTROUTINES_H_

#include <stdio.h>

int printPosition(FILE *, unsigned long);
int printInstruction(FILE *);

// halt nop ret only need outputFile as param
void printHalt(FILE *);
void printNop(FILE *);
void printRet(FILE *);

// Cmovq OPq need ifn byte and register byte 
void printCmovq(FILE *, unsigned char, unsigned char);   // also printRrmovq
void printOPq(FILE *, unsigned char, unsigned char);

// Irmovq Rmmovq Mrmovq need  register byte and quad
void printIrmovq(FILE *, unsigned char, unsigned long);
void printRmmovq(FILE *, unsigned char, unsigned long);
void printMrmovq(FILE *, unsigned char, unsigned long);

// jXX needs ifn byte and quad
void printJ(FILE *, unsigned char, unsigned long);

// call needs quad
void printCall(FILE *, unsigned long);

// push and pop just need rA
void printPushq(FILE *, unsigned char);
void printPopq(FILE *, unsigned char);

void printByte(FILE *, unsigned char);

void printQuad(FILE *, unsigned long);

#endif /* PRINTROUTINES */
