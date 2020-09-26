#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <fstream>
#include <vector>
#include "Mapper.hpp"


class Cartridge {
    public:
        void LOAD(char *FILE);
        uint8_t CPU_ACCESS(uint16_t ADDR, uint8_t VAL=0, bool R=true);
        uint16_t PPU_ACCESS(uint16_t ADDR, uint8_t VAL, bool R, bool NT_M=false);

    private:
        std::ifstream CPU_LINE1;

        uint16_t CPU_STARTPOS;
        uint32_t PPU_STARTPOS;

        std::vector<uint8_t> PRG_ROM;
        std::vector<uint8_t> CHR_ROM;

        bool ChrRam;
        char *C_BUF;
        char *P_BUF;
        Mapper *M;
};

#endif