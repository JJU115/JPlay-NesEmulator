#ifndef CPU_H
#define CPU_H

//Fill in FETCH instruction -- To be completed later

#include <iostream>
#include <chrono>
#include <thread>
#include "Cartridge.hpp"
#include "PPU.hpp"
#include <mutex>
#include <condition_variable>


typedef std::chrono::time_point<std::chrono::high_resolution_clock> HR_CLOCK;


class CPU {
    public:
        void RUN();
        CPU(Cartridge& NES, PPU& P1):ACC(0), IND_X(0), IND_Y(0), STAT(0x34), STCK_PNT(0xFD), PROG_CNT(0xFFFC), 
        ROM(&NES), P(&P1) {}
        
    private:
        uint8_t FETCH(uint16_t ADDR, bool SAVE);
        void WRITE(uint8_t VAL, uint16_t ADDR);
        void EXEC(uint8_t OP, char ADDR_TYPE);
        void BRANCH(char FLAG, char VAL);
        void STACK_PUSH(uint8_t BYTE);
        unsigned char STACK_POP();
        void RESET();
        void IRQ_NMI(uint16_t V);
        void WAIT();

        Cartridge *ROM;
        PPU *P;

        std::array<uint8_t, 2048> RAM;
        uint8_t ACC, IND_X, IND_Y, STAT, STCK_PNT;
        uint16_t PROG_CNT;

};


#endif