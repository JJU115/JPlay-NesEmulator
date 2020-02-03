#include "Cartridge.hpp"
#include <fstream>
#include <iostream>


//For now, only iNES format will be supported, NES 2.0 will come later after initial dev wraps
/*Cartridge& Cartridge::operator=(const Cartridge& C) {
    ROM.swap(C.ROM);
}*/


void Cartridge::LOAD(std::ifstream& NES) {
    //Error handle if needed
    ROM.swap(NES);
    if (ROM.is_open())
        std::cout << "ROM loaded\n";

    char *H = new char[16];
    ROM.read(H, 16);

    if ((H[0] == 'N' && H[1] == 'E' && H[2] == 'S') && (H[3] == 0x1A))
        std::cout << "NES file loaded\n";

    //Flag 6
    c = 7;
    p = 8;

}


/*
void Cartridge::FETCH_PRGROM(uint16_t ADDR) {

}




void Cartridge::FETCH_CHRROM(uint16_t ADDR) {

}*/


