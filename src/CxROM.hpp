#ifndef CXROM_H
#define CXROM_H

#include "Mapper.hpp"


//iNES Mapper #3: CNROM and similar boards


class CxROM : public Mapper {
    public:
        CxROM(uint8_t P, uint8_t C, bool M) : Mapper(P, C), CHR_BANK(0) {NT_MIRROR = (M) ? Vertical : Horizontal;}
        uint32_t CPU_READ(uint16_t ADDR);
        uint32_t PPU_READ(uint16_t ADDR, bool NT);
        void CPU_WRITE(uint16_t ADDR, uint8_t VAL);
        void PPU_WRITE(uint16_t ADDR);

    private:
        uint8_t CHR_BANK;
};



#endif