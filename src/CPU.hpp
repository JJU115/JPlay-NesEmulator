#include <iostream>
#include <chrono>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> HR_CLOCK;

class CPU {
    public:
        void RUN();
        unsigned char FETCH(unsigned short ADDR);
        void EXEC(unsigned char OP, char ADDR_TYPE, HR_CLOCK TIME);
        HR_CLOCK WAIT(HR_CLOCK TIME);

        CPU():ACC(0), IND_X(0), IND_Y(0), STAT(0x34), STCK_PNT(0xFD), PROG_CNT(0xFFFC) {}
    private:
        unsigned char RAM[2048];
        unsigned short PROG_CNT;
        unsigned char STAT, STCK_PNT, ACC, IND_X, IND_Y;
};



unsigned char CPU::FETCH(unsigned short ADDR) {
    return 0x29;
}


HR_CLOCK CPU::WAIT(HR_CLOCK TIME) {
    //Take current time, then wait until 559ns have passed from parameter time value
    //Take current time again then pass as return value
}


//RUN should just call EXEC with addressing type
void CPU::RUN() {
    //Generate a reset interrupt

    auto start = std::chrono::high_resolution_clock::now();
    unsigned char CODE = FETCH(PROG_CNT++);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = end - start;
    //Wait until end of cycle

    start = std::chrono::high_resolution_clock::now();

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
                    EXEC(CODE, 8, start);
                    break;
                }    

                switch (CODE & 0x03) {  //Extract bits 0,1
                    case 0:
                        EXEC(CODE, (CODE & 0x1C) >> 2, start);
                        break;
                    case 1:
                        switch ((CODE & 0x1C) >> 2) {
                            case 0:
                                EXEC(CODE, 2, start);
                                break;
                            case 2:
                                EXEC(CODE, 0, start);
                                break;
                            default:
                                EXEC(CODE, (CODE & 0x1C) >> 2, start);
                        }
                        break;
                    case 2:
                        EXEC(CODE, (CODE & 0x1C) >> 2, start);
                        break;
                }
        }
        break;
        //At this point, all cycles of the instruction have been executed

        //Following is opcode fetch of next instruction
        start = std::chrono::high_resolution_clock::now();
        CODE = FETCH(PROG_CNT++);
        WAIT(start);
    }
}



//Branch instructions get separate function
void CPU::EXEC(unsigned char OP, char ADDR_TYPE, HR_CLOCK TIME) {

    unsigned char REG = 0, VAL = 0, TEMP = 0;

    switch (ADDR_TYPE) {
        //Immediate
        case 0:
            VAL = FETCH(PROG_CNT++);
            break;
        //Zero-Page
        case 1:
            //Fetch the addresss
            VAL = FETCH(PROG_CNT++);
            TIME = WAIT(TIME);
            //Fetch value
            VAL = FETCH(0x0000 | VAL);
            break;
        //Indexed-Indirect
        case 2: {
            unsigned char POINT = FETCH(PROG_CNT++);
            TIME = WAIT(TIME);
            POINT += IND_X;
            TIME = WAIT(TIME);
            unsigned char LOW = FETCH(POINT % 256);
            TIME = WAIT(TIME);
            unsigned short HIGH = FETCH((POINT + 1) % 256);
            TIME = WAIT(TIME);
            VAL = FETCH(LOW | (HIGH << 8));
            break; }
        //Absolute
        case 3: {
            unsigned char LOW = FETCH(PROG_CNT++);
            TIME = WAIT(TIME);
            unsigned short HIGH = FETCH(PROG_CNT++);
            TIME = WAIT(TIME);
            VAL = FETCH(LOW | (HIGH << 8));
            break; }
        //Indirect-Indexed
        case 4:
            unsigned char POINT = FETCH(PROG_CNT++);
            TIME = WAIT(TIME);
            unsigned char LOW = FETCH(POINT);
            TIME = WAIT(TIME);
            unsigned short HIGH = FETCH((POINT + 1) % 256);
            TIME = WAIT(TIME);
            VAL = FETCH((LOW | (HIGH << 8)) + IND_Y);
            TIME = WAIT(TIME);
    }


    switch (OP) {
        //ADC
        case 0x61:
        case 0x65:
        case 0x69:
        case 0x6D:
        case 0x71:
        case 0x75:
        case 0x79:
        case 0x7D:
            ACC += VAL + (STAT & 0x01);
            break;
        //AND
        case 0x21:
        case 0x25:
        case 0x29:
        case 0x2D:
        case 0x31:
        case 0x35:
        case 0x39:
        case 0x3D:
            ACC &= VAL;
            break;
        //ASL
        case 0x0A:
        case 0x1E:
        case 0x16:
        case 0x0E:
        case 0x06:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
            break;
        //BIT
        case 0x24:
        case 0x2C:
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
        //CMP
        case 0xC1:
        case 0xC5:
        case 0xC9:
        case 0xCD:
        case 0xD1:
        case 0xD5:
        case 0xD9:
        case 0xDD:
            if (ACC >= VAL)
                STAT |= 0x01;
            if (ACC == VAL)
                STAT |= 0x02;
            break;
        //CPX
        case 0xE0:
        case 0xE4:
        case 0xEC:
            if (IND_X >= VAL)
                STAT |= 0x01;
            if (IND_X == VAL)
                STAT |= 0x02;
            REG = 1;
            break;
        //CPY
        case 0xC0:
        case 0xC4:
        case 0xCC:
            if (IND_Y >= VAL)
                STAT |= 0x01;
            if (IND_Y == VAL)
                STAT |= 0x02;
            REG = 2;
            break;
        //DEC
        case 0xD6:
        case 0xDE:
        case 0xCE:
        case 0xC6:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
            VAL--;
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
        //EOR
        case 0x41:
        case 0x45:
        case 0x49:
        case 0x4D:
        case 0x51:
        case 0x55:
        case 0x59:
        case 0x5D:
            ACC ^= VAL;
            break;
        //INC
        case 0xEE:
        case 0xE6:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
        case 0xFE:
        case 0xF6:
            VAL++;
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
        //LDA
        case 0xA1:
        case 0xA5:
        case 0xA9:
        case 0xAD:
        case 0xB1:
        case 0xB5:
        case 0xB9:
        case 0xBD:
            ACC = VAL;
            break;
        //LDX
        case 0xA2:
        case 0xA6:
        case 0xAE:
        case 0xB6:
        case 0xBE:
            IND_X = VAL;
            REG = 1;
            break;
        //LDY
        case 0xA0:
        case 0xA4:
        case 0xB4:
        case 0xAC:
        case 0xBC:
            IND_Y = VAL;
            REG = 2;
            break;
        //LSR
        case 0x4A:
        case 0x4E:
        case 0x46:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
        case 0x5E:
        case 0x56:
            STAT &= 0xFE;
            STAT |= (0x01 & ACC);
            ACC = ACC >> 1;
            REG = 1; 
            break;
        //NOP
        case 0xEA:
            break;
        //ORA
        case 0x01:
        case 0x05:
        case 0x09:
        case 0x0D:
        case 0x11:
        case 0x15:
        case 0x19:
        case 0x1D:
            ACC |= VAL;
            break;
        //ROL
        case 0x2A:
        case 0x2E:
        case 0x26:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
        case 0x3E:
        case 0x36:  
            TEMP = TEMP << 1;
            TEMP |= (0x01 & STAT);
            STAT &= 0xFE;
            STAT |= ((0x80 & VAL) >> 7);
            VAL = TEMP;
            break;
        //ROR
        case 0x6A:
        case 0x6E:
        case 0x66:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
        case 0x7E:
        case 0x76:
            TEMP = TEMP >> 1;
            TEMP |= ((0x01 & STAT) << 7);
            STAT &= 0xFE;
            STAT |= (0x01 & VAL);
            VAL = TEMP;
            break;
        //SBC
        case 0xE1:
        case 0xE5:
        case 0xE9:
        case 0xED:
        case 0xF1:
        case 0xF5:
        case 0xF9:
        case 0xFD:
            ACC -= VAL - ~(STAT & 0x01);
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
        //STA
        case 0x81:
        case 0x85:
        case 0x89:
        case 0x8D:
        case 0x91:
        case 0x95:
        case 0x99:
        case 0x9D:
            VAL = ACC;
            break;
        //STX
        case 0x86:
        case 0x8E:
        case 0x96:
            VAL = IND_X;
            break;
        //STY
        case 0x84:
        case 0x94:
        case 0x8C:
            VAL = IND_Y;
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

    //To wait out the last cycle of most instructions
    WAIT(TIME);
}