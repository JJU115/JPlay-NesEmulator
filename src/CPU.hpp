#ifndef CPU_H
#define CPU_H

//Fill in FETCH instruction -- To be completed later

#include <iostream>
#include <chrono>
#include <thread>
#include "Cartridge.hpp"


typedef std::chrono::time_point<std::chrono::high_resolution_clock> HR_CLOCK;


class CPU {
    public:
        void RUN(Cartridge& NES);
        CPU():ACC(0), IND_X(0), IND_Y(0), STAT(0x34), STCK_PNT(0xFD), PROG_CNT(0xFFFC) {}
        
    private:
        uint8_t FETCH(uint16_t ADDR, bool SAVE);
        void WRITE(uint8_t VAL, uint16_t ADDR);
        void EXEC(uint8_t OP, char ADDR_TYPE, HR_CLOCK TIME);
        void BRANCH(char FLAG, char VAL, HR_CLOCK TIME);
        void STACK_PUSH(uint8_t BYTE);
        unsigned char STACK_POP();
        void RESET(HR_CLOCK start);
        void IRQ_NMI(HR_CLOCK start, uint16_t V);
        HR_CLOCK WAIT(HR_CLOCK TIME);

        Cartridge *ROM;

        std::array<uint8_t, 2048> RAM;
        uint8_t ACC, IND_X, IND_Y, STAT, STCK_PNT;
        uint16_t PROG_CNT;
};


#endif