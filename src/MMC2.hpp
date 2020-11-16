#ifndef MMC2_H
#define MMC2_H


#include "Mapper.hpp"


//iNES Mapper #9 - MMC2

class MMC2 : public Mapper {
    public:
        MMC2(uint8_t P, uint8_t C) : Mapper(P, C), PrgBank(0), ChrBankLowFD(0), ChrBankLowFE(0), ChrBankHighFD(0), ChrBankHighFE(0),
        LatchZero(0xFD), LatchOne(0xFD), Mirroring(Vertical) {}

        uint32_t CPU_READ(uint16_t ADDR);
        uint32_t PPU_READ(uint16_t ADDR, bool NT);
        void CPU_WRITE(uint16_t ADDR, uint8_t VAL);
        void PPU_WRITE(uint16_t ADDR);

    private:
    //Prg bank is 8KB, Chr ROM is two sets of two 4KB banks
        uint8_t PrgBank;
        uint8_t ChrBankLowFD, ChrBankLowFE; //Top 8 bits are for the switchout bank when the latch is set to $FE, bottom 8 are when latch is $FD
        uint8_t ChrBankHighFD, ChrBankHighFE;
        uint32_t Read;
        uint8_t LatchZero, LatchOne;
        MirrorMode Mirroring;
};




#endif