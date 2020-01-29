//Make a cartridge class that has access to the game pak PRG-ROM and CHR-ROM which PPU and CPU will 
//interface with to fetch data. Instantiate in NES.cpp and set the same object as the cartridge for 
//both CPU and PPU - add new data member method as needed


#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>

int16_t TICK;


class PPU {
    public:
        void GENERATE_SIGNAL();
        PPU():PPUCTRL(0), PPUMASK(0), PPUSTATUS(0), OAMADDR(0),
        PPUSCROLL(0), PPUADDR(0), PPUDATA(0), VRAM_ADDR(0) {}
    private:
        void PRE_RENDER();
        void SCANLINE();
        void POST_RENDER();
        void CYCLE();
        void VBLANK();

        //Make a sprite class and turn the OAMs into vectors that store them
        std::array<uint8_t, 2048> VRAM; //May need less based on cartridge configuration
        std::array<uint8_t, 256> OAM_PRIMARY;
        std::array<uint8_t, 64> OAM_SECONDARY;

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


void PPU::CYCLE() {
    TICK++;
}



void PPU::GENERATE_SIGNAL() {

    int16_t SLINE_NUM = 0;

    while (true) {

        PRE_RENDER();

        while (SLINE_NUM++ < 240)
            SCANLINE();

        POST_RENDER();
        SLINE_NUM++;

        while (SLINE_NUM++ < 261)
            VBLANK();

        SLINE_NUM = 0;
    }
}



void PPU::PRE_RENDER() {

    TICK = 0;

    while (TICK < 321)
        CYCLE();

    

}


void PPU::SCANLINE() {

}


void PPU::POST_RENDER() {

}


void PPU::VBLANK() {

}


