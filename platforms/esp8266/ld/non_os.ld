/* This linker script generated from xt-genldscripts.tpp for LSP . */
/* Linker Script for ld -N */
MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = _IROM0ADDR, len = _IROM0SIZE
}

PHDRS
{
  dram0_0_phdr PT_LOAD;
  dram0_0_bss_phdr PT_LOAD;
  iram1_0_phdr PT_LOAD;
  irom0_0_phdr PT_LOAD;
}

/*  Default entry point:  */
ENTRY(call_user_start)
EXTERN(_DebugExceptionVector)
EXTERN(_OurDebugExceptionVector)
EXTERN(_NMIExceptionVector)
EXTERN(_OurNMIExceptionVector)
EXTERN(_KernelExceptionVector)
EXTERN(_OurKernelExceptionVector)
EXTERN(_UserExceptionVector)
EXTERN(_OurUserExceptionVector)
EXTERN(_DoubleExceptionVector)
EXTERN(_OurDoubleExceptionVector)

SECTIONS
{
  .data : ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
    */libc.a:(.data .data.*)
    */libhal.a:(.data .data.* .rodata SORT_BY_NAME(.rodata.*))
    */ESP8266_*_SDK/lib*.a:(.data .data.*)
    */liblwip*.a:(.data .data.*)
    */libmbed*.a:(.data .data.*)
    */lib*.a:(.data .data.*)
    */mongoose-os.a:(.data .data.*)
    *(.data)
    *(.data.*)
    _data_end = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_phdr

  .rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);
    _rodata_end = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_phdr

  .bss ALIGN(8) (NOLOAD) : ALIGN(4)
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    _heap_start = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_bss_phdr

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.OurVectors.text)
    . = ALIGN (4);
    *(.DebugExceptionVector.text)
    *(.NMIExceptionVector.text)
    *(.KernelExceptionVector.text)
    *(.UserExceptionVector.text)
    *(.DoubleExceptionVector.text)
    *(.UserEnter.text)
    . = ALIGN (16);
    /* Note: Could reclaim a couple K by only pinning integer operations from libgcc */
    */libgcc.a:(.literal .text SORT_BY_NAME(.literal.*) SORT_BY_NAME(.text.*))
    */libmain.a:(SORT_BY_NAME(.literal.*) SORT_BY_NAME(.text.*))
    */libpp.a:(SORT_BY_NAME(.literal.*) SORT_BY_NAME(.text.*))
    */ESP8266_*_SDK/lib*.a:(.literal .text)
    *(SORT_BY_NAME(.literal.IRAM.*) SORT_BY_NAME(.text.IRAM.*))
    *(.fini.literal .fini)
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >iram1_0_seg :iram1_0_phdr

  .irom0.text : ALIGN(4)
  {
    _irom0_text_start = ABSOLUTE(.);

    /* This is actually read-only data, but section names have to be kept the way esptool expects them. */
    *(.sdk.version)
    *(.rodata SORT_BY_NAME(.rodata.*))

    /*  C++ constructor and destructor tables, properly ordered:  */
    . = ALIGN(4);
    __init_array_start = ABSOLUTE(.);
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    __init_array_end = ABSOLUTE(.);
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))

    . = ALIGN(4096);
    */libc.a:(SORT_BY_NAME(.literal.*) .irom.text .text SORT_BY_NAME(.text.*))
    */libstd*.a:(.literal SORT_BY_NAME(.literal.*) .irom.text .text SORT_BY_NAME(.text.*))
    */ESP8266_*_SDK/lib*.a:(.irom0.literal .literal.* .irom0.text SORT_BY_NAME(.text.*))
    */liblwip*.a:(SORT_BY_NAME(.literal.*) .irom.text .text SORT_BY_NAME(.text.*))
    */libmbed*.a:(SORT_BY_NAME(.literal.*) .irom.text .text SORT_BY_NAME(.text.*))
    */lib*.a:(SORT_BY_NAME(.literal.*) .irom.text .text SORT_BY_NAME(.text.*))
    */mongoose-os.a:(SORT_BY_NAME(.literal.*) .irom.text .text SORT_BY_NAME(.text.*))
    *(.literal SORT_BY_NAME(.literal.*) .irom.text .text SORT_BY_NAME(.text.*))
    _irom0_text_end = ABSOLUTE(.);
  } >irom0_0_seg :irom0_0_phdr
}

/* get ROM code address */
INCLUDE "../ld/eagle.rom.addr.v6.ld"

PROVIDE ( mgos_usleep = 0x40002ecc );  /* AKA ets_delay_us */
PROVIDE ( ets_task_top_of_stack = 0x3ffff000 );  /* All tasks share a 4K stack */
