#ifndef BINARY_READER_H
#define BINARY_READER_H

#include "elf_args.h"

uint32_t read32(unsigned char* buff, int ind);

uint16_t read16(unsigned char* buff, int ind);

Instruction parseInstr(uint32_t instr);

const char* getBind(Elf32_Sym curr);

const char* getType(Elf32_Sym curr);

const char* getVis(Elf32_Sym curr);

const char* getIndex(Elf32_Sym curr, char* buf);

#endif
