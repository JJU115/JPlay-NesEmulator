#ifndef PPU_H
#define PPU_H


//Make a cartridge class that has access to the game pak PRG-ROM and CHR-ROM which PPU and CPU will 
//interface with to fetch data. Instantiate in NES.cpp and set the same object as the cartridge for 
//both CPU and PPU - add new data member method as needed


#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include "Cartridge.hpp"
#include <mutex>
#include <condition_variable>


struct Sprite {
    uint8_t Y_POS;
    uint8_t IND;
    uint8_t ATTR;
    uint8_t X_POS;
};


class PPU {
    public:
        void GENERATE_SIGNAL();
        void REG_WRITE(uint16_t DATA);
        PPU(Cartridge& NES): PPUCTRL(0), PPUMASK(0), PPUSTATUS(0), OAMADDR(0), OAMDATA(0), PPUSCROLL(0), PPUADDR(0), PPUDATA(0), OAMDMA(0),
        VRAM_ADDR(0), BGSHIFT_ONE(0), BGSHIFT_TWO(0), ATTRSHIFT_ONE(0), ATTRSHIFT_TWO(0), ODD_FRAME(false), ROM(&NES) {}

        uint8_t PPUCTRL;
        uint8_t PPUMASK;
        uint8_t PPUSTATUS;
        uint8_t OAMADDR;
        uint8_t OAMDATA;
        uint8_t PPUSCROLL;
        uint8_t PPUADDR;
        uint8_t PPUDATA;
        uint8_t OAMDMA;
    private:
        void PRE_RENDER();
        void SCANLINE(uint16_t SLINE);
        void CYCLE(uint16_t N=1);
        void RENDER_PIXEL();
        void Y_INCREMENT();
        void X_INCREMENT();
        void PRE_SLINE_TILE_FETCH();
        void SPRITE_EVAL(uint16_t SLINE_NUM);
        uint8_t FETCH(uint16_t ADDR);

        Cartridge *ROM;

        //Set the initial size of the OAMs?
        std::array<uint8_t, 2048> VRAM; //May need less based on cartridge configuration
        std::vector<Sprite> OAM_PRIMARY;
        std::vector<Sprite> OAM_SECONDARY;

        uint16_t VRAM_ADDR;
        //uint16_t VRAM_TEMP;
        //uint8_t FINE_X;
        //uint8_t WRITE_TOGGLE

        uint16_t BGSHIFT_ONE, BGSHIFT_TWO;
        uint8_t ATTRSHIFT_ONE, ATTRSHIFT_TWO;

        std::vector<uint8_t> SPR_PAT; //Supposed to be 8 pairs of 8-bit shift registers
        std::vector<uint8_t> SPR_ATTRS;
        std::vector<uint8_t> SPR_XPOS;

        bool ODD_FRAME;
};


#endif