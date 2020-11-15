#ifndef AXROM_H
#define AXROM_H


#include "Mapper.hpp"



class AxROM : public Mapper {
    public:
        AxROM(uint8_t P, uint8_t C) : Mapper(P, C), PrgBank(0), SingleScreen(SingleLower) {}
        uint32_t CPU_READ(uint16_t ADDR);
        uint32_t PPU_READ(uint16_t ADDR, bool NT);
        void CPU_WRITE(uint16_t ADDR, uint8_t VAL);
        void PPU_WRITE(uint16_t ADDR);

    private:
        uint8_t PrgBank;
        MirrorMode SingleScreen;


};

#endif