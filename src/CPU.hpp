#ifndef CPU_H
#define CPU_H


#include <iostream>
#include <chrono>
#include <thread>
#include <SDL_keyboard.h>
#include "PPU.hpp"
#include "APU.hpp"
#include <mutex>
#include <condition_variable>


class CPU {
    public:
        void RUN();
        CPU(Cartridge& NES, PPU& P1, APU& A1):ACC(0), IND_X(0), IND_Y(0), STAT(0x34), STCK_PNT(0xFD), PROG_CNT(0xFFFC), 
        CONTROLLER1(0), CONTROLLER2(0), probe(false), IRQDelay(false), IRQPend(false), ROM(&NES), P(&P1), A(&A1) { keyboard = SDL_GetKeyboardState(NULL); }
        
    private:
        uint8_t FETCH(uint16_t ADDR, bool SAVE=true);
        void WRITE(uint8_t VAL, uint16_t ADDR, uint8_t cyc=0);
        uint8_t EXEC(uint8_t OP, char ADDR_TYPE);
        uint8_t BRANCH(char FLAG, char VAL);
        void STACK_PUSH(uint8_t BYTE);
        unsigned char STACK_POP();
        void RESET();
        void IRQ_NMI(uint16_t V);
        void WAIT(uint16_t N=1);

        Cartridge *ROM;
        PPU *P;
        APU *A;

        std::array<uint8_t, 2048> RAM;
        uint8_t ACC, IND_X, IND_Y, STAT, STCK_PNT, CONTROLLER1, CONTROLLER2;
        uint16_t PROG_CNT;

        bool probe;
        bool IRQDelay;
        bool IRQPend;
        const uint8_t *keyboard;

};


#endif