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


struct Sprite {


};



class PPU {
    public:
        void GENERATE_SIGNAL(Cartridge& NES);
        PPU():PPUCTRL(0), PPUMASK(0), PPUSTATUS(0), OAMADDR(0),
        PPUSCROLL(0), PPUADDR(0), PPUDATA(0), VRAM_ADDR(0), BGSHIFT_ONE(0), BGSHIFT_TWO(0), ATTRSHIFT_ONE(0), ATTRSHIFT_TWO(0) {}
    private:
        void PRE_RENDER();
        void SCANLINE();
        void POST_RENDER();
        void CYCLE();
        void VBLANK();

        Cartridge *ROM;

        //Set the initial size of the OAMs?
        std::array<uint8_t, 2048> VRAM; //May need less based on cartridge configuration
        std::vector<Sprite> OAM_PRIMARY;
        std::vector<Sprite> OAM_SECONDARY;

        //group these together somehow?
        uint8_t PPUCTRL;
        uint8_t PPUMASK;
        uint8_t PPUSTATUS;
        uint8_t OAMADDR;
        uint8_t OAMDATA;
        uint8_t PPUSCROLL;
        uint8_t PPUADDR;
        uint8_t PPUDATA;
        uint8_t OAMDMA;       
        

        uint16_t VRAM_ADDR;
        //uint16_t VRAM_TEMP;
        //uint8_t FINE_X;
        //uint8_t WRITE_TOGGLE

        uint16_t BGSHIFT_ONE, BGSHIFT_TWO;
        uint8_t ATTRSHIFT_ONE, ATTRSHIFT_TWO;


};


#endif