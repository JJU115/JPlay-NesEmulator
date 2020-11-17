#include "Cartridge.hpp"
#include "NROM.hpp"
#include "MMC1.hpp"
#include "UxROM.hpp"
#include "CxROM.hpp"
#include "MMC3.hpp"
#include "AxROM.hpp"
#include "MMC2.hpp"
#include "GxROM.hpp"
#include <fstream>
#include <iostream>


//For now, only iNES format will be supported, NES 2.0 will come later after initial dev wraps

void Cartridge::LOAD(char *FILE) {
    //Error handle if needed
    CPU_LINE1.open(FILE, std::ios::in | std::ios::binary);
    //PPU_LINE1.open(FILE, std::ios::in | std::ios::binary);

    if (CPU_LINE1.is_open())
        std::cout << "ROM loaded\n";

    char *H = new char[16];
    C_BUF = new char[1];
    CPU_LINE1.read(H, 16);
    
    if ((H[0] == 'N') && (H[1] == 'E') && (H[2] == 'S') && (H[3] == 0x1A))
        std::cout << "NES file loaded\n";

    if ((H[6] & 0x04) != 0) { //Trainer present
        CPU_STARTPOS = 544;
        PPU_STARTPOS = 544 + (H[4] * 16384);
        CPU_LINE1.seekg(528);
        //PPU_LINE1.seekg(PPU_STARTPOS);
    } else {
        CPU_STARTPOS = 16;
        PPU_STARTPOS = 16 + (H[4] * 16384);
        //PPU_LINE1.seekg(PPU_STARTPOS);
    }


    std::cout << "Header read\n";
    uint8_t MAP_NUM = ((H[6] & 0xF0) >> 4) | (H[7] & 0xF0); //Usage of byte 7 only needed for mapper 66

    //Copy PRG and CHR data to internal vectors
    PRG_ROM.resize(16 * 1024 * H[4]);
    CHR_ROM.resize(8 * 1024 * H[5]); //If H[5] is zero, board uses CHR RAM

    uint8_t* P_DATA = PRG_ROM.data();
    uint8_t* C_DATA = CHR_ROM.data();
    char* DATA_BUF = (H[5] > 2*H[4]) ? new char[8 * 1024 * H[5]] : new char[16 * 1024 * H[4]]; //Should be the greater size of chr vs prg
    
    CPU_LINE1.read(DATA_BUF, 16 * 1024 * H[4]);
    memcpy(P_DATA, DATA_BUF, 16 * 1024 * H[4]);

    ChrRam = !H[5];
    if (!ChrRam) {
        CPU_LINE1.seekg(PPU_STARTPOS, std::ios::beg);
        CPU_LINE1.read(DATA_BUF, 8 * 1024 * H[5]);
        memcpy(C_DATA, DATA_BUF, 8 * 1024 * H[5]);
    } else {
        CHR_ROM.resize(8 * 1024);
    }
    
    //Load the mapper
    //Need some sort of dictionary to store (mapper#, mapper class) pairs, for now just if statements
    if (MAP_NUM == 0) 
        M = new NROM(H[4], H[5], (H[6] & 0x01)); //0 = horizontal mirror, 1 = vertical mirror
    else if (MAP_NUM == 1)
        M = new MMC1(H[4], H[5]);
    else if (MAP_NUM == 2)
        M = new UxROM(H[4], H[6] & 0x01);
    else if (MAP_NUM == 3)
        M = new CxROM(H[4], H[5], H[6] & 0x01);
    else if (MAP_NUM == 4)
        M = new MMC3(H[4], H[5]);
    else if (MAP_NUM == 7)
        M = new AxROM(H[4], H[5]);
    else if (MAP_NUM == 9)
        M = new MMC2(H[4], H[5]);
    else if (MAP_NUM == 66)
        M = new GxROM(H[4], H[5], (H[6] & 0x01));


    CPU_LINE1.close();
    FireIrq = false;
    delete DATA_BUF;
}


uint8_t Cartridge::CPU_ACCESS(uint16_t ADDR, uint8_t VAL, bool R) {
    if (R) {

        if (ADDR < 0x8000)
            return M->CPU_READ(ADDR);

        if (M->CPU_READ(ADDR) >= PRG_ROM.size())
            std::cout << "PRG OB Read " << ADDR << " --- " << M->CPU_READ(ADDR) << '\n';
        //std::cout << "PRG Read: " << ADDR << '\n';
        return PRG_ROM[M->CPU_READ(ADDR)];
        
    } else {
        M->CPU_WRITE(ADDR, VAL); //possible cases here differ by mapper, this will have to change when implementing more of them 
        return 0;
    }
}


uint16_t Cartridge::PPU_ACCESS(uint16_t ADDR, uint8_t VAL, bool R, bool NT_M) {
    if (NT_M)
        return M->PPU_READ(ADDR, NT_M);

    if (R) {

        if (ChrRam)
            return CHR_ROM[ADDR];
        
        if (M->PPU_READ(ADDR, false) >= CHR_ROM.size()) 
            std::cout << "CHR OB Read " << ADDR << " --- " << M->PPU_READ(ADDR, false) << '\n';

        return CHR_ROM[M->PPU_READ(ADDR, false)]; 

    } else if (ChrRam) {
        //Write. Only if the board contains CHR RAM 
        CHR_ROM[ADDR] = VAL;
    }

    return 0;
}


//If an Irq is pending, FireIrq is true, then keep it true until acknowledged by CPU
bool Cartridge::Scanline() {
    if (FireIrq)
        M->Scanline();
    else if (M->Scanline()) { 
        FireIrq = true;
        M->Irq = false;
    } 
}