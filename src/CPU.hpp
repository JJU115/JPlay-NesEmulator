#include <iostream>
#include <chrono>

class CPU {
    public:
        void RUN();
        unsigned char FETCH(short ADDR);
        void IMMEDIATE(unsigned char OP);
        void ACCUMULATOR_IMPLIED(unsigned char OP);
        void ABSOLUTE(unsigned char OP, unsigned char INDEX);
        void ZERO_PAGE(unsigned char OP, unsigned char INDEX);
        void INDEX_INDIRECT(unsigned char OP);
        void INDIRECT_INDEX(unsigned char OP);

        CPU():ACC(0), IND_X(0), IND_Y(0), STAT(0x34), STCK_PNT(0xFD), PROG_CNT(0xFFFC) {}
    private:
        char RAM[2048];
        char ACC, IND_X, IND_Y, STAT, STCK_PNT;
        short PROG_CNT;
};


unsigned char CPU::FETCH(short ADDR) {
    return 0xE0;
}


void CPU::RUN() {
    //Generate a reset interrupt

    auto start = std::chrono::high_resolution_clock::now();
    unsigned char CODE = FETCH(PROG_CNT++);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = end - start;
    //Wait until end of cycle

    //Main loop
    while (true) {

        //Stack Access Instructions and JMP
        switch (CODE) {
            //BRK
            case 0x00:
                break;
            //RTI
            case 0x40:
                break;
            //RTS
            case 0x60:
                break;
            //PHA
            case 0x48:
                break;
            //PHP
            case 0x08:
                break;
            //PLA
            case 0x68:
                break;
            //PLP
            case 0x28:
                break;
            //JSR
            case 0x20:
                break;
            //JMP - Absolute
            case 0x4C:
                break;
            //JMP - Indirect Absolute
            case 0x6C:
                break;
            default:
                switch (CODE & 0x03) {  //Extract bits 0,1
                    case 0:
                        switch ((CODE & 0x1C) >> 2) { //Extract bits 2,3,4
                            case 0:
                                IMMEDIATE(CODE);
                                break;
                            case 1:
                                ZERO_PAGE(CODE, 0);
                                break;
                            case 3:
                                ABSOLUTE(CODE, 0);
                                break;
                            case 5:
                                ZERO_PAGE(CODE, IND_X);
                                break;
                            case 7:
                                ABSOLUTE(CODE, IND_X);
                                break;
                        }
                        break;
                    case 1:
                        switch ((CODE & 0x1C) >> 2) {
                            case 0:
                                INDEX_INDIRECT(CODE);
                                break;
                            case 1:
                                ZERO_PAGE(CODE, 0);
                                break;
                            case 2:
                                IMMEDIATE(CODE);
                                break;
                            case 3:
                                ABSOLUTE(CODE, 0);
                                break;
                            case 4:
                                INDIRECT_INDEX(CODE);
                                break;
                            case 5:
                                ZERO_PAGE(CODE, IND_X);
                                break;
                            case 6:
                                ABSOLUTE(CODE, IND_Y);
                                break;
                            case 7:
                                ABSOLUTE(CODE, IND_X);
                                break;
                        }
                        break;
                    case 2:
                        switch ((CODE & 0x1C) >> 2) {
                            case 0:
                                IMMEDIATE(CODE);
                                break;
                            case 1:
                                ZERO_PAGE(CODE, 0);
                                break;
                            case 2:
                                ACCUMULATOR_IMPLIED(CODE);
                                break;
                            case 3:
                                ABSOLUTE(CODE, 0);
                                break;
                            case 5:
                                ZERO_PAGE(CODE, IND_X);
                                break;
                            case 7:
                                ABSOLUTE(CODE, IND_X);
                                break;
                        }
                        break;
                }
        }
        break;

        start = std::chrono::high_resolution_clock::now();
        CODE = FETCH(PROG_CNT++);
        end = std::chrono::high_resolution_clock::now();
        elapsed = end - start;
        //Wait until end of cycle
    }
}



void CPU::IMMEDIATE(unsigned char OP) {

    switch (OP) {
        //ADC
        case 0x69:
            break;
        case 0x29:
            break;
        case 0xC9:
            break;
        case 0xE0:
            break;
        case 0xC0:
            break;
        case 0x49:
            break;
        case 0xA9:
            break;
        case 0xA2:
            break;
        case 0xA0:
            break;
        case 0x09:
            break;
        case 0xE9:
            break;
    }

}


 void CPU::ACCUMULATOR_IMPLIED(unsigned char OP) {
        PROG_CNT++; //Next byte is not used so increment PC past it

        switch(OP) {
            case 0x0A:
                break;
            case 0x18:
                break;
            case 0xD8:
                break;
            case 0x58:
                break;
            case 0xB8:
                break;
            case 0xCA:
                break;
            case 0x88:
                break;
            case 0xE8:
                break;
            case 0xC8:
                break;
            case 0x4A:
                break;
            case 0xEA:
                break;
            case 0x2A:
                break;
            case 0x6A:
                break;
            case 0x38:
                break;
            case 0xF8:
                break;
            case 0x78:
                break;
            case 0xAA:
                break;
            case 0xA8:
                break;
            case 0xBA:
                break;
            case 0x8A:
                break;
            case 0x9A:
                break;
            case 0x98:
                break;
        }
}


void CPU::ABSOLUTE(unsigned char OP, unsigned char INDEX) {

}


void CPU::ZERO_PAGE(unsigned char OP, unsigned char INDEX) {

}


void CPU::INDEX_INDIRECT(unsigned char OP) {

}


void CPU::INDIRECT_INDEX(unsigned char OP) {

}