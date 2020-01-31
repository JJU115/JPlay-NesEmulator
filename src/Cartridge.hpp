#ifndef CARTRIDGE_H
#define CARTRIDGE_H


#include <cstdint>


class Cartridge {
    public:
        void FETCH_PRGROM(uint16_t ADDR);
        void FETCH_CHRROM(uint16_t ADDR);


    private:

};

#endif