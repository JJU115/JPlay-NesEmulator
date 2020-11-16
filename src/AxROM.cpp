#include "AxROM.hpp"
#include <iostream>


inline uint32_t AxROM::CPU_READ(uint16_t ADDR) {
    return PrgBank * PRG_BANK_SIZE * 2 + (ADDR % 0x8000);
}





inline uint32_t AxROM::PPU_READ(uint16_t ADDR, bool NT) {
    if (NT) 
        return SelectNameTable(ADDR, SingleScreen);
       
    return ADDR;
}






inline void AxROM::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {
    if (ADDR > 0x7FFF) {
        PrgBank = VAL & 0x07;
        SingleScreen = static_cast<MirrorMode>((VAL & 0x10) >> 4);
    }
}





void AxROM::PPU_WRITE(uint16_t ADDR) {

}