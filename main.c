#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "binary_reader.h"
#include "elf_args.h"

const char* parseIInstr(Instruction instr) {
  switch (instr.opcode) {
    case 0x13: {
      switch (instr.funct3) {
        case 0x0: {
          return "addi";
        }
        case 0x1: {
          if (instr.funct7 == 0x00) {
            return "slli";
          }
          break;
        }
        case 0x2: {
          return "slti";
        }
        case 0x3: {
          return "sltiu";
        }
        case 0x4: {
          return "xori";
        }
        case 0x5: {
          if (instr.funct7 == 0x00) {
            return "srli";
          } else if (instr.funct7 == 0x20) {
            return "srai";
          }
          break;
        }
        case 0x6: {
          return "ori";
        }
        case 0x7: {
          return "andi";
        }
      }
      break;
    }
    case 0x03: {
      switch (instr.funct3) {
        case 0x0: {
          return "lb";
        }
        case 0x1: {
          return "lh";
        }
        case 0x2: {
          return "lw";
        }
        case 0x4: {
          return "lbu";
        }
        case 0x5: {
          return "lhu";
        }
      }
      break;
    }
    case 0x67: {
      if (instr.funct3 == 0x0) {
        return "jalr";
      }
      break;
    }
  }
  return "unknown_instruction";
}

const char* parseUInstr(Instruction instr) {
  switch (instr.opcode) {
    case 0x37: {
      return "lui";
    }
    case 0x17: {
      return "auipc";
    }
  }
  return "unknown_instruction";
}

const char* parseBInstr(Instruction instr) {
  switch (instr.funct3) {
    case 0x0: {
      return "beq";
    }
    case 0x1: {
      return "bne";
    }
    case 0x4: {
      return "blt";
    }
    case 0x5: {
      return "bge";
    }
    case 0x6: {
      return "bltu";
    }
    case 0x7: {
      return "bgeu";
    }
  }
  return "unknown_instruction";
}

const char* parseSInstr(Instruction instr) {
  switch (instr.funct3) {
    case 0x0: {
      return "sb";
    }
    case 0x1: {
      return "sh";
    }
    case 0x2: {
      return "sw";
    }
  }
  return "unknown_instruction";
}

const char* parseRInstr(Instruction instr) {
  switch (instr.funct3) {
    case 0x0: {
      if (instr.funct7 == 0x00) {
        return "add";
      } else if (instr.funct7 == 0x20) {
        return "sub";
      } else if (instr.funct7 == 0x01) {
        return "mul";
      }
      break;
    }
    case 0x1: {
      if (instr.funct7 == 0x00) {
        return "sll";
      } else if (instr.funct7 == 0x01) {
        return "mulh";
      }
      break;
    }
    case 0x2: {
      if (instr.funct7 == 0x00) {
        return "slt";
      } else if (instr.funct7 == 0x01) {
        return "mulhsu";
      }
      break;
    }
    case 0x3: {
      if (instr.funct7 == 0x00) {
        return "sltu";
      } else if (instr.funct7 == 0x01) {
        return "mulhu";
      }
      break;
    }
    case 0x4: {
      if (instr.funct7 == 0x00) {
        return "xor";
      } else if (instr.funct7 == 0x01) {
        return "div";
      }
      break;
    }
    case 0x5: {
      if (instr.funct7 == 0x00) {
        return "srl";
      } else if (instr.funct7 == 0x20) {
        return "sra";
      } else if (instr.funct7 == 0x01) {
        return "divu";
      }
      break;
    }
    case 0x6: {
      if (instr.funct7 == 0x00) {
        return "or";
      } else if (instr.funct7 == 0x01) {
        return "rem";
      }
      break;
    }
    case 0x7: {
      if (instr.funct7 == 0x00) {
        return "and";
      } else if (instr.funct7 == 0x01) {
        return "remu";
      }
      break;
    }
  }
  return "unknown_instruction";
}

void printSymtab(FILE* out, unsigned char* buffer, Elf32_Sym* symbols_info,
                 int symbols_count, Elf32_Word strtab_offset) {
  fprintf(out,
          "Symbol Value            Size Type    Bind    Vis     Index Name\n");

  for (int i = 0; i < symbols_count; ++i) {
    Elf32_Sym curr = symbols_info[i];

    const char* bind = getBind(curr);
    const char* type = getType(curr);
    const char* vis = getVis(curr);

    char index_buf[32];
    const char* index = getIndex(curr, index_buf);

    char* name = "";
    if (curr.st_name != 0) {
      name = (char*)&buffer[strtab_offset + curr.st_name];
    }

    fprintf(out, "[%4i] 0x%-15X %5i %-8s %-8s %-8s %6s %s\n", i, curr.st_value,
            curr.st_size, type, bind, vis, index, name);
  }
}

void fillInstrs(unsigned char* buffer, Instruction* instrs, int count,
                Elf32_Off text_offset) {
  for (int i = 0; i < count; ++i) {
    instrs[i] = parseInstr(read32(buffer, text_offset + i * 4));
  }
}

Symbol* buildSymbolMap(unsigned char* buffer, Elf32_Sym* symbols_info,
                       int symbols_count, Elf32_Word strtab_offset,
                       int* out_size) {
  Symbol* symbols = NULL;
  int symbols_size = 0;
  int symbols_capacity = 0;

  for (int i = 0; i < symbols_count; ++i) {
    Elf32_Sym curr = symbols_info[i];
    Elf32_Off name_start = strtab_offset + curr.st_name;

    if (buffer[name_start] == '\0') {
      continue;
    }

    if (symbols_capacity <= symbols_size) {
      symbols_capacity += 1000;
      symbols = realloc(symbols, symbols_capacity * sizeof(Symbol));
    }

    Symbol new_val;
    int j = 0;
    while (buffer[name_start + j] != '\0') {
      new_val.name[j] = buffer[name_start + j];
      j++;
    }
    new_val.name[j] = '\0';
    new_val.address = curr.st_value;

    symbols[symbols_size++] = new_val;
  }

  *out_size = symbols_size;
  return symbols;
}

void fillSymbolsInfo(unsigned char* buffer, Elf32_Sym* symbols_info, int count,
                     Elf32_Off symtab_offset, Elf32_Word entry_size) {
  for (int i = 0; i < count; ++i) {
    Elf32_Off off = symtab_offset + i * entry_size;
    symbols_info[i].st_name = read32(buffer, off);
    symbols_info[i].st_value = read32(buffer, off + 4);
    symbols_info[i].st_size = read32(buffer, off + 8);
    symbols_info[i].st_info = buffer[off + 12];
    symbols_info[i].st_other = buffer[off + 13];
    symbols_info[i].st_shndx = read16(buffer, off + 14);
  }
}

unsigned char* readFile(const char* filename, int* out_size) {
  FILE* f = fopen(filename, "rb");
  if (!f) {
    printf("Couldn't open input file: %s\n", filename);
    return NULL;
  }

  unsigned char* buffer = NULL;
  int buff_size = 0;
  int c;
  int i = 0;

  while ((c = fgetc(f)) != EOF) {
    if (i >= buff_size) {
      buff_size += 1000;
      unsigned char* tmp = realloc(buffer, buff_size);
      buffer = tmp;
    }
    buffer[i++] = c;
  }

  fclose(f);
  *out_size = i;
  return buffer;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: %s <input.elf> <output.txt>\n", argv[0]);
    return 1;
  }

  int size = 0;
  unsigned char* buffer = readFile(argv[1], &size);
  if (!buffer) {
    return 1;
  }

  FILE* out = fopen(argv[2], "w");
  if (!out) {
    printf("Couldn't open output file: %s\n", argv[2]);
    free(buffer);
    return 1;
  }

  // FINDING SYMTAB AND TEXT INFO
  Elf32_Off e_shoff = read32(buffer, 32);
  Elf32_Half e_shentsize = read16(buffer, 46);
  Elf32_Half e_shnum = read16(buffer, 48);
  Elf32_Half e_shstrndx = read16(buffer, 50);

  Elf32_Off shstr_tab_header = e_shoff + e_shentsize * e_shstrndx;
  Elf32_Off shstr_tab_offset = read32(buffer, shstr_tab_header + 16);

  Elf32_Off symtab_offset = 0;
  Elf32_Word symtab_size = 0;
  Elf32_Word strtab_offset = 0;
  Elf32_Word strtab_index = 0;
  Elf32_Word symtab_entry_size = 0;

  Elf32_Off text_offset = 0;
  Elf32_Word text_size = 0;
  Elf32_Off text_addr = 0;

  for (int i = 0; i < e_shnum; i++) {
    Elf32_Off curr_header = e_shoff + i * e_shentsize;
    Elf32_Word sh_name = read32(buffer, curr_header);

    char* name = &buffer[shstr_tab_offset + sh_name];

    if (strcmp(name, ".symtab") == 0) {
      symtab_offset = read32(buffer, curr_header + 16);
      symtab_size = read32(buffer, curr_header + 20);
      strtab_index = read32(buffer, curr_header + 24);

      Elf32_Off strtab_header = e_shoff + strtab_index * e_shentsize;
      strtab_offset = read32(buffer, strtab_header + 16);

      symtab_entry_size = read32(buffer, curr_header + 36);
    }

    if (strcmp(name, ".text") == 0) {
      text_addr = read32(buffer, curr_header + 12);
      text_offset = read32(buffer, curr_header + 16);
      text_size = read32(buffer, curr_header + 20);
    }
  }

  // FINDING SYMBOLS_INFO
  int symbols_count = symtab_size / symtab_entry_size;
  Elf32_Sym symbols_info[symbols_count];
  fillSymbolsInfo(buffer, symbols_info, symbols_count, symtab_offset,
                  symtab_entry_size);

  // MAKING MAP OF ADDRESS -> NAME
  int symbols_size = 0;
  Symbol* symbols = buildSymbolMap(buffer, symbols_info, symbols_count,
                                   strtab_offset, &symbols_size);

  // for (int i = 0; i < symbols_size; i++) {
  //   fprintf(out, "0x%08x -> %s\n", symbols[i].address, symbols[i].name);
  // }

  // Filling array of instructions
  int instructions_count = text_size / 4;
  Instruction instrs[instructions_count];
  fillInstrs(buffer, instrs, instructions_count, text_offset);

  // Printing instructions
  const char* regs[32] = {"zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
                          "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                          "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
                          "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

  // Нужна отдельная для вывода еще структура : название операции, регистры,
  // символ
  for (int i = 0; i < instructions_count; ++i) {
    Instruction curr = instrs[i];
    const char* op_name = "unknown_instruction";

    switch (curr.opcode) {
      case 0x33: {
        op_name = parseRInstr(curr);
        break;
      }
      case 0x13:
      case 0x03:
      case 0x67: {
        op_name = parseIInstr(curr);
        break;
      }
      case 0x23: {
        op_name = parseSInstr(curr);
        break;
      }
      case 0x63: {
        op_name = parseBInstr(curr);
        break;
      }
      case 0x37:
      case 0x17: {
        op_name = parseUInstr(curr);
        break;
      }
      case 0x6f: {
        op_name = "jal";
        break;
      }
    }
  }

  // PRINTING SYMTAB
  printSymtab(out, buffer, symbols_info, symbols_count, strtab_offset);

  fclose(out);
  free(buffer);
  free(symbols);
}