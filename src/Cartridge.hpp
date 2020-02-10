#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <fstream>
#include "Mapper.hpp"


class Cartridge {
    public:
       // Cartridge& operator=(const Cartridge& C);
        void LOAD(char *FILE);
        uint8_t CPU_READ(uint16_t ADDR);
        uint8_t PPU_READ(uint16_t ADDR);

    private:
        std::ifstream CPU_LINE;
        std::ifstream PPU_LINE;
        std::streampos CPU_POS;
        std::streampos PPU_POS;

        char *C_BUF;
        char *P_BUF;
        Mapper *M;
};

#endif