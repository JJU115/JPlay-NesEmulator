#include <iostream>
#include <chrono>

class CPU {
    public:
        void RUN();
        short FETCH(short ADDR);
        void EXEC(unsigned char OP, char ADDR_TYPE);
        void WAIT();

        CPU():ACC(0), IND_X(0), IND_Y(0), STAT(0x34), STCK_PNT(0xFD), PROG_CNT(0xFFFC) {}
    private:
        char RAM[2048];
        short ACC, IND_X, IND_Y, PROG_CNT;
        char STAT, STCK_PNT;
};


//Values fetched are in two's complement!
short CPU::FETCH(short ADDR) {
    return 0x29;
}


void CPU::WAIT() {

}


//RUN should just call EXEC with addressing type
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
                    EXEC(CODE, 8);
                    break;
                }    

                switch (CODE & 0x03) {  //Extract bits 0,1
                    case 0:
                        EXEC(CODE, (CODE & 0x1C) >> 2);
                        break;
                    case 1:
                        switch ((CODE & 0x1C) >> 2) {
                            case 0:
                                EXEC(CODE, 2);
                                break;
                            case 1:
                                EXEC(CODE, 1);
                                break;
                            case 2:
                                EXEC(CODE, 0);
                                break;
                            case 3:
                                EXEC(CODE, 3);
                                break;
                            case 4:
                                EXEC(CODE, 4);
                                break;
                            case 5:
                                EXEC(CODE, 5);
                                break;
                            case 6:
                                EXEC(CODE, 6);
                                break;
                            case 7:
                                EXEC(CODE, 7);
                                break;
                        }
                        break;
                    case 2:
                        EXEC(CODE, (CODE & 0x1C) >> 2);
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



//Branch instructions may get separate function
void CPU::EXEC(unsigned char OP, char ADDR_TYPE) {

    //Put in VAL and set values properly, discern addressing mode
    short VAL = 0, TEMP = 0;
    char REG = 0;

    switch (ADDR_TYPE) {

    }

    if (OP & 0x0F == 0x05 || OP & 0x0F == 0x09 || OP & 0x0F == 0x0D) {
        OP &= 0xF0;
        OP |= 0x01;
    }

    if (OP & 0x0F == 0x0E) {
        OP &= 0xF0;
        OP |= 0x06;
    }

    switch (OP) {
        //ADC
        case 0x61:
        case 0x71:
            ACC += VAL + (STAT & 0x01);
            break;
        //AND
        case 0x21:
        case 0x31:
            ACC &= VAL;
            break;
        //ASL
        case 0x0A:
        case 0x06:
        case 0x16:
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
        case 0xD1:
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
        case 0xC6:
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
        case 0x51:
            ACC ^= VAL;
            break;
        //INC
        case 0xE6:
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
        case 0xB1:
            ACC = VAL;
            break;
        //LDX
        case 0xA2:
        case 0xA6:
        case 0xB6:
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
        case 0x46:
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
        case 0x11:
            ACC |= VAL;
            break;
        //ROL
        case 0x2A:
        case 0x26:
        case 0x36:  
            TEMP = TEMP << 1;
            TEMP |= (0x01 & STAT);
            STAT &= 0xFE;
            STAT |= ((0x80 & VAL) >> 7);
            VAL = TEMP;
            break;
        //ROR
        case 0x6A:
        case 0x66:
        case 0x76:
            TEMP = TEMP >> 1;
            TEMP |= ((0x01 & STAT) << 7);
            STAT &= 0xFE;
            STAT |= (0x01 & VAL);
            VAL = TEMP;
            break;
        //SBC
        case 0xE1:
        case 0xF1:
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
        case 0x91:
            VAL = ACC;
            break;
        //STX
        case 0x86:
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
}