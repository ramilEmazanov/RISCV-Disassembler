#include <stdbool.h>
#include <stdint.h>

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Section;

typedef struct {
  Elf32_Off symtab_offset;
  Elf32_Word symtab_size;
  Elf32_Word strtab_offset;
  Elf32_Word symtab_entry_size;
  Elf32_Off text_offset;
  Elf32_Word text_size;
  Elf32_Off text_addr;
  Elf32_Off shstr_tab_offset;
  Elf32_Off e_shoff;
  Elf32_Half e_shentsize;
} ElfSectionInfo;

typedef struct {
  unsigned char e_ident[16];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct {
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Section st_shndx;
} Elf32_Sym;

typedef struct {
  Elf32_Addr address;
  char name[64];
} Symbol;

typedef struct {
  uint32_t address;
  uint32_t base;
  uint32_t opcode;
  uint32_t rd;
  uint32_t funct3;
  uint32_t rs1;
  uint32_t rs2;
  uint32_t funct7;
} Instruction;

typedef enum {
  ZERO_ARGS,
  TWO_ARGS,
  THREE_ARGS,
  IMM,
  B_JUMP,
  JAL,
  U_INSTR
} Format;

typedef struct {
  const char* op_name;
  Format format;
  char arg1[16];
  char arg2[16];
  char arg3[16];
  int arg_count;

  char symbol[64];
  bool has_symbol;

  char jump_symbol[64];
  bool has_jump_symbol;

  uint32_t raw_instr;
  uint32_t address;
  uint32_t jump_address;

} DecodedInstr;