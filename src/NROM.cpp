#include "NROM.hpp"


uint32_t NROM::CPU_READ(uint16_t ADDR) {
   
    if (PRG_BANKS == 2)
        return ADDR;
    else
        return (ADDR & 0xBFFF);
        
}


uint16_t NROM::PPU_READ(uint16_t ADDR, bool NT) {

    if (NT) {
        if (NT_MIRROR) { //equals 1 -> vertical
            if (ADDR >= 0x2800)
                return (ADDR - 0x0800);
        } else { //equals 0 -> horizontal
            if ((ADDR >= 0x2400 && ADDR <= 0x27FF) || (ADDR >= 0x2C00))
                return (ADDR - 0x0400);
        }
    }
    
    return ADDR;
}