SECTIONS {

   .text 0x00100000 : AT(0x00100000)
   /*
      This is the address the section will have when the output file is running.
      The second is the LMA, or load memory address.
      It is the address at which the section will be loaded.
   */
   {
      *kernel_start_asm.o(.text)
      * (.text*)
   }
   .data :
   {
      * (.rodata*)
      * (.data)
      . = ALIGN(4096);
   }
   DATA_SECTION_END = .;

   .bss :
   {
      * (.bss)
      . = ALIGN(4096);
   }
   BSS_SECTION_END = .;

   KERNEL_SECTIONS_END = .;

   . = ALIGN(4096);
   FIRST_PAGE_FRAME_ADDRESS = .;
}
