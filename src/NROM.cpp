#include "NROM.hpp"


uint16_t NROM::CPU_ACCESS(uint16_t ADDR) {

    if (PRG_BANKS == 2)
        return ADDR;
    else
        return (ADDR & 0xBFFF);
        
}


uint16_t NROM::PPU_ACCESS(uint16_t ADDR) {
    return ADDR;
}