#ifndef NROM_H
#define NROM_H

//iNES mapper #000

#include "Mapper.hpp"


class NROM : public Mapper {
    public:
        NROM(uint8_t P,  uint8_t C, uint8_t M) : Mapper(P, C) { NT_MIRROR = M; }
        uint32_t CPU_READ(uint16_t ADDR);
        uint16_t PPU_READ(uint16_t ADDR);
        void CPU_WRITE(uint16_t ADDR, uint8_t VAL) {}
        void PPU_WRITE(uint16_t ADDR) {}
};

#endif