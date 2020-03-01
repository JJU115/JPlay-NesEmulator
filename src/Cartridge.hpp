#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <fstream>
#include "Mapper.hpp"


class Cartridge {
    public:
        void LOAD(char *FILE);
        uint8_t CPU_ACCESS(uint16_t ADDR, uint8_t VAL=0, bool R=true);
        uint8_t PPU_READ(uint16_t ADDR);

    private:
        std::ifstream CPU_LINE1;

        char *C_BUF;
        char *P_BUF;
        Mapper *M;
};

#endif