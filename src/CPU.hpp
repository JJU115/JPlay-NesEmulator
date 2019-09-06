#include <iostream>
#include <chrono>

class CPU {
    public:
        void RUN();
        short FETCH(short ADDR);
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
        short ACC, IND_X, IND_Y, STAT, STCK_PNT, PROG_CNT;
};


//Values fetched are in two's complement!
short CPU::FETCH(short ADDR) {
    return 0x29;
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
                if (CODE & 0x0F == 0x0A || CODE & 0x0F == 0x08) {
                    ACCUMULATOR_IMPLIED(CODE);
                    break;
                }    

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

    //Fetch value, increment PC
    short VAL = FETCH(PROG_CNT++);
    char REG = 0;

    switch (OP) {
        //ADC
        case 0x69:
            ACC += VAL + (STAT & 0x01);
            break;
        //AND
        case 0x29:
            ACC &= VAL;
            break;
        //CMP
        case 0xC9:
            if (ACC >= VAL)
                STAT |= 0x01;
            if (ACC == VAL)
                STAT |= 0x02;
            break;
        //CPX
        case 0xE0:
            if (IND_X >= VAL)
                STAT |= 0x01;
            if (IND_X == VAL)
                STAT |= 0x02;
            REG = 1;
            break;
        //CPY
        case 0xC0:
            if (IND_Y >= VAL)
                STAT |= 0x01;
            if (IND_Y == VAL)
                STAT |= 0x02;
            REG = 2;
            break;
        //EOR
        case 0x49:
            ACC ^= VAL;
            break;
        //LDA
        case 0xA9:
            ACC = VAL;
            break;
        //LDX
        case 0xA2:
            IND_X = VAL;
            REG = 1;
            break;
        //LDY
        case 0xA0:
            IND_Y = VAL;
            REG = 2;
            break;
        //ORA
        case 0x09:
            ACC |= VAL;
            break;
        //SBC
        case 0xE9:
            ACC -= VAL - ~(STAT & 0x01);
            break;
    }

    if (REG == 0) {
        if (ACC == 0)
            STAT |= 0x02;
        if (ACC < 0)
            STAT |= 0x80; 
    } else if (REG == 1) {
        if (IND_X == 0)
            STAT |= 0x02;
        if (IND_X < 0)
            STAT |= 0x80;
    } else if (REG == 2) {
        if (IND_Y == 0)
            STAT |= 0x02;
        if (IND_Y < 0)
            STAT |= 0x80;
    }
    //Wait until end of 1 cycle

}


 void CPU::ACCUMULATOR_IMPLIED(unsigned char OP) {

     char REG = 0;
     short TEMP = ACC;

        switch(OP) {
            //ASL
            case 0x0A:
                STAT &= 0xFE;
                STAT |= ((0x80 & ACC) >> 7);
                ACC = ACC << 1;
                REG = 1;
                break;
            //CLC
            case 0x18:
                STAT &= 0xFE;
                break;
            //CLD
            case 0xD8:
                STAT &= 0xF7;
                break;
            //CLI
            case 0x58:
                STAT &= 0xFB;
                break;
            //CLV
            case 0xB8:
                STAT &= 0xBF;
                break;
            //DEX
            case 0xCA:
                IND_X--;
                REG = 2;
                break;
            //DEY
            case 0x88:
                IND_Y--;
                REG = 3;
                break;
            //INX
            case 0xE8:
                IND_X++;
                REG = 2;
                break;
            //INY
            case 0xC8:
                IND_Y++;
                REG = 3;
                break;
            //LSR
            case 0x4A:
                STAT &= 0xFE;
                STAT |= (0x01 & ACC);
                ACC = ACC >> 1;
                REG = 1; 
                break;
            //NOP
            case 0xEA:
                break;
            //ROL
            case 0x2A:
                TEMP = TEMP << 1;
                TEMP |= (0x01 & STAT);
                STAT &= 0xFE;
                STAT |= ((0x80 & ACC) >> 7);
                ACC = TEMP;
                REG = 1; 
                break;
            //ROR
            case 0x6A:
                TEMP = TEMP >> 1;
                TEMP |= ((0x01 & STAT) << 7);
                STAT &= 0xFE;
                STAT |= (0x01 & ACC);
                ACC = TEMP;
                REG = 1; 
                break;
            //SEC
            case 0x38:
                STAT |= 0x01;
                break;
            //SED
            case 0xF8:
                STAT |= 0x08;
                break;
            //SEI
            case 0x78:
                STAT |= 0x04;
                break;
            //TAX
            case 0xAA:
                IND_X = ACC;
                REG = 2;
                break;
            //TAY
            case 0xA8:
                IND_Y = ACC;
                REG = 3;
                break;
            //TSX
            case 0xBA:
                IND_X = STCK_PNT;
                REG = 2;
                break;
            //TXA
            case 0x8A:
                ACC = IND_X;
                REG = 0;
                break;
            //TXS
            case 0x9A:
                STCK_PNT = IND_X;
                break;
            //TYA
            case 0x98:
                ACC = IND_Y;
                REG = 1;
                break;
        }

        if (REG == 1) {
            if (ACC == 0)
                STAT |= 0x02;
            if (ACC < 0)
                STAT |= 0x80; 
        } else if (REG == 1) {
            if (IND_X == 0)
                STAT |= 0x02;
            if (IND_X < 0)
                STAT |= 0x80;
        } else if (REG == 2) {
            if (IND_Y == 0)
                STAT |= 0x02;
            if (IND_Y < 0)
                STAT |= 0x80;
        }
        

        //Wait until end of 1 cycle
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
}