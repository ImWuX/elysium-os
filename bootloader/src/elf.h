#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>
#include <fat32.h>

#define ELF_LITTLE_ENDIAN   1
#define ELF_ARCH_64         2
#define ELF_MACHINE_386     0x3E
#define ELF_VERSION         1

#define ELF_XINDEX 0xFFFF

#define ELF_IDENTIFIER0 0x7F
#define ELF_IDENTIFIER1 'E'
#define ELF_IDENTIFIER2 'L'
#define ELF_IDENTIFIER3 'F'

#define ELF64_SYMBOL_BIND(i)   ((i)>>4)
#define ELF64_SYMBOL_TYPE(i)   ((i)&0xf)
#define ELF64_SYMBOL_INFO(b,t) (((b)<<4)+((t)&0xf))

#define ELF64_REL_SYM(i)    ((i)>>32)
#define ELF64_REL_TYPE(i)   ((i)&0xffffffffL)
#define ELF64_REL_INFO(s,t) (((s)<<32)+((t)&0xffffffffL))

typedef uint64_t elf64_addr_t;
typedef uint64_t elf64_off_t;
typedef uint16_t elf64_half_t;
typedef uint32_t elf64_word_t;
typedef int32_t elf64_sword_t;
typedef uint64_t elf64_xword_t;
typedef int64_t elf64_sxword_t;

typedef struct {
    uint8_t file_identifier[4];
    uint8_t architecture;
    uint8_t endian;
    uint8_t header_version;
    uint8_t abi;
    uint8_t abi_version;
    uint8_t rsv0[6];
    uint8_t nident;
} __attribute__((packed)) elf_identifier_t;

typedef struct {
    elf_identifier_t identifier;
    elf64_half_t type;
    elf64_half_t machine;
    elf64_word_t version;
    elf64_addr_t entry;
    elf64_off_t program_header_offset;
    elf64_off_t section_header_offset;
    elf64_word_t flags;
    elf64_half_t header_size;
    elf64_half_t program_header_entry_size;
    elf64_half_t program_header_entry_count;
    elf64_half_t section_header_entry_size;
    elf64_half_t section_header_entry_count;
    elf64_half_t shstrtab_index;
} __attribute__((packed)) elf64_header_t;

typedef enum {
    ELF_NONE = 0,
    ELF_RELOCATABLE,
    ELF_EXECUTABLE,
    ELF_SHARED,
    ELF_CORE,
    ELF_LOOS = 0xfe00,
    ELF_HIOS = 0xfeff,
    ELF_LOPROC = 0xff00,
    ELF_HIPROC = 0xffff
} elf_type_t;

typedef struct {
    elf64_word_t type;
    elf64_word_t flags;
    elf64_off_t offset;
    elf64_addr_t vaddr;
    elf64_addr_t sys_v_abi_undefined;
    elf64_xword_t filesz;
    elf64_xword_t memsz;
    elf64_xword_t alignment;
} __attribute__((packed)) elf64_program_header_t;

typedef enum {
    PT_NULL_SEGMENT = 0,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR,
    PT_TLS,
    PT_LOOS = 0x60000000,
    PT_HIOS = 0x6fffffff,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff
} elf_program_segment_type_t;

typedef struct {
    elf64_word_t name;
    elf64_word_t type;
    elf64_xword_t flags;
    elf64_addr_t address;
    elf64_off_t offset;
    elf64_xword_t size;
    elf64_word_t link;
    elf64_word_t info;
    elf64_xword_t address_align;
    elf64_xword_t entsize;
} __attribute__((packed)) elf64_section_header_t;

typedef enum {
    ST_NULL_SECTION = 0,
    ST_PROGBITS,
    ST_SYMTAB,
    ST_STRTAB,
    ST_RELA,
    ST_HASH,
    ST_DYNAMIC,
    ST_NOTE,
    ST_NOBITS,
    ST_REL,
    ST_SHLIB,
    ST_DYNSYM,
    ST_INIT_ARRAY = 14,
    ST_FINI_ARRAY,
    ST_PREINIT_ARRAY,
    ST_GROUP,
    ST_SYMTAB_SHNDX,
    ST_NUM,
    ST_LOOS = 0x60000000,
    ST_HIOS = 0x6fffffff,
    ST_LOPROC = 0x70000000,
    ST_HIPROC = 0x7fffffff,
    ST_LOUSER = 0x80000000,
    ST_HIUSER = 0xffffffff
} elf_section_type_t;

typedef enum {
    SF_WRITE = 0x1,
    SF_ALLOC = 0x2,
    SF_EXECINSTR = 0x4,
    SF_MERGE = 0x10,
    SF_STRINGS = 0x20,
    SF_INFO_LINK = 0x40,
    SF_LINK_ORDER = 0x80,
    SF_OS_NONCONFORMING = 0x100,
    SF_GROUP = 0x200,
    SF_TLS = 0x400,
    SF_MASKOS = 0xFF00000,
    SF_MASKPROC = 0xF0000000,
    SF_ORDERED = 0x4000000,
    SF_EXCLUDE = 0x8000000,
} elf_section_flag_t;

typedef struct {
    elf64_word_t name;
    uint8_t info;
    uint8_t other;
    elf64_half_t section_index;
    elf64_addr_t value;
    elf64_xword_t size;
} __attribute__((packed)) elf64_symbol_t;

typedef enum {
    STT_NOTYPE = 0,
    STT_OBJECT,
    STT_FUNC,
    STT_SECTION,
    STT_FILE,
    STT_COMMON,
    STT_TLS,
    STT_LOOS = 10,
    STT_HIOS = 12,
    STT_LOPROC = 13,
    STT_HIPROC = 15
} elf_symbol_type_t;

typedef struct {
    elf64_addr_t offset;
    elf64_xword_t info;
} __attribute__((packed)) elf64_rel_t;

typedef struct {
    elf64_addr_t offset;
    elf64_xword_t info;
    elf64_sword_t addend;
} __attribute__((packed)) elf64_rela_t;

elf64_addr_t read_elf_file(file_descriptor_t *file_descriptor);

#endif