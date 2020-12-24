#ifndef H_UXROM
#define H_UXROM

#include "Mapper.hpp"

//iNES Mapper #2: UxROM

class UxROM : public Mapper {
    public:
        UxROM(uint8_t P, bool M) : Mapper(P, 1), LOW_PRG_BANK(0) {NT_MIRROR = (M) ? Vertical : Horizontal;}
        uint32_t CPU_READ(uint16_t ADDR);
        uint32_t PPU_READ(uint16_t ADDR);
        void CPU_WRITE(uint16_t ADDR, uint8_t VAL);
        void PPU_WRITE(uint16_t ADDR);

    private:
        uint8_t LOW_PRG_BANK;

};




#endif