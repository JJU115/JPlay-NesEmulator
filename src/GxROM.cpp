#include "GxROM.hpp"


inline uint32_t GxROM::CPU_READ(uint16_t ADDR) {

    return (PrgBank * PRG_BANK_SIZE * 2) + (ADDR % 0x8000);
}


inline uint32_t GxROM::PPU_READ(uint16_t ADDR, bool NT) {

    return (NT) ? SelectNameTable(ADDR, NT_MIRROR) : (ChrBank * CHR_BANK_SIZE * 2) + ADDR;
}



inline void GxROM::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {

    PrgBank = ((VAL & 0x30) >> 4);
    ChrBank = (VAL & 0x03);
}


void GxROM::PPU_WRITE(uint16_t ADDR) { 

}