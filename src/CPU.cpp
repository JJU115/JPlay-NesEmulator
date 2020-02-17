//Replace unsigned chars and shorts with uints from <cstdint> library
//Logging won't always be enabled so calls to FETCH will have a bool extracted from the command line
//Branch instructions can have an oops cycle as well, still need to implement this
//Many instructions seem to reset flag values from 1 to 0, need to check this with MOS manual
//Program still relies on implicit conversions, need to fix this

#include "CPU.hpp"

#include <sstream> //Only include if logging enabled
#include "6502Mnemonics.hpp" //Only if logging enabled


unsigned char VAL, TEMP, LOW, HIGH, POINT;
short WBACK_ADDR;
std::ofstream LOG;
std::ifstream CONTROL;
char *B;
std::string LOG_LINE;
std::stringstream LOG_STREAM;


void CPU::STACK_PUSH(unsigned char BYTE) {
    RAM[0x0100 + STCK_PNT--] = BYTE;   
}


unsigned char CPU::STACK_POP() {
    return RAM[0x0100 + STCK_PNT];
}


//Will probably include separate ROM class to use in this function
uint8_t CPU::FETCH(unsigned short ADDR, bool SAVE=false) {
    //Every 0x0800 bytes of 0x0800 -- 0x1FFF mirrors 0x0000 -- 0x07FF
    if (ADDR < 0x2000)
        ADDR &= 0x07FF;

    //0x2008 -- 0x3FFF mirrors 0x2000 -- 0x2007 every 8 bytes
    if (ADDR >= 0x2008 && ADDR < 0x4000)
        ADDR &= 0x2007;

    if (ADDR < 0x2000) {
        if (SAVE)
            LOG << std::hex << int(RAM[ADDR]) << " ";
        return RAM[ADDR];
    }


    if (ADDR >= 0x4020) {
       if (SAVE) {
            uint8_t L = ROM->CPU_READ(ADDR);
            LOG << std::hex << int(L) << " ";
            return L;
       }
        return ROM->CPU_READ(ADDR);
    } 


    return 0;
}

//No PPU/APU registers yet, can only write to RAM 
void CPU::WRITE(unsigned char VAL, unsigned short ADDR) {
    if (ADDR < 0x2000)
        ADDR &= 0x07FF;

    RAM[ADDR] = VAL;
}

//If master clock times cycles, wait functions like this may not have to sleep, just wait on condition variable
HR_CLOCK CPU::WAIT(HR_CLOCK TIME) {
    HR_CLOCK now = std::chrono::high_resolution_clock::now();
   /*
    auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(now - TIME);
    if (diff.count() >= 558)
        return std::chrono::high_resolution_clock::now();
*/
  /*  std::chrono::duration<int,std::nano> point (558); //Not sure this is needed
    struct timespec req = {0};
    req.tv_sec = 0;
    req.tv_nsec = 558 - diff.count();
    nanosleep(&req, (struct timespec *)NULL);*/
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    std::cout << "Cycle" << std::endl;
    return std::chrono::high_resolution_clock::now();

}


void CPU::RESET(HR_CLOCK start) {
    start = WAIT(start);
    start = WAIT(start);
    STCK_PNT--;
    start = WAIT(start);
    STCK_PNT--;
    start = WAIT(start);

    STAT &= 0xEF;
    STCK_PNT--;
    start = WAIT(start);

    PROG_CNT &= 0;
    PROG_CNT |= FETCH(0xFFFC);
    STAT |= 0x04;
    start = WAIT(start);

    PROG_CNT |= FETCH(0xFFFD) << 8;
    start = WAIT(start);
}


void CPU::IRQ_NMI(HR_CLOCK start, uint16_t V) {
    start = WAIT(start);
    start = WAIT(start);

    STACK_PUSH(PROG_CNT >> 8);
    start = WAIT(start);

    STACK_PUSH(PROG_CNT & 0x00FF);
    start = WAIT(start);

    STAT &= 0xEF;
    STACK_PUSH(STAT);
    start = WAIT(start);

    PROG_CNT &= 0;
    PROG_CNT |= FETCH(V);
    STAT |= 0x04;
    start = WAIT(start);

    PROG_CNT |= FETCH(V) << 8;
    start = WAIT(start);
}


void CPU::RUN(Cartridge& NES) {
    RAM.fill(0); //Clear RAM
    //Enable logging
    LOG.open("CPU_LOG.txt", std::ios::trunc | std::ios::out);
    CONTROL.open("NESTEST_LOG.txt");
    B = new char[6];
    //Load Cartridge
    ROM = &NES;

    //To run nestest.nes on auto
    PROG_CNT = 0xC000;

    //Interrupt bools
    bool R = false;//true;
    bool N = false;
    bool I = false;
    bool BRK = false;

    //First cycle has started at the end of this call
    HR_CLOCK start = std::chrono::high_resolution_clock::now();

    uint8_t CODE;

    //Main loop
    while (true) {
        
        if (R) {
            RESET(start);
            R = false;
        } else if (N)
            IRQ_NMI(start, 0xFFFA);
        else if (I && !BRK)
            IRQ_NMI(start, 0xFFFE);

        start = std::chrono::high_resolution_clock::now();
        LOG << std::hex << PROG_CNT << "  ";

        CODE = FETCH(PROG_CNT++);
        start = WAIT(start);

        //Compose a string and append after EXEC
        LOG_STREAM.str(std::string());
        LOG << std::hex << int(CODE) << " ";
        LOG_STREAM << "ACC:" << std::hex << int(ACC) << " ";
        LOG_STREAM << "X:" << std::hex << int(IND_X) << " ";
        LOG_STREAM << "Y:" << std::hex << int(IND_Y) << " ";
        LOG_STREAM << "STAT:" << std::hex << int(STAT) << " ";
        LOG_STREAM << "SP:" << std::hex << int(STCK_PNT) << '\n';

        //Stack Access Instructions and JMP
        switch (CODE) {
            //BRK - This causes an interrupt
            case 0x00:
                PROG_CNT++;
                start = WAIT(start);

                STACK_PUSH(PROG_CNT >> 8);
                start = WAIT(start);

                STACK_PUSH(PROG_CNT & 0x00FF);
                start = WAIT(start);

                STAT |= 0x10;
                STACK_PUSH(STAT);
                start = WAIT(start);

                PROG_CNT &= 0;
                PROG_CNT |= FETCH(0xFFFE);
                STAT |= 0x04;
                start = WAIT(start);

                PROG_CNT |= FETCH(0xFFFF) << 8;
                start = WAIT(start);
                I = BRK = true;
                break;
            //RTI
            case 0x40:
                N = I = false;
                start = WAIT(start);

                STCK_PNT++;
                start = WAIT(start);

                STAT = STACK_POP();
                STCK_PNT++;
                start = WAIT(start);

                PROG_CNT &= 0;
                PROG_CNT |= STACK_POP();
                STCK_PNT++;
                start = WAIT(start);

                PROG_CNT |= (STACK_POP() << 8);
                start = WAIT(start);
                break;
            //RTS
            case 0x60:
                start = WAIT(start);

                STCK_PNT++;
                start = WAIT(start);

                PROG_CNT &= 0;
                PROG_CNT |= STACK_POP();
                STCK_PNT++;
                start = WAIT(start);

                PROG_CNT |= (STACK_POP() << 8);
                start = WAIT(start);

                PROG_CNT++;
                start = WAIT(start);
                break;
            //PHA
            case 0x48:
                start = WAIT(start);

                STACK_PUSH(ACC);
                start = WAIT(start);
                break;
            //PHP
            case 0x08:
                start = WAIT(start);

                STACK_PUSH(STAT);
                start = WAIT(start);
                break;
            //PLA
            case 0x68:
                start = WAIT(start);

                STCK_PNT++;
                start = WAIT(start);

                ACC = STACK_POP();
                STAT = (ACC == 0) ? (STAT | 0x02) : (STAT & 0xFD);
                STAT = ((ACC & 0x80) > 0) ? (STAT | 0x80) : (STAT & 0x7F);      
                start = WAIT(start);
                break;
            //PLP
            case 0x28:
                start = WAIT(start);

                STCK_PNT++;
                start = WAIT(start);

                STAT = STACK_POP();
                start = WAIT(start);
                break;
            //JSR
            case 0x20:
                LOW = FETCH(PROG_CNT++, true);
                start = WAIT(start);

                start = WAIT(start);

                STACK_PUSH(PROG_CNT >> 8);
                start = WAIT(start);

                STACK_PUSH(PROG_CNT & 0x00FF);
                start = WAIT(start);

                HIGH = FETCH(PROG_CNT, true);
                PROG_CNT &= 0;
                PROG_CNT |= LOW;
                PROG_CNT |= (HIGH << 8);
                start = WAIT(start);
                break;
            //JMP - Absolute
            case 0x4C:
                LOW = FETCH(PROG_CNT++, true);
                start = WAIT(start);

                HIGH = FETCH(PROG_CNT, true);
                PROG_CNT &= 0;
                PROG_CNT |= LOW;
                PROG_CNT |= (HIGH << 8);
                start = WAIT(start);
                break;
            //JMP - Indirect Absolute
            case 0x6C:
                LOW = FETCH(PROG_CNT++, true);
                start = WAIT(start);

                HIGH = FETCH(PROG_CNT++, true);
                start = WAIT(start);
                start = WAIT(start);

                PROG_CNT &= 0;
                PROG_CNT |= FETCH(((HIGH << 8) | LOW), true);
                if ((LOW & 0xFF) == 0xFF)
                    PROG_CNT |= (FETCH(HIGH << 8, true) << 8);
                else
                    PROG_CNT |= (FETCH(((HIGH << 8) | LOW) + 1) << 8);
                start = WAIT(start);
                break;
            default:
                if ((CODE & 0x0F) == 0x0A || (CODE & 0x0F) == 0x08) {
                    EXEC(CODE, 8, start);
                    break;
                }

                if ((CODE & 0x1F) == 0x10) {
                    BRANCH(CODE >> 6, (CODE >> 5) & 0x01, start);
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
                        if (CODE != 0xBE)
                            EXEC(CODE, (CODE & 0x1C) >> 2, start);
                        else
                            EXEC(CODE, 6, start);
                        break;
                }
        }
        //break;
        //At this point, all cycles of the instruction have been executed
        //Check for interrupts before next opcode fetch
      
        LOG << OPCODES[CODE] << '\t' << '\t';
        CONTROL.get(B, 6);
        CONTROL.getline(B, 6);
        LOG << LOG_STREAM.str();

        if (B != OPCODES[CODE]) {
            std::cout << "Instruction mismatch: " << B << " != " << OPCODES[CODE] << std::endl;
            break;
        }

        if (RAM[2] != 0) {
            std::cout << "Fail with code: " << RAM[2];
            break;
        }

        if (RAM[3] != 0) {
            std::cout << "Fail with code: " << RAM[3];
            break;
        }
    }
}



void CPU::EXEC(unsigned char OP, char ADDR_TYPE, HR_CLOCK TIME) {
  
    bool W_BACK = false;
    unsigned char *REG_P = nullptr;
    
    switch (ADDR_TYPE) {
        //Immediate
        case 0:
            VAL = FETCH(PROG_CNT++, true);
            break;
        //Zero-Page
        case 1:
            //Fetch the addresss
            VAL = FETCH(PROG_CNT++, true);
            TIME = WAIT(TIME);
            WBACK_ADDR = (0x0000 | VAL);
            //Fetch value
            VAL = FETCH(WBACK_ADDR);
            break;
        //Indexed-Indirect
        case 2:
            POINT = FETCH(PROG_CNT++, true);
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
            WBACK_ADDR = (0 | FETCH((POINT + IND_X) % 256, true));
            TIME = WAIT(TIME);
            WBACK_ADDR |= (FETCH((POINT + IND_X + 1) % 256, true) << 8);
            TIME = WAIT(TIME);
            VAL = FETCH(WBACK_ADDR);
            break;
        //Absolute
        case 3:
            LOW = FETCH(PROG_CNT++, true);
            TIME = WAIT(TIME);
            HIGH = FETCH(PROG_CNT++, true);
            TIME = WAIT(TIME);
            WBACK_ADDR = (LOW | (HIGH << 8));
            VAL = FETCH(WBACK_ADDR);
            break;
        //Indirect-Indexed - Potential 'oops' cycle, only for read instructions, here
        case 4:
            POINT = FETCH(PROG_CNT++, true);
            TIME = WAIT(TIME);
            LOW = FETCH(POINT);
            TIME = WAIT(TIME);
            HIGH = FETCH((POINT + 1) % 256); //test result
            //'oops' cycle
            if ((OP != 0x91) && ((LOW + IND_Y) > 255))
                TIME = WAIT(TIME); 
            TIME = WAIT(TIME);
            VAL = FETCH((LOW | (HIGH << 8)) + IND_Y);
            break;
        //Zero-Page Indexed
        case 5:
            VAL = FETCH(PROG_CNT++, true);
            TIME = WAIT(TIME);
            LOW = (OP != 0xB6 && OP != 0x96) ? (VAL + IND_X) % 256 : (VAL + IND_Y) % 256; //test result
            HIGH = 0;
            VAL = FETCH(LOW);
            TIME = WAIT(TIME);
            WBACK_ADDR = LOW;
            break;
        //Absolute Indexed - Potential 'oops' cycle, only for read instructions, here
        case 6:
        case 7: {
            WBACK_ADDR = (0 | FETCH(PROG_CNT++, true));
            TIME = WAIT(TIME);
            WBACK_ADDR |= (FETCH(PROG_CNT++, true) << 8);
            TIME = WAIT(TIME);
            //'Oops' cycle
            bool oops = (((OP & 0x0F) == 0x0D) || ((OP & 0x0F) == 0x09));
            if (oops && ((ADDR_TYPE == 6 && (LOW + IND_X) > 255) || (ADDR_TYPE == 7 && (LOW + IND_Y) > 255)))
                TIME = WAIT(TIME);
            WBACK_ADDR += (ADDR_TYPE == 7) ? IND_X : IND_Y;
            VAL = FETCH(WBACK_ADDR);
            break; }
        //Accumulator/Implied - No timed waits needed
        default:
            break;
    }

    //Maybe replace W_BACK bool with call to WRITE
    switch (OP) {
        //ADC
        case 0x61:
        case 0x71:
        case 0x79:
        case 0x75:
        case 0x7D:
        case 0x6D:
        case 0x65:
        case 0x69:
            TEMP = ACC + VAL + (STAT & 0x01);
            STAT = (((ACC & 0x80) == (VAL & 0x80)) && ((TEMP & 0x80) != (ACC & 0x80))) ? (STAT | 0x40) : (STAT & 0xBF);
            STAT = (ACC + VAL + (STAT & 0x01) > 0xFF) ? (STAT | 0x01) : (STAT & 0xFE);
            ACC = TEMP;           
            REG_P = &ACC;
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
            REG_P = &ACC;
            break;
        //ASL
        case 0x1E:
            TIME = WAIT(TIME);
        case 0x06:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
        case 0x16:
        case 0x0E:
            STAT &= 0xFE;
            STAT |= (VAL >> 7);
            VAL = VAL << 1;
            W_BACK = true;
            REG_P = &VAL;
            break;
        case 0x0A:
            STAT &= 0xFE;
            STAT |= (ACC >> 7);
            ACC = ACC << 1;
            REG_P = &ACC;
            break;
        //BIT
        case 0x24:
        case 0x2C:
            STAT &= 0x3F;
            STAT |= (VAL & 0xC0);
            if ((ACC & VAL) == 0)
                STAT |= 0x02;
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
            TEMP = ACC - VAL;
            STAT = (ACC >= VAL) ? (STAT | 0x01) : (STAT & 0xFE);
            STAT = (ACC == VAL) ? (STAT | 0x02) : (STAT & 0xFD);
            STAT = (TEMP & 0x80) ? (STAT | 0x80) : (STAT & 0x7F);
            break;
        //CPX
        case 0xE0:
        case 0xE4:
        case 0xEC:
            TEMP = IND_X - VAL;
            STAT = (IND_X >= VAL) ? (STAT | 0x01) : (STAT & 0xFE);
            STAT = (IND_X == VAL) ? (STAT | 0x02) : (STAT & 0xFD);
            STAT = (TEMP & 0x80) ? (STAT | 0x80) : (STAT & 0x7F);
            break;
        //CPY
        case 0xC0:
        case 0xC4:
        case 0xCC:
            TEMP = IND_Y - VAL;
            STAT = (IND_Y >= VAL) ? (STAT | 0x01) : (STAT & 0xFE);
            STAT = (IND_Y == VAL) ? (STAT | 0x02) : (STAT & 0xFD);
            STAT = (TEMP & 0x80) ? (STAT | 0x80) : (STAT & 0x7F);
            break;
        //DEC
        case 0xDE:
            TIME = WAIT(TIME);
        case 0xD6:
        case 0xCE:
        case 0xC6:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
            VAL--;
            W_BACK = true;
            REG_P = &VAL;
            break;
        //DEX
        case 0xCA:
            IND_X--;
            REG_P = &IND_X;
            break;
        //DEY
        case 0x88:
            IND_Y--;
            REG_P = &IND_Y;
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
            REG_P = &ACC;
            break;
        //INC
        case 0xFE:
            TIME = WAIT(TIME);
        case 0xEE:
        case 0xE6:
        case 0xF6:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
            VAL++;
            W_BACK = true;
            REG_P = &VAL;
            break;
        //INX
        case 0xE8:
            IND_X++;
            REG_P = &IND_X;
            break;
        //INY
        case 0xC8:
            IND_Y++;
            REG_P = &IND_Y;
            break;
        //LAX
        case 0xA3:
            ACC = VAL;
            IND_X = VAL;
            REG_P = &ACC;
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
            REG_P = &ACC;
            break;
        //LDX
        case 0xA2:
        case 0xA6:
        case 0xAE:
        case 0xB6:
        case 0xBE:
            IND_X = VAL;
            REG_P = &IND_X;
            break;
        //LDY
        case 0xA0:
        case 0xA4:
        case 0xB4:
        case 0xAC:
        case 0xBC:
            IND_Y = VAL;
            REG_P = &IND_Y;
            break;
        //LSR
        case 0x4A:
            STAT &= 0xFE;
            STAT |= (0x01 & ACC);
            ACC = ACC >> 1;
            REG_P = &ACC;
            break;
        case 0x5E:
            TIME = WAIT(TIME);
        case 0x4E:
        case 0x46:
        case 0x56:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
            STAT &= 0xFE;
            STAT |= (0x01 & VAL);
            VAL = VAL >> 1;
            W_BACK = true;
            REG_P = &VAL;
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
            REG_P = &ACC;
            break;
        //ROL
        case 0x2A:
            TEMP = ACC << 1;
            TEMP |= (0x01 & STAT);
            STAT &= 0xFE;
            STAT |= ((0x80 & ACC) >> 7);
            ACC = TEMP;
            REG_P = &ACC;
            break;
        case 0x3E:
            TIME = WAIT(TIME);
        case 0x2E:
        case 0x26:
        case 0x36:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
            TEMP = VAL << 1;
            TEMP |= (0x01 & STAT);
            STAT &= 0xFE;
            STAT |= ((0x80 & VAL) >> 7);
            VAL = TEMP;
            W_BACK = true;
            REG_P = &VAL;
            break;
        //ROR
        case 0x6A:
            TEMP = STAT;
            STAT &= 0xFE;
            STAT |= (0x01 & ACC);
            ACC = ACC >> 1;
            ACC |= ((TEMP & 0x01) << 7);
            REG_P = &ACC;
            break;
        case 0x7E:
            TIME = WAIT(TIME);
        case 0x6E:
        case 0x66:
        case 0x76:
            TIME = WAIT(TIME);
            TIME = WAIT(TIME);
            TEMP = STAT;
            STAT &= 0xFE;
            STAT |= (0x01 & VAL);
            VAL = VAL >> 1;
            VAL |= ((TEMP & 0x01) << 7);
            W_BACK = true;
            REG_P = &VAL;
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
            TEMP = ((STAT & 0x01) > 0) ? (ACC - VAL) : (ACC - VAL - 1);
            if ((STAT & 0x01) > 0)
                STAT = (((int8_t(ACC) - int8_t(VAL)) > 127) || ((int8_t(ACC) - int8_t(VAL)) < -128)) ? (STAT ^ 0x40) : (STAT & 0xBF);  
            else 
                STAT = (((ACC - VAL - 1) > 127) || ((ACC - VAL - 1) < -128)) ? (STAT ^ 0x40) : (STAT & 0xBF); 

            STAT = ((TEMP & 0x80) == 0) ? (STAT | 0x01) : (STAT & 0xFE);
            ACC = TEMP;
            REG_P = &ACC;
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
        case 0x91:
        case 0x9D:
        case 0x99:
            TIME = WAIT(TIME);
        case 0x81:
        case 0x85:
        case 0x89:
        case 0x8D:
        case 0x95:
            VAL = ACC;
            W_BACK = true;
            break;
        //STX
        case 0x86:
        case 0x8E:
        case 0x96:
            VAL = IND_X;
            W_BACK = true;
            break;
        //STY
        case 0x84:
        case 0x94:
        case 0x8C:
            VAL = IND_Y;
            W_BACK = true;
            break;
        //TAX
        case 0xAA:
            IND_X = ACC;
            REG_P = &IND_X;
            break;
        //TAY
        case 0xA8:
            IND_Y = ACC;
            REG_P = &IND_Y;
            break;
        //TSX
        case 0xBA:
            IND_X = STCK_PNT;
            REG_P = &IND_X;
            break;
        //TXA
        case 0x8A:
            ACC = IND_X;
            REG_P = &ACC;
            break;
        //TXS
        case 0x9A:
            STCK_PNT = IND_X;
            break;
        //TYA
        case 0x98:
            ACC = IND_Y;
            REG_P = &ACC;
            break;       
    }
    
    if (W_BACK) {
        WRITE(VAL, WBACK_ADDR);
    }

    //This section may not be necessary at all
    //Examine and change status flags if needed
    if (REG_P) {
        STAT = (*REG_P == 0) ?  (STAT | 0x02) : (STAT & 0xFD);
        STAT = ((*REG_P & 0x80) > 0) ?  (STAT | 0x80) : (STAT & 0x7F); 
    }
    
    //To wait out the last cycle of most instructions
    WAIT(TIME);
}


//If FLAG equals VAL then take the branch, another 'oops' cycle here
void CPU::BRANCH(char FLAG, char VAL, HR_CLOCK TIME) {

    switch (FLAG) {
        case 0:
            FLAG = (STAT >> 7);
            break;
        case 1:
            FLAG = ((STAT & 0x40) >> 6);
            break;
        case 2:
            FLAG = (STAT & 0x01);
            break;
        case 3:
            FLAG = ((STAT & 0x02) >> 1);
            break;
    }
    
    unsigned char OPRAND = FETCH(PROG_CNT++, true);
    TIME = WAIT(TIME);
  
    if (FLAG == VAL) {
        PROG_CNT += int8_t(OPRAND);
        TIME = WAIT(TIME);
    }

    WAIT(TIME);
}