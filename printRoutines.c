#include <stdio.h>
#include <unistd.h>
#include "printRoutines.h"
#include <string.h>
#include <stdlib.h>

/*
  You probably want to create a number of printing routines in this
  file.  Put the prototypes in printRoutines.h.
*/

int printPosition(FILE *out, unsigned long location) {

  return fprintf(out, ".pos 0x%lx\n", location);
}

/* This is a function to demonstrate how to do print formatting. It
 * takes the file descriptor the I/O is to be done to. You are not
 * required to use the same type of printf formatting, but you must
 * produce the same resulting format. You are also not required to
 * keep this function in your final result, and may replace it with
 * other equivalent instructions.
 *
 * The arguments for the format string in the example printing are
 * strictly for illustrating the spacing. You are free to construct
 * the output however you want.
 */
int printInstruction(FILE *out) {

  int res = 0;
  
  char * r1 = "%rax";
  char * r2 = "%rdx";
  char * inst1 = "rrmovq";
  char * inst2 = "jne";
  char * inst3 = "irmovq";
  char * inst4 = "mrmovq";
  unsigned long destAddr = 8193;
  
  res += fprintf(out, "    %-8s%s, %s          # %-22s\n", 
		 inst1, r1, r2, "2002");

  res += fprintf(out, "    %-8s0x%lx              # %-22s\n", 
		 inst2, destAddr, "740120000000000000");

  res += fprintf(out, "    %-8s$0x%lx, %s         # %-22s\n", 
		 inst3, 16L, r2, "30F21000000000000000");

  res += fprintf(out, "    %-8s0x%lx(%s), %s # %-22s\n", 
		 inst4, 65536L, r2, r1, "50020000010000000000"); 
  
  res += fprintf(out, "    %-8s%s, %s          # %-22s\n", 
		 inst1, r2, r1, "2020");
  
  res += fprintf(out, "    %-8s0x%lx  # %-22s\n", 
		 ".quad", 0xFFFFFFFFFFFFFFFFL, "FFFFFFFFFFFFFFFF");

  return res;
}  
  
// halt nop ret only need outputFile as param
// register array
const char * registers[0xf] = {"%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi", "%r8 ", "%r9 ", "%r10", "%r11", "%r12", "%r13", "%r14"};

// OP array
const char * op[0x7] = {"addq", "subq", "andq", "xorq", "mulq", "divq", "modq"};

// jXX/cmovXX array
const char * cond[0x7] = {"", "le", "l ", "e ", "ne", "ge", "g "};

void printHalt(FILE *out) {
    fprintf(out, "    %-8s                                        # %s\n", "halt", "00");
}
void printNop(FILE *out) {
    fprintf(out, "    %-8s                                      # %s\n", "nop", "10");
}

// Cmovq OPq need ifn byte and register byte 
void printCmovq(FILE *out, unsigned char ifn, unsigned char reg) {
    unsigned char rA = (reg & 0xf0)>>4;
    unsigned char rB = (reg & 0x0f);
    char instr[7];
    if (ifn == 0x0) {
        strcpy(instr, "rrmovq");
    } else {
        strcpy(instr, "cmov");
        strcat(instr, cond[ifn]);
    }
    fprintf(out, "    %-8s%s, %s                              # %s%X%X%X\n",
            instr, registers[rA], registers[rB], "2",ifn,rA,rB);
}

// Irmovq Rmmovq Mrmovq need  register byte and quad
void printIrmovq(FILE *out, unsigned char reg, unsigned long quad) {
    unsigned char rB = (reg & 0x0f);
    fprintf(out, "    %-8s$0x%lx, %s                         # %s%X%lX\n",
            "irmovq", quad, registers[rB], "30F",rB,((quad&0xff00000000000000)>>56)+((quad&0x00ff000000000000)>>40)+((quad&0x0000ff0000000000)>>24)+((quad&0x000000ff00000000)>>8)+((quad&0x00000000ff000000)<<8)+((quad&0x0000000000ff0000)<<24)+((quad&0x000000000000ff00)<<40)+((quad&0x00000000000000ff)<<56));
}

void printRmmovq(FILE *out, unsigned char reg, unsigned long quad) {
    unsigned char rA = (reg & 0xf0)>>4;
    unsigned char rB = (reg & 0x0f);
    fprintf(out, "    %-8s%s, 0x%lx(%s)                 # %s%X%X%lX\n",
            "rmmovq", registers[rA], quad, registers[rB], "40F",rA,rB,((quad&0xff00000000000000)>>56)+((quad&0x00ff000000000000)>>40)+((quad&0x0000ff0000000000)>>24)+((quad&0x000000ff00000000)>>8)+((quad&0x00000000ff000000)<<8)+((quad&0x0000000000ff0000)<<24)+((quad&0x000000000000ff00)<<40)+((quad&0x00000000000000ff)<<56));
}

void printMrmovq(FILE *out, unsigned char reg, unsigned long quad) {
    unsigned char rA = (reg & 0xf0)>>4;
    unsigned char rB = (reg & 0x0f);
    fprintf(out, "    %-8s0x%lx(%s), %s                  # %s%X%X%lX\n",
            "mrmovq", quad, registers[rB], registers[rA], "50F",rA,rB,((quad&0xff00000000000000)>>56)+((quad&0x00ff000000000000)>>40)+((quad&0x0000ff0000000000)>>24)+((quad&0x000000ff00000000)>>8)+((quad&0x00000000ff000000)<<8)+((quad&0x0000000000ff0000)<<24)+((quad&0x000000000000ff00)<<40)+((quad&0x00000000000000ff)<<56)); 
}

// OPq
void printOPq(FILE *out, unsigned char ifn, unsigned char reg) {
    unsigned char rA = (reg & 0xf0)>>4;
    unsigned char rB = (reg & 0x0f);
    fprintf(out, "    %-8s%s, %s                           # %s%X%X%X\n",
            op[ifn], registers[rA], registers[rB], "6",ifn,rA,rB);
}

// jXX needs ifn byte and quad
void printJ(FILE *out, unsigned char ifn, unsigned long quad) {
  char instr[4];
  strcpy(instr, "j");
  if (ifn== 0x0) {
      strcat(instr, "mp");
  } else{
      strcat(instr, cond[ifn]);
   } 
  fprintf(out, "    %-8s0x%lx                                   # %s%X%lX\n",
          instr, quad, "7",ifn,((quad&0xff00000000000000)>>56)+((quad&0x00ff000000000000)>>40)+((quad&0x0000ff0000000000)>>24)+((quad&0x000000ff00000000)>>8)+((quad&0x00000000ff000000)<<8)+((quad&0x0000000000ff0000)<<24)+((quad&0x000000000000ff00)<<40)+((quad&0x00000000000000ff)<<56));
}

// call needs quad
void printCall(FILE *out, unsigned long quad) {
    fprintf(out, "    %-8s%lx                                   # %s%lX\n",
            "call", quad, "80",((quad&0xff00000000000000)>>56)+((quad&0x00ff000000000000)>>40)+((quad&0x0000ff0000000000)>>24)+((quad&0x000000ff00000000)>>8)+((quad&0x00000000ff000000)<<8)+((quad&0x0000000000ff0000)<<24)+((quad&0x000000000000ff00)<<40)+((quad&0x00000000000000ff)<<56));
}

void printRet(FILE *out) {
    fprintf(out, "    %-8s                                          # %s\n", "ret", "90");
}

// push and pop just need rA
void printPushq(FILE *out, unsigned char reg) {
    unsigned char rA = (reg & 0xf0)>>4;
    fprintf(out, "    %-8s%s                                    # %s%X%s\n",
            "pushq", registers[rA], "A0",rA,"F");
}

void printPopq(FILE *out, unsigned char reg) {
    unsigned char rA = (reg & 0xf0)>>4;
    fprintf(out, "    %-8s%s                                    # %s%X%s\n",
            "popq", registers[rA], "B0",rA,"F");
}

void printByte(FILE *out, unsigned char byte){
    fprintf(out, "    %-8s%x                                      # %X\n",
             ".byte", byte, byte);
}

// printf("%02x%02x ", a & 255, (a / 256) & 255);
void printQuad(FILE *out, unsigned long quad){
  fprintf(out, "    %-8s0x%lX                               # %lX\n", 
            ".quad", quad, ((quad&0xff00000000000000)>>56)+((quad&0x00ff000000000000)>>40)+((quad&0x0000ff0000000000)>>24)+((quad&0x000000ff00000000)>>8)+((quad&0x00000000ff000000)<<8)+((quad&0x0000000000ff0000)<<24)+((quad&0x000000000000ff00)<<40)+((quad&0x00000000000000ff)<<56));
}


