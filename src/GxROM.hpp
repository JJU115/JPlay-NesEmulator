#ifndef GXROM_H
#define GXROM_H

#include "Mapper.hpp"


//iNES mapper #66 - GxROM


class GxROM : public Mapper {
    public:
        GxROM(uint8_t P, uint8_t C, uint8_t M) : Mapper(P, C), PrgBank(0), ChrBank(0) { NT_MIRROR = (M == 0) ? Vertical : Horizontal; }
        uint32_t CPU_READ(uint16_t ADDR);
        uint32_t PPU_READ(uint16_t ADDR, bool NT);
        void CPU_WRITE(uint16_t ADDR, uint8_t VAL);
        void PPU_WRITE(uint16_t ADDR);

    private:
        uint8_t PrgBank;
        uint8_t ChrBank;
};


#endif