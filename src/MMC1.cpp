#include "MMC1.hpp"


enum PRGModes {Fixed32M0 = 0, Fixed32M1 = 1, SwitchLast = 2, SwitchFirst = 3};

//Handle nametable mirroring
uint32_t MMC1::CPU_READ(uint16_t ADDR) {
   
    if ((ADDR >= 0x6000) && (ADDR <= 0x7FFF))
        return PRG_RAM[ADDR % 0x6000];

    if ((CONTROL & 0x0C) > 4) {//2 or 3, that is in 16 KB + 16 KB mode
        if (ADDR > 0x7FFF && ADDR < 0xC000)
            return PRG_BANK_SIZE * (PBANK1 % PRG_BANKS) + (ADDR % 0x8000);
        else
            return PRG_BANK_SIZE * (PBANK2 % PRG_BANKS) + (ADDR % 0xC000);

    } else { //Mapper is in 32 KB PRG mode
        return PRG_BANK_SIZE * (PBANK1 % PRG_BANKS) + (ADDR % 0x8000);
    }
 }


//There are four mirroring possibilities here: vertical, horizontal, one (low), one (high) 
//Should really just use an enum
//Need to implement proper selection based on CHR bank positions
uint32_t MMC1::PPU_READ(uint16_t ADDR, bool NT) {
    if (NT)
        return SelectNameTable(ADDR, static_cast<MirrorMode>(CONTROL & 0x03));

    if (CONTROL & 0x10)  //Two switchable 4 KB banks
        return (ADDR < 0x1000) ? CHR_BANK_SIZE * (CHR_BANK1 % (2 * CHR_BANKS)) + ADDR : CHR_BANK_SIZE * (CHR_BANK2 % (2 * CHR_BANKS)) + (ADDR % 0x1000);
    else  //One switchable 8 KB bank
        return  CHR_BANK_SIZE * ((CHR_BANK1 & 0x1E) % (2 * CHR_BANKS)) + ADDR;
    
}


void MMC1::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {

    if ((ADDR >= 0x6000) && (ADDR <= 0x7FFF)) {
        PRG_RAM[ADDR % 0x6000] = VAL;
        return;
    }

    if (ADDR < 0x8000)
        return;

    LOAD = VAL;
    if (LOAD > 0x7F) {//Bit 7 set, reset shift register
        SHIFT = 0x10;
        CONTROL |= 0x0C;
    } else { 
        //Otherwise, if shift is full, time to write shift contents to an internal register
        if (SHIFT & 0x01) {
            SHIFT >>= 1;
            SHIFT |= ((LOAD & 0x01) << 4);

            //Address of fith write determines which reg to write to, only bits 13, 14 matter here
            switch (ADDR & 0x6000) {
                case 0:
                    CONTROL = SHIFT;
                    switch ((CONTROL & 0x0C) >> 2) {
                        case Fixed32M0: //Switch 32 KB at $8000, ignore low bit of bank number
                        case Fixed32M1:
                            PBANK1 = PBANK2 = (PRG_BANK & 0x0E); //CH
                            break;
                        case SwitchLast: //Fix 1st bank at $8000, switch last at $C000, both 16 KB
                            PBANK1 = 0;
                            PBANK2 = PRG_BANK & 0x0F;
                            break;
                        case SwitchFirst: //Fix last bank at $C000, switch 1st at $8000, 16 KB each
                            PBANK1 = PRG_BANK & 0x0F;
                            PBANK2 = NUM_BANKS - 1;
                            break;
                    }
                    break;
                case 0x2000:
                    CHR_BANK1 = SHIFT;
                    break;
                case 0x4000:
                    CHR_BANK2 = SHIFT;
                    break;
                case 0x6000:
                    PRG_BANK = SHIFT; //change prg bank
                    switch ((CONTROL & 0x0C) >> 2) {
                        case Fixed32M0: //Switch 32 KB at $8000, ignore low bit of bank number
                        case Fixed32M1:
                            PBANK1 = PBANK2 = (PRG_BANK & 0x0E); //CH
                            break;
                        case SwitchLast: //Fix 1st bank at $8000, switch last at $C000, both 16 KB
                            PBANK1 = 0;
                            PBANK2 = PRG_BANK & 0x0F;
                            break;
                        case SwitchFirst: //Fix last bank at $C000, switch 1st at $8000, 16 KB each
                            PBANK1 = PRG_BANK & 0x0F;
                            PBANK2 = NUM_BANKS - 1;
                            break;
                    }
                    break;
            }
            SHIFT = 0x10;

        } else {
            SHIFT >>= 1;
            SHIFT |= ((LOAD & 0x01) << 4);
        }
    }
}


void MMC1::PPU_WRITE(uint16_t ADDR) {

}