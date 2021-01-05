#include "NROM.hpp"


uint32_t NROM::CPU_READ(uint16_t ADDR) {
   
    return (PRG_BANKS == 2) ? (ADDR - 0x8000) : ((ADDR & 0xBFFF) - 0x8000);     
}

//For nametable reads, want to put every address within a 0x2000 - 0x27FF range
uint32_t NROM::PPU_READ(uint16_t ADDR) {
    return ADDR;
}