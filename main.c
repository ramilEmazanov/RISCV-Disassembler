#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf_args.h"

uint32_t read32(unsigned char* buff, int ind) {
  return (buff[ind]) | (buff[ind + 1] << 8) | (buff[ind + 2] << 16) |
         (buff[ind + 3] << 24);
}

uint16_t read16(unsigned char* buff, int ind) {
  return (buff[ind]) | (buff[ind + 1] << 8);
}

Instruction parseInstr(uint32_t instr, uint32_t address) {
  Instruction temp;
  temp.address = address;
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

const char* parseSysInstr(Instruction instr) {
  switch (instr.funct3) {
    case 0x0: {
      if (instr.base == 0x00000073) {
        return "ecall";
      }
      if (instr.base == 0x00100073) {
        return "ebreak";
      }
      if (instr.base == 0x0100000f) {
        return "pause";
      }
      break;
    }
  }
  return "unknown_system";
}

void printSymtab(FILE* out, unsigned char* buffer, Elf32_Sym* symbols_info,
                 int symbols_count, Elf32_Word strtab_offset,
                 Elf32_Off shstr_tab_offset, Elf32_Off e_shoff,
                 Elf32_Half e_shentsize) {
  fprintf(out, "\n.symtab\n");
  fprintf(out,
          "Symbol Value              Size Type     Bind     Vis       Index "
          "Name\n");

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
    } else if (curr.st_shndx != 0 && curr.st_shndx < 0xff00) {
      Elf32_Off sh_header = e_shoff + curr.st_shndx * e_shentsize;
      Elf32_Word sh_name = read32(buffer, sh_header);
      name = (char*)&buffer[shstr_tab_offset + sh_name];
    }

    fprintf(out, "[%4i] 0x%-15X %5i %-8s %-8s %-8s %6s %s\n", i, curr.st_value,
            curr.st_size, type, bind, vis, index, name);
  }
}

void fillInstrs(unsigned char* buffer, Instruction* instrs, int count,
                Elf32_Off text_offset, Elf32_Off text_addr) {
  for (int i = 0; i < count; ++i) {
    instrs[i] =
        parseInstr(read32(buffer, text_offset + i * 4), text_addr + i * 4);
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

    if (buffer[name_start] == '\0' || buffer[name_start] == '$') {
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

void fillDecodedInsts(Instruction* instrs, int instructions_count,
                      DecodedInstr* decoded_insts) {
  const char* regs[32] = {"zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
                          "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                          "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
                          "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

  for (int i = 0; i < instructions_count; ++i) {
    Instruction curr = instrs[i];
    const char* op_name = "unknown_instruction";
    DecodedInstr decoded;
    decoded.raw_instr = curr.base;
    decoded.address = curr.address;

    switch (curr.opcode) {
      case 0x33: {
        decoded.op_name = parseRInstr(curr);
        decoded.format = THREE_ARGS;
        snprintf(decoded.arg1, sizeof(decoded.arg1), "%s", regs[curr.rd]);
        snprintf(decoded.arg2, sizeof(decoded.arg2), "%s", regs[curr.rs1]);
        snprintf(decoded.arg3, sizeof(decoded.arg3), "%s", regs[curr.rs2]);
        decoded.arg_count = 3;
        break;
      }
      case 0x13: {
        decoded.op_name = parseIInstr(curr);
        decoded.format = THREE_ARGS;
        snprintf(decoded.arg1, sizeof(decoded.arg1), "%s", regs[curr.rd]);
        snprintf(decoded.arg2, sizeof(decoded.arg2), "%s", regs[curr.rs1]);

        if (curr.funct3 == 0x1 || curr.funct3 == 0x5) {
          snprintf(decoded.arg3, sizeof(decoded.arg3), "%u", curr.rs2);
        } else {
          snprintf(decoded.arg3, sizeof(decoded.arg3), "%d",
                   (int32_t)curr.base >> 20);
        }
        decoded.arg_count = 3;
        break;
      }
      case 0x03: {
        decoded.op_name = parseIInstr(curr);
        decoded.format = IMM;
        int32_t imm_load = (int32_t)curr.base >> 20;
        snprintf(decoded.arg1, sizeof(decoded.arg1), "%s", regs[curr.rd]);
        snprintf(decoded.arg2, sizeof(decoded.arg2), "%s", regs[curr.rs1]);
        snprintf(decoded.arg3, sizeof(decoded.arg3), "%d", imm_load);
        decoded.arg_count = 3;
        break;
      }
      case 0x67: {
        decoded.op_name = parseIInstr(curr);
        decoded.format = IMM;
        int32_t imm_jalr = (int32_t)curr.base >> 20;
        snprintf(decoded.arg1, sizeof(decoded.arg1), "%s", regs[curr.rd]);
        snprintf(decoded.arg2, sizeof(decoded.arg2), "%s", regs[curr.rs1]);
        snprintf(decoded.arg3, sizeof(decoded.arg3), "%d", imm_jalr);
        decoded.arg_count = 3;
        break;
      }
      case 0x23: {
        decoded.op_name = parseSInstr(curr);
        decoded.format = IMM;
        int32_t imm_s = (int32_t)((curr.funct7 << 5) | curr.rd) << 20 >> 20;
        snprintf(decoded.arg1, sizeof(decoded.arg1), "%s", regs[curr.rs2]);
        snprintf(decoded.arg2, sizeof(decoded.arg2), "%s", regs[curr.rs1]);
        snprintf(decoded.arg3, sizeof(decoded.arg3), "%d", imm_s);
        decoded.arg_count = 3;
        break;
      }
      case 0x63: {
        decoded.op_name = parseBInstr(curr);
        decoded.format = B_JUMP;
        int32_t imm_b = ((curr.funct7 >> 6) & 0x1) << 12 |
                        (curr.rd & 0x1) << 11 | (curr.funct7 & 0x3f) << 5 |
                        (curr.rd >> 1) << 1;
        imm_b = imm_b << 19 >> 19;
        decoded.jump_address = decoded.address + imm_b;

        snprintf(decoded.arg1, sizeof(decoded.arg1), "%s", regs[curr.rs1]);
        snprintf(decoded.arg2, sizeof(decoded.arg2), "%s", regs[curr.rs2]);
        snprintf(decoded.arg3, sizeof(decoded.arg3), "%d", imm_b);
        decoded.arg_count = 3;
        break;
      }
      case 0x37:
      case 0x17: {
        decoded.op_name = parseUInstr(curr);
        decoded.format = U_INSTR;
        snprintf(decoded.arg1, sizeof(decoded.arg1), "%s", regs[curr.rd]);
        snprintf(decoded.arg2, sizeof(decoded.arg2), "%d",
                 (int32_t)curr.base >> 12);
        decoded.arg_count = 2;
        break;
      }
      case 0x6f: {
        decoded.op_name = "jal";
        decoded.format = JAL;
        int32_t imm_j = ((curr.funct7 >> 6) & 0x1) << 20 |
                        ((curr.rs1 << 3) | curr.funct3) << 12 |
                        (curr.rs2 & 0x1) << 11 | (curr.funct7 & 0x3f) << 5 |
                        ((curr.rs2 >> 1) & 0xf) << 1;
        imm_j = imm_j << 11 >> 11;
        decoded.jump_address = decoded.address + imm_j;

        snprintf(decoded.arg1, sizeof(decoded.arg1), "%s", regs[curr.rd]);
        snprintf(decoded.arg2, sizeof(decoded.arg2), "%d", imm_j);
        decoded.arg_count = 2;
        break;
      }
      case 0x73: {
        decoded.op_name = parseSysInstr(curr);
        decoded.format = ZERO_ARGS;
        decoded.arg_count = 0;
        break;
      }
      case 0x0f: {
        decoded.op_name = "unknown_instruction";
        decoded.format = ZERO_ARGS;
        decoded.arg_count = 0;
        break;
      }
    }
    decoded_insts[i] = decoded;
  }
}

void fillSymbolsForInstruction(Symbol* symbols, int symbols_size,
                               DecodedInstr* decoded_insts,
                               int instructions_count,
                               Symbol* target_symbols_map,
                               int target_symbols_size) {
  for (int i = 0; i < instructions_count; ++i) {
    decoded_insts[i].has_symbol = false;
    decoded_insts[i].has_jump_symbol = false;

    bool find = false;

    for (int j = 0; j < symbols_size; ++j) {
      if (decoded_insts[i].address == symbols[j].address) {
        decoded_insts[i].has_symbol = true;
        strcpy(decoded_insts[i].symbol, symbols[j].name);
        find = true;
        break;
      }
    }

    if (find) {
      continue;
    }

    for (int j = 0; j < target_symbols_size; ++j) {
      if (decoded_insts[i].address == target_symbols_map[j].address) {
        decoded_insts[i].has_symbol = true;
        strcpy(decoded_insts[i].symbol, target_symbols_map[j].name);
        break;
      }
    }
  }
}

void fillJumpSymbols(DecodedInstr* decoded_insts, int instructions_count,
                     Symbol* symbols, int symbols_size,
                     Symbol** target_symbols_map, int* target_symbols_size) {
  int symbol_counter = 0;
  int capacity = *target_symbols_size;

  for (int i = 0; i < instructions_count; ++i) {
    if (decoded_insts[i].format != B_JUMP && decoded_insts[i].format != JAL) {
      continue;
    }

    bool find = false;

    for (int j = 0; j < symbols_size; ++j) {
      if (decoded_insts[i].jump_address == symbols[j].address) {
        decoded_insts[i].has_jump_symbol = true;
        strcpy(decoded_insts[i].jump_symbol, symbols[j].name);
        find = true;
        break;
      }
    }

    if (find) {
      continue;
    }

    for (int j = 0; j < symbol_counter; ++j) {
      if ((*target_symbols_map)[j].address == decoded_insts[i].jump_address) {
        strcpy(decoded_insts[i].jump_symbol, (*target_symbols_map)[j].name);
        decoded_insts[i].has_jump_symbol = true;
        find = true;
        break;
      }
    }

    if (find) {
      continue;
    }

    if (symbol_counter >= capacity) {
      capacity += 10;
      *target_symbols_map =
          realloc(*target_symbols_map, capacity * sizeof(Symbol));
    }

    Symbol temp;
    temp.address = decoded_insts[i].jump_address;
    snprintf(temp.name, sizeof(temp.name), "L%d", symbol_counter);

    strcpy(decoded_insts[i].jump_symbol, temp.name);
    decoded_insts[i].has_jump_symbol = true;

    (*target_symbols_map)[symbol_counter] = temp;
    symbol_counter++;
  }

  *target_symbols_size = symbol_counter;
}

void printText(DecodedInstr* decoded_insts, int instructions_count, FILE* out) {
  fprintf(out, ".text\n");
  for (int i = 0; i < instructions_count; ++i) {
    DecodedInstr curr = decoded_insts[i];

    if (curr.has_symbol) {
      fprintf(out, "%08x <%s>:\n", curr.address, curr.symbol);
    }

    switch (curr.format) {
      case (ZERO_ARGS): {
        fprintf(out, "   %05x:\t%08x\t%7s\n", curr.address, curr.raw_instr,
                curr.op_name);
        break;
      }
      case (TWO_ARGS): {
        fprintf(out, "   %05x:\t%08x\t%7s\t%s, %s\n", curr.address,
                curr.raw_instr, curr.op_name, curr.arg1, curr.arg2);
        break;
      }
      case (THREE_ARGS): {
        fprintf(out, "   %05x:\t%08x\t%7s\t%s, %s, %s\n", curr.address,
                curr.raw_instr, curr.op_name, curr.arg1, curr.arg2, curr.arg3);
        break;
      }
      case (IMM): {
        fprintf(out, "   %05x:\t%08x\t%7s\t%s, %s(%s)\n", curr.address,
                curr.raw_instr, curr.op_name, curr.arg1, curr.arg3, curr.arg2);
        break;
      }
      case (B_JUMP): {
        fprintf(out, "   %05x:\t%08x\t%7s\t%s, %s, %s ; 0x%x\n", curr.address,
                curr.raw_instr, curr.op_name, curr.arg1, curr.arg2,
                curr.jump_symbol, curr.jump_address);

        break;
      }
      case (JAL): {
        fprintf(out, "   %05x:\t%08x\t%7s\t%s, <%s> ; 0x%x\n", curr.address,
                curr.raw_instr, curr.op_name, curr.arg1, curr.jump_symbol,
                curr.jump_address);
        break;
      }
      case (U_INSTR): {
        fprintf(out, "   %05x:\t%08x\t%7s\t%s, %s\n", curr.address,
                curr.raw_instr, curr.op_name, curr.arg1, curr.arg2);
        break;
      }
      default: {
        continue;
      }
    }
  }
}

ElfSectionInfo parseElfHeaders(unsigned char* buffer) {
  ElfSectionInfo info = {0};

  Elf32_Off e_shoff = read32(buffer, 32);
  Elf32_Half e_shentsize = read16(buffer, 46);
  Elf32_Half e_shnum = read16(buffer, 48);
  Elf32_Half e_shstrndx = read16(buffer, 50);

  Elf32_Off shstr_tab_header = e_shoff + e_shentsize * e_shstrndx;
  Elf32_Off shstr_tab_offset = read32(buffer, shstr_tab_header + 16);

  info.shstr_tab_offset = shstr_tab_offset;
  info.e_shoff = e_shoff;
  info.e_shentsize = e_shentsize;

  for (int i = 0; i < e_shnum; i++) {
    Elf32_Off curr_header = e_shoff + i * e_shentsize;
    Elf32_Word sh_name = read32(buffer, curr_header);
    char* name = (char*)&buffer[shstr_tab_offset + sh_name];

    if (strcmp(name, ".symtab") == 0) {
      info.symtab_offset = read32(buffer, curr_header + 16);
      info.symtab_size = read32(buffer, curr_header + 20);
      Elf32_Word strtab_index = read32(buffer, curr_header + 24);
      Elf32_Off strtab_header = e_shoff + strtab_index * e_shentsize;
      info.strtab_offset = read32(buffer, strtab_header + 16);
      info.symtab_entry_size = read32(buffer, curr_header + 36);
    }

    if (strcmp(name, ".text") == 0) {
      info.text_addr = read32(buffer, curr_header + 12);
      info.text_offset = read32(buffer, curr_header + 16);
      info.text_size = read32(buffer, curr_header + 20);
    }
  }

  return info;
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

  ElfSectionInfo sections = parseElfHeaders(buffer);

  int symbols_count = sections.symtab_size / sections.symtab_entry_size;
  Elf32_Sym symbols_info[symbols_count];
  fillSymbolsInfo(buffer, symbols_info, symbols_count, sections.symtab_offset,
                  sections.symtab_entry_size);

  int symbols_size = 0;
  Symbol* symbols = buildSymbolMap(buffer, symbols_info, symbols_count,
                                   sections.strtab_offset, &symbols_size);

  int instructions_count = sections.text_size / 4;
  Instruction instrs[instructions_count];
  fillInstrs(buffer, instrs, instructions_count, sections.text_offset,
             sections.text_addr);

  DecodedInstr decoded_insts[instructions_count];
  fillDecodedInsts(instrs, instructions_count, decoded_insts);

  Symbol* target_symbols_map = NULL;
  int target_symbols_size = 0;
  fillJumpSymbols(decoded_insts, instructions_count, symbols, symbols_size,
                  &target_symbols_map, &target_symbols_size);

  fillSymbolsForInstruction(symbols, symbols_size, decoded_insts,
                            instructions_count, target_symbols_map,
                            target_symbols_size);

  printText(decoded_insts, instructions_count, out);
  printSymtab(out, buffer, symbols_info, symbols_count, sections.strtab_offset,
              sections.shstr_tab_offset, sections.e_shoff,
              sections.e_shentsize);

  fclose(out);
  free(buffer);
  free(symbols);
  free(target_symbols_map);
}