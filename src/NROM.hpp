#ifndef NROM_H
#define NROM_H

//iNES mapper #0

#include "Mapper.hpp"


class NROM : public Mapper {
    public:
        NROM(uint8_t P,  uint8_t C) : Mapper(P, C) {}
        uint16_t CPU_ACCESS(uint16_t ADDR);
        uint16_t PPU_ACCESS(uint16_t ADDR);
};

#endif