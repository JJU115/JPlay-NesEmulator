#include <iostream>
#include <chrono>
#include "CPU.hpp"
#include <time.h>
#include <thread>

//These don't need to be extern, can make them properties of the CPU and PPU class
extern std::condition_variable CPU_CV;
extern std::condition_variable PPU_CV;

 
int main(int argc, char *argv[]) {
    Cartridge C;
    PPU RICOH_2C02(C);
    CPU MOS_6502(C, RICOH_2C02);
    SDL_Event evt;
    
    if (argc > 1)
        std::cout << argv[1] << std::endl;

    if (argc > 1)
        C.LOAD(argv[1]);

    std::thread CP(&CPU::RUN, &MOS_6502);
    std::thread PPU(&PPU::GENERATE_SIGNAL, &RICOH_2C02);
    
    uint8_t COUNT = 1;
    while (true) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(20));
        if ((COUNT % 4) == 0)
            PPU_CV.notify_one();
        if ((COUNT % 12) == 0) {
            CPU_CV.notify_one();
        }
        
        COUNT++;
        SDL_PollEvent(&evt);
    }

    
    return 0;
}