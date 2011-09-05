/*--------------------------------------------------------------------

  Title     : Header file for eeprom_rw.c  
  Filename	: eeprom_rw.h 
    
----------------------------------------------------------------------*/

#ifndef EEPROM_RW_H
#define EEPROM_RW_H

#include "posix_types.h"

/*--- Eeprom function prototypes ---*/

void WriteWord(uint16_t address, uint16_t data);
uint16_t ReadWord(uint16_t address);

#endif

/*--- End of File ---*/
