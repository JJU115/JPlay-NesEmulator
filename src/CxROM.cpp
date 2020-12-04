#include "CxROM.hpp"


inline uint32_t CxROM::CPU_READ(uint16_t ADDR) {

    if (PRG_BANKS == 2)
        return (ADDR % 0x8000);

    return (ADDR < 0xC000) ? (ADDR % 0x8000) : (ADDR % 0xC000);
}


inline uint32_t CxROM::PPU_READ(uint16_t ADDR, bool NT) {

    return (NT) ? SelectNameTable(ADDR, NT_MIRROR) : CHR_BANK * 2 * CHR_BANK_SIZE + ADDR;

}


inline void CxROM::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {
    //CNROM only uses the bottom 2 bits capping CHR at 32KB, games using this board with >4 CHR banks should use bottom 4 or more bits
    CHR_BANK = VAL & 0x03;
}


void CxROM::PPU_WRITE(uint16_t ADDR) {

}