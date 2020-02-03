#ifndef CPU_H
#define CPU_H

//Fill in FETCH instruction -- To be completed later
//Handle interrupts - How does CPU determine if an interrupt has occurred?
//Think about breaking this into separate files
//Replace unsigned chars and shorts with uints from <cstdint> library
//Consider dynamic memory allocation for variables in RUN and EXEC
//Consider a template function for instruction execution

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
        unsigned char FETCH(unsigned short ADDR);
        void EXEC(unsigned char OP, char ADDR_TYPE, HR_CLOCK TIME);
        void BRANCH(char FLAG, char VAL, HR_CLOCK TIME);
        void STACK_PUSH(unsigned char BYTE);
        unsigned char STACK_POP();
        void RESET(HR_CLOCK start);
        void IRQ_NMI(HR_CLOCK start, uint16_t V);
        HR_CLOCK WAIT(HR_CLOCK TIME);

        Cartridge *ROM;

        unsigned char RAM[2048];    //Change to vector or std::array?
        unsigned char ACC, IND_X, IND_Y, STAT, STCK_PNT;
        unsigned short PROG_CNT;
};


#endif