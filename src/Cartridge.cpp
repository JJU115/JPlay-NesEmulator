#include "Cartridge.hpp"
#include "NROM.hpp"
#include <fstream>
#include <iostream>


//For now, only iNES format will be supported, NES 2.0 will come later after initial dev wraps
/*Cartridge& Cartridge::operator=(const Cartridge& C) {
    ROM.swap(C.ROM);
}*/


void Cartridge::LOAD(char *FILE) {
    //Error handle if needed
    CPU_LINE.open(FILE, std::ios::in | std::ios::binary);
    PPU_LINE.open(FILE, std::ios::in | std::ios::binary);

    if (CPU_LINE.is_open())
        std::cout << "ROM loaded\n";

    char *H = new char[16];
    CPU_LINE.read(H, 16);
    
    if ((H[0] == 'N' && H[1] == 'E' && H[2] == 'S') && (H[3] == 0x1A))
        std::cout << "NES file loaded\n";

    if ((H[6] & 0x04) != 0) {
        CPU_LINE.seekg(528);
        PPU_LINE.seekg(528 + H[4]*16384);
    } else 
        PPU_LINE.seekg(16 + H[4]*16384); 
    
    uint8_t MAP_NUM = ((H[6] >> 4) | (H[7] & 0xF0));

    //Load the mapper
    //Need some sort of dictionary to store (mapper#, mapper class) pairs, for now just if statements
    if (MAP_NUM == 0) 
        M = new NROM(H[4], H[5]);

  /*  CPU_LINE.seekg(16 * 1023, std::ios::cur);

    CPU_LINE.read(H, 16);
    for (int i=0; i<16; i++)
        std::cout << std::hex << int16_t(H[i]) << std::endl;*/

}


//CPU_LINE position sits at start of PRG ROM - address $8000 from CPU's perspective 
//For now, no battery backed RAM or SRAM, just PRG banks
uint8_t Cartridge::CPU_READ(uint16_t ADDR) {
    uint16_t A = M->CPU_ACCESS(ADDR);
    if (A >= 0x8000)
        CPU_LINE.seekg(A - 0x8000, std::ios::cur);

    CPU_LINE.read(C_BUF, 1);
    CPU_LINE.seekg(0x7FFF - A, std::ios::cur);
    
    return *C_BUF;
}



//PPU_LINE position sits at start of CHR ROM
uint8_t Cartridge::PPU_READ(uint16_t ADDR) {
    return 0;
}


