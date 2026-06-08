#include "binary_reader.h"

#include <stdio.h>

uint32_t read32(unsigned char* buff, int ind) {
  return (buff[ind]) | (buff[ind + 1] << 8) | (buff[ind + 2] << 16) |
         (buff[ind + 3] << 24);
}

uint16_t read16(unsigned char* buff, int ind) {
  return (buff[ind]) | (buff[ind + 1] << 8);
}

Instruction parseInstr(uint32_t instr) {
  Instruction temp;
  temp.base = instr;
  temp.opcode = instr & 0x7f;
  temp.rd = (instr >> 7) & 0x1f;
  temp.funct3 = (instr >> 12) & 0x7;
  temp.rs1 = (instr >> 15) & 0x1f;
  temp.rs2 = (instr >> 20) & 0x1f;
  temp.funct7 = (instr >> 25) & 0x7f;
  return temp;
}

const char* getBind(Elf32_Sym curr) {
  switch (curr.st_info >> 4) {
    case 0: {
      return "LOCAL";
    }
    case 1: {
      return "GLOBAL";
    }
    case 2: {
      return "WEAK";
    }
    default: {
      return "UNKNOWN";
    }
  }
}

const char* getType(Elf32_Sym curr) {
  switch (curr.st_info & 0xf) {
    case 0: {
      return "NOTYPE";
    }
    case 1: {
      return "OBJECT";
    }
    case 2: {
      return "FUNC";
    }
    case 3: {
      return "SECTION";
    }
    case 4: {
      return "FILE";
    }
    default: {
      return "UNKNOWN";
    }
  }
}

const char* getVis(Elf32_Sym curr) {
  switch (curr.st_other) {
    case 0: {
      return "DEFAULT";
    }
    case 1: {
      return "PROTECTED";
    }
    case 2: {
      return "HIDDEN";
    }
    case 3: {
      return "INTERNAL";
    }
    default: {
      return "UNKNOWN";
    }
  }
}

const char* getIndex(Elf32_Sym curr, char* buf) {
  switch (curr.st_shndx) {
    case 0x0000: {
      return "UND";
    }
    case 0xfff1: {
      return "ABS";
    }
    case 0xfff2: {
      return "COMMON";
    }
    case 0xff00: {
      return "LOPROC";
    }
    case 0xff1f: {
      return "HIPROC";
    }
    default: {
      if (curr.st_shndx >= 0xff00 && curr.st_shndx <= 0xff1f) {
        return "PROC";
      }

      if (curr.st_shndx >= 0xff00 && curr.st_shndx <= 0xffff) {
        return "RESERVE";
      }

      sprintf(buf, "%u", curr.st_shndx);
      return buf;
    }
  }
}
