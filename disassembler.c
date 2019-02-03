#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "printRoutines.h"
#include <stdbool.h>

#define ERROR_RETURN -1
#define SUCCESS 0

// check registers rA and rB 0 to e
bool validRegisters(unsigned char registers){
    unsigned char rA = (registers & 0xf0)>>4;
    unsigned char rB = (registers & 0x0f);
    if ((rA <= 0xe) && (rB <= 0xe)) { 
        return true;
    }else return false;
}

// check registers rA 0xf and rB 0 to e
bool validRegistersAf(unsigned char registers){
    unsigned char rA = (registers & 0xf0)>>4;
    unsigned char rB = (registers & 0x0f);
    if ((rA == 0xf) && (rB <= 0xe)) { 
        return true;
    }else return false;
}

// check registers rA 0 to e and rB 0xf
bool validRegistersBf(unsigned char registers){
    unsigned char rA = (registers & 0xf0)>>4;
    unsigned char rB = (registers & 0x0f);
    if ((rA <= 0xe) && (rB == 0xf)) { 
        return true;
    }else return false;
}

bool hasNBytes(FILE *nextCode,int n){
    for(int i=0; i<n; i++){
        if(feof(nextCode)) return false;
        fgetc(nextCode);
    }
    return true;
}

int main(int argc, char **argv) {
  
  FILE *machineCode, *outputFile;
  long currAddr = 0; 

  // Verify that the command line has an appropriate number
  // of arguments

  if (argc < 2 || argc > 4) {
    fprintf(stderr, "Usage: %s InputFilename [OutputFilename] [startingOffset]\n", argv[0]);
    return ERROR_RETURN;
  }

  // First argument is the file to read, attempt to open it 
  // for reading and verify that the open did occur.
  machineCode = fopen(argv[1], "rb");

  if (machineCode == NULL) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
    return ERROR_RETURN;
  }

  // Second argument is the file to write, attempt to open it for
  // writing and verify that the open did occur. Use standard output
  // if not provided.
  outputFile = argc <= 2 ? stdout : fopen(argv[2], "w");
  
  if (outputFile == NULL) {
    fprintf(stderr, "Failed to open %s: %s\n", argv[2], strerror(errno));
    fclose(machineCode);
    return ERROR_RETURN;
  }

  // If there is a 3rd argument present it is an offset so convert it
  // to a numeric value.
  if (4 == argc) {
    errno = 0;
    currAddr = strtol(argv[3], NULL, 0);
    if (errno != 0) {
      perror("Invalid offset on command line");
      fclose(machineCode);
      fclose(outputFile);
      return ERROR_RETURN;
    }
  }

  fprintf(stderr, "Opened %s, starting offset 0x%lX\n", argv[1], currAddr);
  fprintf(stderr, "Saving output to %s\n", argc <= 2 ? "standard output" : argv[2]);

  // ------------------- READ BEGINNING OF FILE ----------------------
  // Either go to offset provided, or read file until first non-zero byte
  //
  unsigned char byte;
  
  // read currAddr num bytes to advance to offset
  fseek(machineCode, currAddr, SEEK_SET);
  // get byte at currAddr 
  byte = fgetc(machineCode);
  // machineCode now points to currAddr + 1 and currAddr is still provided offset or 0

  // check if instruction is non-zero
  while(byte == 0) {
      byte = fgetc(machineCode);
      currAddr += 1; 
  }

  // if we advanced currAddr (first bytes were zeros), then print .pos currAddr
  // else, we should not print .pos because we did not have 3rd argument, and first byte is non-zero
  if (currAddr) printPosition(outputFile, currAddr);
  currAddr -= 1; // this will be incremented in do-while after PROPERLY reading first byte VS checking non-zero
  fseek(machineCode, -1, SEEK_CUR); // need to do this so that the do-while loop is not messed up
  
  
  // NOW WE SHOULD HAVE machineCode POINTING TO FIRST NON-ZERO BYTE
  // ------------------- READ BEGINNING OF FILE ----------------------
  
  
  // TODO: REMEMBER little-endian! reading vals will be low order->high order
  // TODO: REMEMBER byte = 8 bits = 4bits,4bits = 1 hex, 1 hex
  // TODO: REMEMBER in C non-zero int is true, 0 is false
  // TODO: REMEMBER machineCode FILE pointer points to the address of NEXT byte  
  // ------------------- READ AND DISASSEMBLE INSTRUCTIONS ---------------------
  // 
  // VARS USED IN WHILE LOOP
  unsigned char icode;  // high order half-byte
  unsigned char ifn;    // low order half-byte
  unsigned char tempByte;    // can use this for intermediary reading
  unsigned long quad;    

  // set this to true in case 0x0
  // 
  bool inLongHalt = false;
  bool prevHalt = false;
  bool valid = false;
  bool iCodeOk = false;
  bool iFnOk = false;

  //! ----------------- START OF WHILE LOOP -----------------
  // assume byte is first byte of instruction, machineCode points to NEXT byte
  //
  //   1. Try parsing the instruction, assume valid = false
  //      1. If you get full instruction:
  //              * print it
  //              * set valid = true
  //              * advance machineCode to next instruction byte and increment currAddr
  //      2. If at any point we see that instruction is invalid, break WITHOUT changing machineCode OR byte OR currAddr
  //   2. If !valid:
  //      1. if currAddr multiple of 8, and we can read next 8 bytes, then output .quad directive
  //      2. else output .byte directive

  while (!feof(machineCode)) {
    // assume machineCode points to the FIRST INSTRUCTION BYTE
    byte = fgetc(machineCode);

    // read high order half-byte (instruction code) and low order half-byte (fn)
    icode = (byte & 0xF0)>>4;
    ifn = byte & 0x0F;

    
    valid = false;
    
    // invalid if code is not one of 0 1 2 3 4 5 6 7 8 9 A B
    // invalid if ifn not 0 or if code is 2 or 6 or 7 and ifn > 0x06
    //
    iCodeOk = false;
    if (icode < 0xc) iCodeOk = true;
    iFnOk = false;
    if ((icode == 0x2 || icode == 0x6 || icode == 0x7) && ifn <= 0x6) iFnOk = true;
    if ((icode != 0x2 && icode != 0x6 && icode != 0x7) && ifn == 0x0) iFnOk = true;
        
    
    // this stuff is to print .pos once we find first non-zero byte during long halt
    // needs to be outside if statement because we don't care if the byte is proper, just non-zero
    if (prevHalt && icode != 0x0) prevHalt = false;
    if (inLongHalt && icode != 0x0) {
        fprintf(outputFile, "\n");  // print blank line!
        printPosition(outputFile, currAddr+1);  // we are off by one
        inLongHalt = false;
        prevHalt = false;
    }

    if (iCodeOk && iFnOk) {
        // full OP code is correct, try to parse rest of instruction
        //
        // IMPORTANT: advance machineCode, byte, and currAddr ONLY after valid = true
        switch (icode) {
            case 0x0:                                                                                                   //00 halt
                if (!prevHalt) {
                    printHalt(outputFile);
                    prevHalt = true;
                    inLongHalt = false;
                } else {
                    inLongHalt = true;
                }
                valid = true;                                                                                                                                 
                currAddr++;
                break;
            case 0x1:                                                                                                    //10 nop
                printNop(outputFile);                                                                                                                         

                valid = true;                                                                                                                                 
                currAddr++;
                break;
            case 0x2:                                                                                                    //2fn rrmovq rA, rB or cmovXX rA, rB
                // get the next Byte, we know !feof(machinecode) is true                                                 
                tempByte = fgetc(machineCode);
                if (!validRegisters(tempByte)) {
                    fseek(machineCode, -1, SEEK_CUR);   // move FILE handle back one byte
                    break;
                }

                printCmovq(outputFile, ifn, tempByte);  
                
                valid = true;
                currAddr += 2; // read 2 bytes
                break;
            case 0x3:                                                                                                    //30 irmovq V, rB
                // get the next Byte, we know !feof(machinecode) is true
                // check registers
                tempByte = fgetc(machineCode);
                if (!validRegistersAf(tempByte)) {
                    fseek(machineCode, -1, SEEK_CUR);   // move FILE handle back one byte
                    break;
                }
                
                // the register byte is valid. check for 8 byte value, machineCode should point to beginning of quad
                if (fread(&quad, 8, 1, machineCode) != 1) {
                    fseek(machineCode, -8, SEEK_CUR);   // move FILE handle back eight bytes
                    break;
                }
                
                // instruction valid
                printIrmovq(outputFile, tempByte, quad); // printf("%08lX\n",*quad);
                
                valid = true;
                currAddr += 10;
                break;
            case 0x4:                                                                                                   //40 rmmovq rA, D(rB)
                // get the next Byte, we know !feof(machinecode) is true                                                 
                tempByte = fgetc(machineCode);
                if (!validRegisters(tempByte)) {
                    fseek(machineCode, -1, SEEK_CUR);   // move FILE handle back one byte
                    break;
                }

                // the register byte is valid. check for 8 byte value, machineCode should point to beginning of quad
                if (fread(&quad, 8, 1, machineCode) != 1) {
                    fseek(machineCode, -8, SEEK_CUR);   // move FILE handle back eight bytes
                    break;
                }
                
                // instruction valid
                printRmmovq(outputFile, tempByte, quad); // printf("%08lX\n",*quad);
                
                valid = true;
                currAddr += 10;
                break;
            case 0x5:                                                                                                   //50 mrmovq D(rB), rA
                // get the next Byte, we know !feof(machinecode) is true                                                 
                tempByte = fgetc(machineCode);
                if (!validRegisters(tempByte)) {
                    fseek(machineCode, -1, SEEK_CUR);   // move FILE handle back one byte
                    break;
                }

                // the register byte is valid. check for 8 byte value, machineCode should point to beginning of quad
                if (fread(&quad, 8, 1, machineCode) != 1) {
                    fseek(machineCode, -8, SEEK_CUR);   // move FILE handle back eight bytes
                    break;
                }
                
                // instruction valid
                printMrmovq(outputFile, tempByte, quad); // printf("%08lX\n",*quad);
                
                valid = true;
                currAddr += 10;
                break;
            case 0x6:                                                                                                   //6fn OPq rA, rB
                // get the next Byte, we know !feof(machinecode) is true                                                 
                tempByte = fgetc(machineCode);
                if (!validRegisters(tempByte)) {
                    fseek(machineCode, -1, SEEK_CUR);   // move FILE handle back one byte
                    break;
                }

                // instruction valid
                printOPq(outputFile, ifn, tempByte);
                
                valid = true;
                currAddr += 2; // read 2 bytes
                break;
            case 0x7:                                                                                                   //7fn jXX Dest
                // fprintf(stdout, "JUMPING\n");
                if (fread(&quad, 8, 1, machineCode) != 1) {
                    // TODO
                    fprintf(stdout, "could not read\n");
                    fseek(machineCode, -8, SEEK_CUR);   // move FILE handle back eight bytes
                    break;
                }
                
                // instruction valid
                printJ(outputFile, ifn, quad); // printf("%08lX\n",*quad);
                
                valid = true;
                currAddr += 9;
                break;
            case 0x8:                                                                                                   //80 call Dest
                if (fread(&quad, 8, 1, machineCode) != 1) {
                    fseek(machineCode, -8, SEEK_CUR);   // move FILE handle back eight bytes
                    break;
                }
                
                // instruction valid
                printCall(outputFile, quad); // printf("%08lX\n",*quad);
                
                valid = true;
                currAddr += 9;
                break;
            case 0x9:                                                                                                   //90 ret
                printRet(outputFile);
                if (ifn != 0x0) fprintf(stdout, "WTF ifn is not 0!\n");
                
                valid = true;                                                                                                                                 
                currAddr++;
                break;
            case 0xA:                                                                                                   //A0 pushq rA
                // get the next Byte, we know !feof(machinecode) is true
                // check registers
                tempByte = fgetc(machineCode);
                if (!validRegistersBf(tempByte)) {
                    fseek(machineCode, -1, SEEK_CUR);   // move FILE handle back one byte
                    break;
                }

                // instruction valid
                printPushq(outputFile, tempByte);
                
                valid = true;
                currAddr += 2; // read 2 bytes
                break;
            case 0xB:                                                                                                   //B0 popq rA
                // get the next Byte, we know !feof(machinecode) is true
                // check registers
                tempByte = fgetc(machineCode);
                if (!validRegistersBf(tempByte)) {
                    fseek(machineCode, -1, SEEK_CUR);   // move FILE handle back one byte
                    break;
                }

                // instruction valid
                printPopq(outputFile, tempByte);
                
                valid = true;
                currAddr += 2; // read 2 bytes
                break;
            default:        // TODO: should we do anything here? Probably not, but think about it
                break;
		}
	}
    // ----------------- CHECK IF INVALID -----------------
    // machine code, are address of instruction to be executed NEXT
    if (!valid) {
        if (feof(machineCode)) break;
        // handle invalid
        // try to read quad
        fseek(machineCode, -1, SEEK_CUR);   // do this so that quad var below gets full quad on success read
        int cur = ftell(machineCode);
        int readResult = fread(&quad, 8, 1, machineCode);
        if ((currAddr % 8 == 0) && (readResult == 1)) { // if 8 aligned and quad read was successful
            printQuad(outputFile,quad);
            currAddr += 8; //total read bytes of machineCode since start of while loop

        } else {
            // output .byte
            fseek(machineCode, cur + 1 , SEEK_SET);
            // byte is still first byte from beginning of while loop
            printByte(outputFile, byte);
            currAddr += 1;
        }
    }
  } 
  //! ----------------- END OF WHILE LOOP -----------------
  //special case of ending on 0's bytes, we should print .pos AND .byte 0x0
  if (inLongHalt) {
      fprintf(outputFile, "\n");  // print blank line!
      printPosition(outputFile, currAddr+1);  // we are off by one
      printByte(outputFile, 0x0);
  } 
  fclose(machineCode);
  fclose(outputFile);
  return SUCCESS;
}
