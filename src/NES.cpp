#include <iostream>
#include <chrono>
#include "CPU.hpp"
#include "Display.hpp"
#include <time.h>
#include <thread>

bool pause;
bool log;
SDL_cond* mainPPUCond;
SDL_mutex* mainThreadMutex;

int CPU_Run(void* data);
int PPU_Run(void* data);
 
int main(int argc, char *argv[]) {
    Cartridge C;
    PPU RICOH_2C02(C);
    CPU MOS_6502(C, RICOH_2C02);
    Display Screen;
    SDL_Event evt;
    bool quit = false;
    pause = false;
    log = false;

    SDL_Thread* PPU_Thread;
    SDL_Thread* CPU_Thread;
    //SDL_Thread* APU_Thread;
    mainThreadMutex = SDL_CreateMutex();
    mainPPUCond = SDL_CreateCond();
    
    if (argc > 1)
        std::cout << argv[1] << std::endl;

    if (argc > 1)
        C.LOAD(argv[1]);

    SDL_LockMutex(mainThreadMutex);

    int elapsedTime;
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameStart = std::chrono::high_resolution_clock::now();

    //Start the threads
    CPU_Thread = SDL_CreateThread(CPU_Run, "CPU", &MOS_6502);
    PPU_Thread = SDL_CreateThread(PPU_Run, "PPU", &RICOH_2C02);
    //APU_Thread = SDL_CreateThread(APU_Run, "APU", &RICOH_2A03);
    SDL_DetachThread(CPU_Thread);
    SDL_DetachThread(PPU_Thread);
    //SDL_DetachThread(APU_Thread);

    while (!quit) {
        frameStart = std::chrono::high_resolution_clock::now();

        //Wait on PPU
        SDL_CondWait(mainPPUCond, mainThreadMutex);
        
        //Draw frame
        Screen.RENDER_FRAME(RICOH_2C02.framePixels);
         
        //Check for events
        if(SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT)
                quit = true;
            else if (evt.type == SDL_KEYDOWN) {
                switch (evt.key.keysym.sym) {
                    case SDLK_p:
                        pause = true;
                        break;
                    case SDLK_l:
                        log = true;
                        break;
                }

                while (pause) {
                    if(SDL_PollEvent(&evt)) {
                        if (evt.type == SDL_KEYDOWN) {
                            if (evt.key.keysym.sym == SDLK_p)
                                pause = false;
                            else if (evt.key.keysym.sym == SDLK_l)
                                log = false;
                        }
                    }
                    SDL_Delay(500);
                }
            }
        }
        
        //Wait if needed
        frameEnd = std::chrono::high_resolution_clock::now();
        elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart).count();
        std::cout << "FPS: " << (1000/elapsedTime) << '\n';
        if (elapsedTime < 16)
            std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsedTime));
    }

    return 0;
}


int CPU_Run(void* data) {
    
    ((CPU* )data)->RUN();
    
    return 0;
}


int PPU_Run(void* data) {
    
    ((PPU* )data)->GENERATE_SIGNAL();

    return 0;
}