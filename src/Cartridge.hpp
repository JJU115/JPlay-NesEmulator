#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <fstream>


class Cartridge {
    public:
       // Cartridge& operator=(const Cartridge& C);
        void LOAD(std::ifstream& NES);
        void FETCH_PRGROM(uint16_t ADDR);
        void FETCH_CHRROM(uint16_t ADDR);

    private:
        std::ifstream ROM;
        std::streampos PRGPOS;
        std::streampos CHRPOS;

};

#endif