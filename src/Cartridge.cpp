#include "Cartridge.hpp"
#include "NROM.hpp"
#include "MMC1.hpp"
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
    uint8_t MAP_NUM = (((H[6] & 0xF0) >> 4) | (H[7] & 0xF0));

    //Copy PRG and CHR data to internal vectors
    PRG_ROM.resize(16 * 1024 * H[4]);
    CHR_ROM.resize(8 * 1024 * H[5]); //If H[5] is zero, board uses CHR RAM

    uint8_t* P_DATA = PRG_ROM.data();
    uint8_t* C_DATA = CHR_ROM.data();
    char* DATA_BUF = new char[16 * 1024 * H[4]]; //Should be the greater size of chr vs prg
    
    CPU_LINE1.read(DATA_BUF, 16 * 1024 * H[4]);
    memcpy(P_DATA, DATA_BUF, 16 * 1024 * H[4]);

    //Need to properly handle CHR RAM vs CHR ROM, this will do for now
    ChrRam = !H[5];
    if (!ChrRam) {
        CPU_LINE1.seekg(PPU_STARTPOS, std::ios::beg);
        CPU_LINE1.read(DATA_BUF, 8 * 1024 * H[5]);
        memcpy(C_DATA, DATA_BUF, 8 * 1024 * H[5]);
    } else {
        CHR_ROM.resize(1);
    }
    
    //Load the mapper
    //Need some sort of dictionary to store (mapper#, mapper class) pairs, for now just if statements
    if (MAP_NUM == 0) 
        M = new NROM(H[4], H[5], (H[6] & 0x01)); //0 = horizontal mirror, 1 = vertical mirror
    else if (MAP_NUM == 1)
        M = new MMC1(H[4], H[5], H[10]); //Using NES 2.0 flag 10 for PRG RAM detection


    CPU_LINE1.close();
    delete DATA_BUF;
}


uint8_t Cartridge::CPU_ACCESS(uint16_t ADDR, uint8_t VAL, bool R) {
    if (R) {
        if (ADDR < 0x8000) //Work RAM, presence determined by mapper
            return 0;

        if (M->CPU_READ(ADDR) >= PRG_ROM.size())
            std::cout << "OB Read " << ADDR << " --- " << M->CPU_READ(ADDR) << '\n';

        return PRG_ROM[M->CPU_READ(ADDR)]; //Works for NROM but not MMC1
        
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
            return 0;
        else {
            if (M->PPU_READ(ADDR, false) >= CHR_ROM.size()) {
                std::cout << "OB Read " << ADDR << " --- " << M->PPU_READ(ADDR, false) << '\n';
            }

            return CHR_ROM[M->PPU_READ(ADDR, false)]; 
        } 
    } else {
        //Write. The only thing writable to here is the CHR data on the cartridge which might not even be possible
        //CHR_ROM[M->PPU_READ(ADDR, false)] = VAL;
    }

    return *P_BUF;
}