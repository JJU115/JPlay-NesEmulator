#ifndef MMC1_H
#define MMC1_H

//iNES mapper #001 - Still unclear on register state at power-up/reset 

#include "Mapper.hpp"
#include <vector>


class MMC1 : public Mapper {
    public:
        MMC1(uint8_t P,  uint8_t C) : Mapper(P, C), LOAD(0x80), SHIFT(0x10), CONTROL(0x1C), PRG_BANK(0), CHR_BANK1(0), CHR_BANK2(0),
        PBANK1(0), PBANK2(P-1), NUM_BANKS(P) { 
            PRG_RAM.resize(8 * 1024); //Enabled by default for now
        } //Start up values not entirely clear yet
        uint32_t CPU_READ(uint16_t ADDR);
        uint16_t PPU_READ(uint16_t ADDR, bool NT);
        void CPU_WRITE(uint16_t ADDR, uint8_t VAL);
        void PPU_WRITE(uint16_t ADDR);
    private:
        uint8_t LOAD, CONTROL, PRG_BANK, CHR_BANK1, CHR_BANK2, SHIFT; //Registers
        uint8_t PBANK1; //PRG bank locations
        uint8_t PBANK2;
        uint8_t NUM_BANKS;
        std::vector<uint8_t> PRG_RAM; //Need NES 2.0 header to determine this

};




#endif