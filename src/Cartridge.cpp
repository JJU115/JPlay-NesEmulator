#include "Cartridge.hpp"
#include "NROM.hpp"
#include "MMC1.hpp"
#include <fstream>
#include <iostream>


//For now, only iNES format will be supported, NES 2.0 will come later after initial dev wraps
/*Cartridge& Cartridge::operator=(const Cartridge& C) {
    ROM.swap(C.ROM);
}*/


void Cartridge::LOAD(char *FILE) {
    //Error handle if needed
    CPU_LINE1.open(FILE, std::ios::in | std::ios::binary);
    PPU_LINE1.open(FILE, std::ios::in | std::ios::binary);

    if (CPU_LINE1.is_open())
        std::cout << "ROM loaded\n";

    char *H = new char[16];
    C_BUF = new char[1];
    CPU_LINE1.read(H, 16);
    
    if ((H[0] == 'N') && (H[1] == 'E') && (H[2] == 'S') && (H[3] == 0x1A))
        std::cout << "NES file loaded\n";

    if ((H[6] & 0x04) != 0) {
        CPU_LINE1.seekg(528);
        PPU_LINE1.seekg(544 + (H[4] * 16384));
    } else {
        PPU_LINE1.seekg(16 + (H[4] * 16384));
    }


    std::cout << "Header read\n";
    uint8_t MAP_NUM = (((H[6] & 0xF0) >> 4) | (H[7] & 0xF0));
    
    //Load the mapper
    //Need some sort of dictionary to store (mapper#, mapper class) pairs, for now just if statements
    if (MAP_NUM == 0) 
        M = new NROM(H[4], H[5], (H[6] & 0x01));
    else if (MAP_NUM == 1)
        M = new MMC1(H[4], H[5]);

}


//Have write return where the file pointer should point to next, in 8Kb from start of first bank?
uint8_t Cartridge::CPU_ACCESS(uint16_t ADDR, uint8_t VAL, bool R) {
    
    if (R) {
        if (ADDR < 0x8000)
            return M->CPU_READ(ADDR);
        
        uint32_t A = M->CPU_READ(ADDR);
       // std::cout << std::hex << A;
       //std::cout << std::hex << CPU_LINE1.tellg() << '\n';
        CPU_LINE1.seekg(A - 0x8000, std::ios::cur);
       //std::cout << std::hex << CPU_LINE1.tellg() << '\n';
        CPU_LINE1.read(C_BUF, 1);
        CPU_LINE1.seekg(16, std::ios::beg); //Assuming no trainer, most nes files don't have but should still handle properly anyway
       // std::cout << std::hex << CPU_LINE1.tellg() << '\n';
    
        return *C_BUF;
        
    } else {
        M->CPU_WRITE(ADDR, VAL);
        return 0;
    }
}



//PPU_LINE position sits at start of CHR ROM
uint16_t Cartridge::PPU_ACCESS(uint16_t ADDR, bool NT_M) {

    if (NT_M)
        return M->PPU_READ(ADDR);
        
    uint16_t A = M->PPU_READ(ADDR);

    return 0;
}


