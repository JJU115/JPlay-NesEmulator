#include <iostream>
#include <chrono>
#include <thread>


class PPU {
    public:
        void START();
    private:
        void PRE_RENDER();
        void SCANLINE();
        void POST_RENDER();
        void CYCLE();
        void VBLANK();
}