/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x23000, LENGTH = 0x1D000  /* Length was chosen arbitrarily */

  /* WARNING: Must use the ram allocation of the first major version of a SoftDevice. WARNING */
  /* Subsequent releases might have a smaller ram usage and the app will crash with an older SoftDevice! */
  RAM (rwx) :  ORIGIN = 0x20003810, LENGTH = 0x4750
  /*RAM (rwx) :  ORIGIN = 0x200046D0, LENGTH = 0x3890*/
  RAM_NO_INIT (rwx) :  ORIGIN = 0x20007F60, LENGTH = 0xA0
}
