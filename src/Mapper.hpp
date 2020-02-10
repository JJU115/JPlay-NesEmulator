#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <fstream>


class Mapper {
    public:
        Mapper() {}
        Mapper(uint16_t P, uint16_t C): PRG_BANKS(P), CHR_BANKS(C) {}
        virtual uint16_t CPU_ACCESS(uint16_t ADDR) = 0;
        virtual uint16_t PPU_ACCESS(uint16_t ADDR) = 0;
        uint8_t PRG_BANKS;
    protected:
        //uint8_t PRG_BANKS;  //In 16KB
        uint8_t CHR_BANKS;  //In 8KB

};

#endif