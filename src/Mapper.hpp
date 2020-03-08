#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <fstream>


class Mapper {
    public:
        Mapper() {}
        Mapper(uint16_t P, uint16_t C): PRG_BANKS(P), CHR_BANKS(C) {}
        virtual uint32_t CPU_READ(uint16_t ADDR) = 0;
        virtual uint16_t PPU_READ(uint16_t ADDR) = 0;
        virtual void CPU_WRITE(uint16_t ADDR, uint8_t VAL) = 0;
        virtual void PPU_WRITE(uint16_t ADDR) = 0;
    protected:
        uint8_t PRG_BANKS;  //In 16KB
        uint8_t CHR_BANKS;  //In 8KB
        uint8_t NT_MIRROR;

};

#endif