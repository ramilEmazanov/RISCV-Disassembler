#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf_args.h"

int parseSymtab(char* symtab_arr, int size) { return 0; }

int getEShoff() { return 0; }

int findSymtab() { return 0; }

uint32_t read32(unsigned char* buff, int ind) {
  return (buff[ind]) | (buff[ind + 1] << 8) | (buff[ind + 2] << 16) |
         (buff[ind + 3] << 24);
}

uint16_t read16(unsigned char* buff, int ind) {
  return (buff[ind]) | (buff[ind + 1] << 8);
}

Instruction parseInstr(uint32_t instr) {
  Instruction temp;
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
    case (0): {
      return "LOCAL";
    }
    case (1): {
      return "GLOBAL";
    }
    case (2): {
      return "WEAK";
    }
    default: {
      return "UNKNOWN";
    }
  }
}

const char* getType(Elf32_Sym curr) {
  switch (curr.st_info & 0xf) {
    case (0): {
      return "NOTYPE";
    }
    case (1): {
      return "OBJECT";
    }
    case (2): {
      return "FUNC";
    }
    case (3): {
      return "SECTION";
    }
    case (4): {
      return "FILE";
    }
    default : {
      return "UNKNOWN";
    }
  }
}

const char* getVis(Elf32_Sym curr) {
  switch (curr.st_other) {
    case (0): {
      return "DEFAULT";
    }
    case (1): {
      return "PROTECTED";
    }
    case (2): {
      return "HIDDEN";
    }
    case (3): {
      return "INTERNAL";
    }
    default: {
      return "UNKNOWN";
    }
  }
}

const char* getIndex(Elf32_Sym curr, char* buf) {
  switch (curr.st_shndx) {
    case 0x0000:
      return "UNDEF";

    case 0xfff1:
      return "ABS";

    case 0xfff2:
      return "COMMON";

    case 0xff00:
      return "LOPROC";

    case 0xff1f:
      return "HIPROC";

    default:
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

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: %s <input.elf> <output.txt>\n", argv[0]);
    return 1;
  }

  ////////////////////////////////////
  // READING FILE
  FILE* elf_file = fopen(argv[1], "rb");
  unsigned char* buffer = NULL;
  int buff_size = 0;

  if (!elf_file) {
    printf("Couldn't open input file: %s\n", argv[1]);
    return 1;
  }

  FILE* out = fopen(argv[2], "w");
  if (!out) {
    printf("Couldn't open output file: %s\n", argv[2]);
    fclose(elf_file);
    return 1;
  }

  int c;
  int i = 0;
  while ((c = fgetc(elf_file)) != EOF) {
    if (i >= buff_size) {
      buff_size += 1000;
      buffer = realloc(buffer, buff_size);
      if (!buffer) {
        return 1;
      }
    }
    buffer[i] = c;
    i++;
  }

  fclose(elf_file);
  int size = i;

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

  for (int i = 0; i < symbols_count; ++i) {
    Elf32_Off curr_offset = symtab_offset + i * symtab_entry_size;

    Elf32_Sym value;
    value.st_name = read32(buffer, curr_offset);
    value.st_value = read32(buffer, curr_offset + 4);
    value.st_size = read32(buffer, curr_offset + 8);
    value.st_info = buffer[curr_offset + 12];
    value.st_other = buffer[curr_offset + 13];
    value.st_shndx = read16(buffer, curr_offset + 14);

    symbols_info[i] = value;
  }

  Symbol* symbols = NULL;
  int symbols_size = 0;
  int symbols_capacity = 0;

  // MAKING MAP OF ADDRES -> NAME
  for (int i = 0; i < symbols_count; ++i) {
    Elf32_Sym curr = symbols_info[i];
    Symbol new_val;

    Elf32_Off name_start = strtab_offset + curr.st_name;

    if (buffer[name_start] == '\0') {
      continue;
    } else {
      if (symbols_capacity <= symbols_size) {
        symbols_capacity += 1000;
        symbols = realloc(symbols, symbols_capacity * sizeof(Symbol));
      }

      int j = 0;
      while (buffer[name_start + j] != '\0') {
        new_val.name[j] = buffer[name_start + j];
        j++;
      }

      new_val.name[j] = '\0';
      new_val.address = curr.st_value;
    }
    symbols[symbols_size] = new_val;
    symbols_size++;
  }

  for (int i = 0; i < symbols_size; i++) {
    fprintf(out, "0x%08x -> %s\n", symbols[i].address, symbols[i].name);
  }

  int instructions_count = text_size / 4;
  Instruction instrs[instructions_count];

  for (int i = 0; i < instructions_count; ++i) {
    Elf32_Off curr_instr_offset = text_offset + i * 4;
    Elf32_Off instr_addr = text_addr + i * 4;
    uint32_t instr = read32(buffer, curr_instr_offset);

    Instruction inst = parseInstr(instr);
    instrs[i] = inst;

    // printf("%08x: %08x\n", instr_addr, instr);
  }

  // PRINTING SYMTAB
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
      name = &buffer[strtab_offset + curr.st_name];
    }

    fprintf(out, "[%4i] 0x%-15X %5i %-8s %-8s %-8s %6s %s\n", i, curr.st_value,
            curr.st_size, type, bind, vis, index, name);
  }

  fclose(out);
  free(buffer);
  free(symbols);
}