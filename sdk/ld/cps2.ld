OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

/* hardware register & memory map symbols (shared with cps2.hpp) */
/* PROVIDE(OBJ_RAM      = 0x700000); */
/* PROVIDE(CPSA         = 0x800100); */
/* PROVIDE(CPSB         = 0x800140); */
/* PROVIDE(REG_IN0      = 0x804000); */
/* PROVIDE(REG_IN1      = 0x804010); */
/* PROVIDE(REG_IN2      = 0x804020); */
/* PROVIDE(REG_OBJ_BANK = 0x8040e0); */
/* PROVIDE(GFX_RAM      = 0x900000); */

MEMORY
{
  ROM  (rx)  : ORIGIN = 0x000000, LENGTH = 4M
  WRAM (rwx) : ORIGIN = 0xff0000, LENGTH = 64K
}

SECTIONS
{
  .text :
  {
    KEEP(*(vectors .vectors))
    *(text .text .text.*)
    . = ALIGN(4);
  } > ROM

  .rodata :
  {
    *(.rodata .rodata.* .gnu.linkonce.r.*)

    . = ALIGN(4);
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP(*(.init_array))
    __init_array_end = .;

    . = ALIGN(4);
    _etext = .;
  } > ROM

  .data : AT(_etext)
  {
    _sdata = .;
    *(data .data .data.*)
    . = ALIGN(4);
    _edata = .;
  } > WRAM

  .bss (NOLOAD) :
  {
    _sbss = .;
    *(bss .bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > WRAM

  /DISCARD/ : {
    *(.note*)
    *(.comment)
    *(.eh_frame*)
  }

  ASSERT((_etext + SIZEOF(.data)) <= (ORIGIN(ROM) + LENGTH(ROM)), "Error: ROM image exceeds 4 MB CPS2 limit")
  ASSERT(_ebss <= (ORIGIN(WRAM) + LENGTH(WRAM)), "Error: WRAM (.data + .bss) exceeds 64 KB CPS2 limit")
}
