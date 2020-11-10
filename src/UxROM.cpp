#include "UxROM.hpp"




inline uint32_t UxROM::CPU_READ(uint16_t ADDR) {

    return (ADDR < 0xC000) ? (LOW_PRG_BANK * PRG_BANK_SIZE + (ADDR % 0x8000)) : ((PRG_BANKS - 1) * PRG_BANK_SIZE + (ADDR % 0xC000));
}


inline uint32_t UxROM::PPU_READ(uint16_t ADDR, bool NT) {

    return (NT) ? SelectNameTable(ADDR, NT_MIRROR) : ADDR;
}


inline void UxROM::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {

    LOW_PRG_BANK = VAL;
}


void UxROM::PPU_WRITE(uint16_t ADDR) {

}