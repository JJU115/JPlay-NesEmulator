#include "NROM.hpp"


uint32_t NROM::CPU_READ(uint16_t ADDR) {
   
    if (PRG_BANKS == 2)
        return ADDR;
    else
        return (ADDR & 0xBFFF);
        
}


uint16_t NROM::PPU_READ(uint16_t ADDR) {
    return ADDR;
}