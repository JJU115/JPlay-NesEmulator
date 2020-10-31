#include "CxROM.hpp"
#include <iostream>


inline uint32_t CxROM::CPU_READ(uint16_t ADDR) {

    if (PRG_BANKS == 2)
        return (ADDR % 0x8000);

    return (ADDR < 0xC000) ? (ADDR % 0x8000) : (ADDR % 0xC000);
}


inline uint16_t CxROM::PPU_READ(uint16_t ADDR, bool NT) {

    return (NT) ? SelectNameTable(ADDR, NT_MIRROR) : CHR_BANK * 2 * CHR_BANK_SIZE + ADDR;

}


inline void CxROM::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {
    
    CHR_BANK = VAL & 0x03;
}


void CxROM::PPU_WRITE(uint16_t ADDR) {

}