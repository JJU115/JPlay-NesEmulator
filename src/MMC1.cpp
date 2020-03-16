#include "MMC1.hpp"

//Handle nametable mirroring
uint32_t MMC1::CPU_READ(uint16_t ADDR) {
   
    if ((ADDR >= 0x6000) && (ADDR <= 0x7FFF))
        return PRG_RAM[ADDR % 0x6000];

    return ((ADDR >= 0x8000) && (ADDR <= 0xBFFF)) ? (0x8000 + (ADDR % 0x8000) + (PBANK1 * 16384)) : (0x8000 + (ADDR % 0xC000) + (PBANK2 * 16384));
}


uint16_t MMC1::PPU_READ(uint16_t ADDR) {
    return ADDR;
}


void MMC1::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {

    if ((ADDR >= 0x6000) && (ADDR <= 0x7FFF)) {
        PRG_RAM[ADDR % 0x6000] = VAL;
        return;
    }

    uint8_t *REG;
    if (ADDR <= 0x9FFF)
        REG = &CONTROL;
    else if (ADDR <= 0xBFFF)
        REG = &CHR_BANK1;
    else if (ADDR <= 0xDFFF)
        REG = &CHR_BANK2;
    else
        REG = &PRG_BANK;

    if (VAL >= 0x80) {
        LOAD = 0x10;
    } else if ((LOAD & 0x01) == 1) { //Shift reg is full if a 1 is in position 0 
        *REG = ((LOAD >> 1) | ((VAL & 0x01) << 4));
        if (REG == &PRG_BANK) {
            switch ((CONTROL & 0x0C) >> 2) {
                case 0:
                case 1:
                    PBANK1 = ((PRG_BANK >> 1) & 0x07);
                    PBANK2 = -1;
                    break;
                case 2:
                    PBANK1 = 0;
                    PBANK2 = (PRG_BANK & 0x0F);
                    break;
                case 3:
                    PBANK1 = (PRG_BANK & 0x0F);
                    PBANK2 = (PRG_BANKS - 1);
                    break;
            }
        }

        LOAD = 0x10;
    } else {
        LOAD = ((LOAD >> 1) | ((VAL & 0x01) << 4));
    }
}


void MMC1::PPU_WRITE(uint16_t ADDR) {

}