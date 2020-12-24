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


//For now, only iNES format will be supported

bool Cartridge::LOAD(char *FILE) {
    //Error handle if needed
    CPU_LINE1.open(FILE, std::ios::in | std::ios::binary);

    if (CPU_LINE1.is_open())
        std::cout << "ROM loaded\n";
    else
        return false; 

    char *H = new char[16];
    C_BUF = new char[1];
    CPU_LINE1.read(H, 16);
    
    if ((H[0] == 'N') && (H[1] == 'E') && (H[2] == 'S') && (H[3] == 0x1A))
        std::cout << "NES file loaded\n";
    else
        return false;

    if ((H[6] & 0x04) != 0) { //Trainer present
        CPU_STARTPOS = 544;
        PPU_STARTPOS = 544 + (H[4] * 16384);
        CPU_LINE1.seekg(528);
    } else {
        CPU_STARTPOS = 16;
        PPU_STARTPOS = 16 + (H[4] * 16384);
    }


    uint8_t MAP_NUM = ((H[6] & 0xF0) >> 4 | (H[7] & 0xF0)); //Usage of byte 7 only needed for mapper 66

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
    switch (MAP_NUM) {
        case 0:
            M = new NROM(H[4], H[5], (H[6] & 1));
            break;
        case 1:
            M = new MMC1(H[4], H[5]);
            break;
        case 2:
            M = new UxROM(H[4], (H[6] & 1));
            break;
        case 3:
            M = new CxROM(H[4], H[5], (H[6] & 1));
            break;
        case 4:
            M = new MMC3(H[4], H[5]);
            break;
        case 7:
            M = new AxROM(H[4], H[5]);
            break;
        case 9:
            M = new MMC2(H[4], H[5]);
            break;
        case 66:
            M = new GxROM(H[4], H[5], (H[6] & 1));
            break;
        default:
            CPU_LINE1.close();
            delete DATA_BUF;
            return false;
    }


    CPU_LINE1.close();
    FireIrq = &(M->Irq);
    delete DATA_BUF;
    return true;
}


uint8_t Cartridge::CPU_ACCESS(uint16_t ADDR, uint8_t VAL, bool R) {
    if (R) {

        if (ADDR < 0x8000)
            return M->CPU_READ(ADDR);

        uint32_t a = M->CPU_READ(ADDR);
        if (a >= PRG_ROM.size())
            std::cout << "PRG OB Read " << ADDR << " --- " << M->CPU_READ(ADDR) << '\n';

        return PRG_ROM[a];
        
    } else {
        M->CPU_WRITE(ADDR, VAL);
        return 0;
    }
}


uint16_t Cartridge::PPU_ACCESS(uint16_t ADDR, uint8_t VAL, bool R, bool NT_M) {
    if (NT_M)
        return M->SelectNameTable(ADDR);

    if (R) {

        if (ChrRam)
            return CHR_ROM[ADDR];
        
        uint32_t p = M->PPU_READ(ADDR);
        if (p >= CHR_ROM.size()) 
            std::cout << "CHR OB Read " << ADDR << " --- " << M->PPU_READ(ADDR) << '\n';

        return CHR_ROM[p]; 

    } else if (ChrRam) {
        //Write. Only if the board contains CHR RAM 
        CHR_ROM[ADDR] = VAL;
    }

    return 0;
}


//If an Irq is pending, FireIrq is true, then keep it true until acknowledged by CPU
void Cartridge::Scanline() {
    M->Scanline();
}