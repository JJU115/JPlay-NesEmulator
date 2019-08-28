#include <iostream>
#include <chrono>

class CPU {
    public:
        void RUN();
        unsigned char FETCH(short ADDR);
        void IMMEDIATE(unsigned char OP);
        void ACCUMULATOR_IMPLIED(unsigned char OP);
        void ABSOLUTE(unsigned char OP);
        void ZERO_PAGE(unsigned char OP);
        void INDEX_INDIRECT(unsigned char OP);
        void INDIRECT_INDEX(unsigned char OP);
        void ABSOLUTE_INDEX(unsigned char OP, char IND);
        void ZERO_PAGE_INDEX(unsigned char OP, char IND);

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
                                ZERO_PAGE(CODE);
                                break;
                            case 3:
                                ABSOLUTE(CODE);
                                break;
                            case 5:
                                ZERO_PAGE_INDEX(CODE, IND_X);
                                break;
                            case 7:
                                ABSOLUTE_INDEX(CODE, IND_X);
                                break;
                        }
                        break;
                    case 1:
                        switch ((CODE & 0x1C) >> 2) {
                            case 0:
                                INDEX_INDIRECT(CODE);
                                break;
                            case 1:
                                ZERO_PAGE(CODE);
                                break;
                            case 2:
                                IMMEDIATE(CODE);
                                break;
                            case 3:
                                ABSOLUTE(CODE);
                                break;
                            case 4:
                                INDIRECT_INDEX(CODE);
                                break;
                            case 5:
                                ZERO_PAGE_INDEX(CODE, IND_X);
                                break;
                            case 6:
                                ABSOLUTE_INDEX(CODE, IND_Y);
                                break;
                            case 7:
                                ABSOLUTE_INDEX(CODE, IND_X);
                                break;
                        }
                        break;
                    case 2:
                        switch ((CODE & 0x1C) >> 2) {
                            case 0:
                                IMMEDIATE(CODE);
                                break;
                            case 1:
                                ZERO_PAGE(CODE);
                                break;
                            case 2:
                                ACCUMULATOR_IMPLIED(CODE);
                                break;
                            case 3:
                                ABSOLUTE(CODE);
                                break;
                            case 5:
                                ZERO_PAGE_INDEX(CODE, IND_X);
                                break;
                            case 7:
                                ABSOLUTE_INDEX(CODE, IND_X);
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


void CPU::ABSOLUTE(unsigned char OP) { 

    switch(OP) {
        case 0x6D:
            break;
        case 0x2D:
            break;
        case 0x0E:
            break;
        case 0xCD:
            break;
        case 0xEC:
            break;
        case 0xCC:
            break;
        case 0xCE:
            break;
        case 0x4D:
            break;
        case 0xEE:
            break;
        case 0xAD:
            break;
        case 0xAE:
            break;
        case 0xAC:
            break;
        case 0x4E:
            break;
        case 0x0D:
            break;
        case 0x2E:
            break;
        case 0x6E:
            break;
        case 0xED:
            break;
        case 0x8D:
            break;
        case 0x8E:
            break;
        case 0x8C:
            break;
    }

}


void CPU::ZERO_PAGE(unsigned char OP) {

    switch(OP) {
        case 0x65:
            break;
        case 0x25:
            break;
        case 0x06:
            break;
        case 0x24:
            break;
        case 0xC5:
            break;
        case 0xE4:
            break;
        case 0xC4:
            break;
        case 0xC6:
            break;
        case 0x45:
            break;
        case 0xE6:
            break;
        case 0xA5:
            break;
        case 0xA6:
            break;
        case 0xA4:
            break;
        case 0x46:
            break;
        case 0x05:
            break;
        case 0x26:
            break;
        case 0x66:
            break;
        case 0xE5:
            break;
        case 0x85:
            break;
        case 0x86:
            break;
        case 0x84:
            break;
    }            


}


void CPU::INDEX_INDIRECT(unsigned char OP) {

    switch(OP) {
        case 0x61:
            break;
        case 0x21:
            break;
        case 0xC1:
            break;
        case 0x41:
            break;
        case 0xA1:
            break;
        case 0x01:
            break;
        case 0xE1:
            break;
        case 0x81:
            break;
    }
                
}


void CPU::INDIRECT_INDEX(unsigned char OP) {

    switch(OP) {
        case 0x71:
            break;
        case 0x31:
            break;
        case 0xD1:
            break;
        case 0x51:
            break;
        case 0xB1:
            break;
        case 0x11:
            break;
        case 0xF1:
            break;
        case 0x91:
            break;
    }

}


void CPU::ABSOLUTE_INDEX(unsigned char OP, char IND) {

    if (0x0F & OP == 0x09)
        OP += 0x04; 

    switch(OP) {
        case 0x7D:
            break;
        case 0x3D:
            break;
        case 0x1E:
            break;
        case 0xDD:
            break;
        case 0xDE:
            break;
        case 0x5D:
            break;
        case 0xFE:
            break;
        case 0xBD:
            break;
        case 0xBE:
            break;
        case 0xBC:
            break;
        case 0x5E:
            break;
        case 0x1D:
            break;
        case 0x3E:
            break;
        case 0xFD:
            break;
        case 0x05:
            break;
        case 0x9D:
            break;
    }

}


void CPU::ZERO_PAGE_INDEX(unsigned char OP, char IND) {

    switch(OP) {
        case 0x75:
            break;
        case 0x35:
            break;
        case 0x16:
            break;
        case 0xD5:
            break;
        case 0xD6:
            break;
        case 0x55:
            break;
        case 0xF6:
            break;
        case 0xB5:
            break;
        case 0xB6:
            break;
        case 0xB4:
            break;
        case 0x56:
            break;
        case 0x15:
            break;
        case 0x36:
            break;
        case 0x76:
            break;
        case 0xF5:
            break;
        case 0x95:
            break;
        case 0x94:
            break;
        case 0x96:
            break;                            
}