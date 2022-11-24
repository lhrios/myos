SECTIONS {

   .text 0x40000000:
   {
      *executable_entry_point_asm.o(.text)
      * (.text*)
   }
   .data :
   {
      * (.bss)
      * (COMMON)
      * (.rodata*)
      * (.data)
   }
}
