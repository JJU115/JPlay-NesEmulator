#include "MMC2.hpp"
#include <iostream>


uint32_t MMC2::CPU_READ(uint16_t ADDR) {

    if (ADDR < 0xA000)
        return PrgBank * (PRG_BANK_SIZE / 2) + (ADDR % 0x8000);

    if (ADDR < 0xC000)
        return (CHR_BANKS - 3) * (PRG_BANK_SIZE / 2) + (ADDR % 0xA000);
    else if (ADDR < 0xE000)
        return (CHR_BANKS - 2) * (PRG_BANK_SIZE / 2) + (ADDR % 0xC000);
    else
        return (CHR_BANKS - 1) * (PRG_BANK_SIZE / 2) + (ADDR % 0xE000);
}


uint32_t MMC2::PPU_READ(uint16_t ADDR, bool NT) {

    if (NT)
        return SelectNameTable(ADDR, Mirroring);

    if (ADDR < 0x1000)
        Read = (LatchZero == 0xFD) ? (ChrBankLowFD * CHR_BANK_SIZE) + ADDR : (ChrBankLowFE * CHR_BANK_SIZE) + ADDR;
    else
        Read = (LatchOne == 0xFD) ? (ChrBankHighFD * CHR_BANK_SIZE) + (ADDR % 0x1000) : (ChrBankHighFE * CHR_BANK_SIZE) + (ADDR % 0x1000);

    if (ADDR == 0x0FD8)
        LatchZero = 0xFD;
    else if (ADDR == 0x0FE8)
        LatchZero = 0xFE;

    if ((ADDR > 0x1FD7) && (ADDR < 0x1FE0))
        LatchOne = 0xFD;
    else if ((ADDR > 0x1FE7) && (ADDR < 0x1FF0))
        LatchOne = 0xFE;

    return Read;
}



void MMC2::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {

    if (ADDR < 0x8000)
        return;

    switch (ADDR & 0xF000) {
        case 0xA000:
            PrgBank = VAL & 0x0F;
            break;
        case 0xB000: //For $FD/0x0000 of low 4KB CHR ROM
            ChrBankLowFD = (VAL & 0x1F);
            break;
        case 0xC000: //For high 4KB of CHR ROM
            ChrBankLowFE = (VAL & 0x1F);
            break;
        case 0xD000:
            ChrBankHighFD = (VAL & 0x1F);
            break;
        case 0xE000:
            ChrBankHighFE = (VAL & 0x1F);
            break;
        case 0xF000:
            Mirroring = (VAL & 0x01) ? Horizontal : Vertical;
            break;
    }
    //std::cout << "Wrote " << int(VAL) << " to " << ADDR << '\n';
}



void MMC2::PPU_WRITE(uint16_t ADDR) {

}