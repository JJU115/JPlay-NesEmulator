#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <fstream>
#include "Mapper.hpp"


class Cartridge {
    public:
        void LOAD(char *FILE);
        uint8_t CPU_ACCESS(uint16_t ADDR, uint8_t VAL=0, bool R=true);
        uint16_t PPU_ACCESS(uint16_t ADDR, bool NT_M=false);

    private:
        std::ifstream CPU_LINE1;
        std::ifstream PPU_LINE1;

        char *C_BUF;
        char *P_BUF;
        Mapper *M;
};

#endif