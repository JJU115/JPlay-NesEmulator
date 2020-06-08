//Logging won't always be enabled so calls to FETCH will have a bool extracted from the command line
//Branch instructions can have an oops cycle as well, still need to implement this
//Reconsider using local over global vars for val, temp, low, high....

#include "CPU.hpp"

#include <sstream> //Only include if logging enabled
#include "6502Mnemonics.hpp" //Only if logging enabled

std::mutex CPU_MTX;
std::condition_variable CPU_COND;
std::unique_lock<std::mutex> CPU_LCK(CPU_MTX);

uint8_t VAL, TEMP, LOW, HIGH, POINT;
uint16_t WBACK_ADDR;
std::ofstream LOG;
std::ifstream CONTROL;
char *B;
std::string LOG_LINE;
std::stringstream LOG_STREAM;

short CTRL_IGNORE;


void CPU::STACK_PUSH(uint8_t BYTE) {
    RAM[0x0100 + STCK_PNT--] = BYTE;   
}


unsigned char CPU::STACK_POP() {
    return RAM[0x0100 + ++STCK_PNT];
}


//May not have CPU access PPU regs directly, instead CPU will call PPU function which reads for the CPU
uint8_t CPU::FETCH(uint16_t ADDR, bool SAVE=false) {
    //LOG << "CPU read of " << std::hex << int(ADDR) << '\n';
    //Every 0x0800 bytes of 0x0800 -- 0x1FFF mirrors 0x0000 -- 0x07FF
    if (ADDR < 0x2000)
        ADDR &= 0x07FF;

    uint8_t TEMP;
    //0x2008 -- 0x3FFF mirrors 0x2000 -- 0x2007 every 8 bytes
    if (ADDR >= 0x2000 && ADDR < 0x4000)
        return P->REG_READ((ADDR & 0x2007) % 0x2000);

    if (ADDR < 0x2000) {
        if (SAVE)
            LOG << std::hex << int(RAM[ADDR]) << " ";
        return RAM[ADDR];
    }


    if ((ADDR >= 0x4020) && (ADDR <= 0xFFFF)) {
       if (SAVE) {
            uint8_t L = ROM->CPU_ACCESS(ADDR);
            //LOG << std::hex << int(L) << " ";
            return L;
       }
        return ROM->CPU_ACCESS(ADDR);
    } 


    return 0;
}

//Like with fetch, may not have CPU directly access PPU regs
void CPU::WRITE(uint8_t VAL, uint16_t ADDR) {
    //LOG << "CPU write of " << int(VAL) << " to " << std::hex << int(ADDR) << '\n';
    if (ADDR < 0x2000) {
        ADDR &= 0x07FF;
        RAM[ADDR] = VAL;
    }

    //PPU registers - WRITE_BUF stores VAL in the lower 8 bits and a number representing the register to write to
    //in the 8th bit
    if ((ADDR >= 0x2000) && (ADDR <= 0x3FFF))
        P->REG_WRITE(VAL, (ADDR & 0x2007) % 0x2000);
        

    //CPU is suspended during OAM DMA transfer, 513 or 514 cycles, need some way to determine odd vs even cpu cycles
    //transfer begins at current OAM write address
    if (ADDR == 0x4014) {
        P->OAMDMA = VAL;
        WAIT();  
        for (uint16_t i=0; i<256; i++) {
            WAIT();
            WAIT();
            P->REG_WRITE(FETCH((VAL << 8) + i), 4);
        }
    }

    if ((ADDR >= 0x4020) && (ADDR <= 0xFFFF))
        ROM->CPU_ACCESS(ADDR, VAL, false);
    
}


inline void CPU::WAIT() {
    //Wait on condition var, will be signaled by the PPU every time it goes through 3 ticks
    //CPU_COND.wait(CPU_LCK);
    //std::this_thread::sleep_for(std::chrono::nanoseconds(558))
    if (CTRL_IGNORE < 30000)
        CTRL_IGNORE++;
}

void CPU::RESET() {
    WAIT();
    WAIT();
    STCK_PNT--;
    WAIT();
    STCK_PNT--;
    WAIT();

    STAT &= 0xEF;
    STCK_PNT--;
    WAIT();

    PROG_CNT &= 0;
    PROG_CNT |= FETCH(0xFFFC);
    STAT |= 0x04;
    WAIT();

    PROG_CNT |= (FETCH(0xFFFD) << 8);
    WAIT();
    CTRL_IGNORE = 0;

}


void CPU::IRQ_NMI(uint16_t V) {
    WAIT();
    WAIT();

    STACK_PUSH(PROG_CNT >> 8);
    WAIT();

    STACK_PUSH(PROG_CNT & 0x00FF);
    WAIT();

    STAT &= 0xEF;
    STACK_PUSH(STAT);
    WAIT();

    PROG_CNT &= 0;
    PROG_CNT |= FETCH(V++);
    STAT |= 0x04;
    WAIT();

    PROG_CNT |= (FETCH(V) << 8);
    WAIT();
}

//Concerns regearding interrupts, specifically when and where to set interrupt bools, need to look into it more
void CPU::RUN() {
    RAM.fill(0); //Clear RAM
    CTRL_IGNORE = 0;
    
    //Enable logging
    LOG.open("CPU_LOG.txt", std::ios::trunc | std::ios::out);
    //CONTROL.open("NESTEST_LOG.txt");
    //B = new char[6];
    //std::cout << "CPU start\n";
    //To run nestest.nes on auto
    //PROG_CNT = 0x8000;

    //Interrupt bools
    bool R = true;//false for nestest;
    bool I = false;
    bool BRK = false;

    //First cycle has started

    uint8_t CODE;
    
    //Main loop
    while (true) {
        //LOG << "Interrupts - R: " << R << " NMI: " << int(P->GEN_NMI) << " I: " << I << " BRK: " << BRK << '\n';
        if (R) {
            RESET();
            R = false;
        } else if (P->GEN_NMI) {
            P->GEN_NMI = 0;
            IRQ_NMI(0xFFFA);
        } else if (I && !BRK)
            IRQ_NMI(0xFFFE);

        //LOG << std::hex << "Cycle 1\n";
        CODE = FETCH(PROG_CNT++);
        WAIT();

        //Compose a string and append after EXEC
        LOG_STREAM.str(std::string());
        //LOG << std::hex << int(CODE) << " ";
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
                WAIT();

                STACK_PUSH(PROG_CNT >> 8);
                WAIT();

                STACK_PUSH(PROG_CNT & 0x00FF);
                WAIT();

                STAT |= 0x10;
                STACK_PUSH(STAT);
                WAIT();

                PROG_CNT &= 0;
                PROG_CNT |= FETCH(0xFFFE);
                STAT |= 0x04;
                WAIT();

                PROG_CNT |= FETCH(0xFFFF) << 8;
                WAIT();
                I = BRK = true;
                break;
            //RTI
            case 0x40:
                I = false;
                WAIT();

                WAIT();

                STAT = STACK_POP();
                WAIT();

                PROG_CNT &= 0;
                PROG_CNT |= STACK_POP();
                WAIT();

                PROG_CNT |= (STACK_POP() << 8);
                WAIT();
                break;
            //RTS
            case 0x60:
                WAIT();

                WAIT();

                PROG_CNT &= 0;
                PROG_CNT |= STACK_POP();
                WAIT();

                PROG_CNT |= (STACK_POP() << 8);
                WAIT();

                PROG_CNT++;
                WAIT();
                break;
            //PHA
            case 0x48:
                WAIT();

                STACK_PUSH(ACC);
                WAIT();
                break;
            //PHP
            case 0x08:
                WAIT();

                STACK_PUSH(STAT);
                WAIT();
                break;
            //PLA
            case 0x68:
                WAIT();

                WAIT();

                ACC = STACK_POP();
                STAT = (ACC == 0) ? (STAT | 0x02) : (STAT & 0xFD);
                STAT = ((ACC & 0x80) > 0) ? (STAT | 0x80) : (STAT & 0x7F);      
                WAIT();
                break;
            //PLP
            case 0x28:
                WAIT();

                WAIT();

                STAT = STACK_POP();
                WAIT();
                break;
            //JSR
            case 0x20:
                LOW = FETCH(PROG_CNT++, true);
                WAIT();

                WAIT();

                STACK_PUSH(PROG_CNT >> 8);
                WAIT();

                STACK_PUSH(PROG_CNT & 0x00FF);
                WAIT();

                HIGH = FETCH(PROG_CNT, true);
                PROG_CNT &= 0;
                PROG_CNT |= LOW;
                PROG_CNT |= (HIGH << 8);
                WAIT();
                break;
            //JMP - Absolute
            case 0x4C:
                LOW = FETCH(PROG_CNT++, true);
                WAIT();

                HIGH = FETCH(PROG_CNT, true);
                PROG_CNT &= 0;
                PROG_CNT |= LOW;
                PROG_CNT |= (HIGH << 8);
                WAIT();
                break;
            //JMP - Indirect Absolute
            case 0x6C:
                LOW = FETCH(PROG_CNT++, true);
                WAIT();

                HIGH = FETCH(PROG_CNT++, true);
                WAIT();
                WAIT();

                PROG_CNT &= 0;
                PROG_CNT |= FETCH(((HIGH << 8) | LOW), true);
                if ((LOW & 0xFF) == 0xFF)
                    PROG_CNT |= (FETCH(HIGH << 8, true) << 8);
                else
                    PROG_CNT |= (FETCH(((HIGH << 8) | LOW) + 1) << 8);
                WAIT();
                break;
            default:
                if ((CODE & 0x0F) == 0x0A || (CODE & 0x0F) == 0x08) {
                    EXEC(CODE, 8);
                    break;
                }

                if ((CODE & 0x1F) == 0x10) {
                    BRANCH(CODE >> 6, (CODE >> 5) & 0x01);
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
                            case 2:
                                EXEC(CODE, 0);
                                break;
                            default:
                                EXEC(CODE, (CODE & 0x1C) >> 2);
                        }
                        break;
                    case 2:
                        if (CODE != 0xBE)
                            EXEC(CODE, (CODE & 0x1C) >> 2);
                        else
                            EXEC(CODE, 6);
                        break;
                }
        }
        //break;
        //At this point, all cycles of the instruction have been executed
        //LOG << "Instr end\n";
        //LOG << OPCODES[CODE] << '\t' << '\t';
        //CONTROL.get(B, 6);
        //CONTROL.getline(B, 6);
        //LOG << LOG_STREAM.str();

       /* if (B != OPCODES[CODE]) {
            std::cout << "Instruction mismatch: " << B << " != " << OPCODES[CODE] << std::endl;
            break;
        }*/

    }
}

/*
Problem: All addressing types don't distinguish between reads and writes so a write will first read the place its writing to,
then write the value there. Doesn't affect CPU operation but causes an issue when accessing PPU registers since some registers 
changes the write toggle, VRAM address, or temporary VRAM address. 

Solved? - Moved the FETCH call from the ADDR_TYPE switch statement that initializes VAL to the individual instruction cases
in the main switch statement. Of course this affects cycle-by-cycle accuracy marginally but allows proper functionality. CPU passes
nestest so will reconcile with PPU operation.

*/

void CPU::EXEC(uint8_t OP, char ADDR_TYPE) {
  
    bool W_BACK = false;
    uint8_t *REG_P = nullptr;
    
    switch (ADDR_TYPE) {
        //Immediate
        case 0:
            VAL = FETCH(PROG_CNT++, true);
            break;
        //Zero-Page
        case 1:
            //Fetch the addresss
            VAL = FETCH(PROG_CNT++, true);
            WAIT();
            WBACK_ADDR = (0x0000 | VAL);
            //Fetch value
            //VAL = FETCH(WBACK_ADDR);
            break;
        //Indexed-Indirect
        case 2:
            POINT = FETCH(PROG_CNT++, true);
            WAIT();
            WAIT();
            WBACK_ADDR = (0 | FETCH((POINT + IND_X) % 256, true));
            WAIT();
            WBACK_ADDR |= (FETCH((POINT + IND_X + 1) % 256, true) << 8);
            WAIT();
            //VAL = FETCH(WBACK_ADDR);
            break;
        //Absolute
        case 3:
            LOW = FETCH(PROG_CNT++, true);
            WAIT();
            HIGH = FETCH(PROG_CNT++, true);
            WAIT();
            WBACK_ADDR = (LOW | (HIGH << 8));
            //VAL = FETCH(WBACK_ADDR);
            break;
        //Indirect-Indexed - Potential 'oops' cycle, only for read instructions, here
        case 4:
            POINT = FETCH(PROG_CNT++, true);
            WAIT();
            LOW = FETCH(POINT);
            WAIT();
            HIGH = FETCH((POINT + 1) % 256); //test result
            //'oops' cycle
            if ((OP != 0x91) && ((LOW + IND_Y) > 255))
                WAIT(); 
            WAIT();
            WBACK_ADDR = ((LOW | (HIGH << 8)) + IND_Y);
           // VAL = FETCH((LOW | (HIGH << 8)) + IND_Y);
            break;
        //Zero-Page Indexed
        case 5:
            VAL = FETCH(PROG_CNT++, true);
            WAIT();
            LOW = (OP != 0xB6 && OP != 0x96) ? (VAL + IND_X) % 256 : (VAL + IND_Y) % 256; //test result
            HIGH = 0;
           // VAL = FETCH(LOW);
            WAIT();
            WBACK_ADDR = LOW;
            break;
        //Absolute Indexed - Potential 'oops' cycle, only for read instructions, here
        case 6:
        case 7: {
            WBACK_ADDR = (0 | FETCH(PROG_CNT++, true));
            WAIT();
            WBACK_ADDR |= (FETCH(PROG_CNT++, true) << 8);
            WAIT();
            //'Oops' cycle
            bool oops = (((OP & 0x0F) == 0x0D) || ((OP & 0x0F) == 0x09));
            if (oops && ((ADDR_TYPE == 6 && (LOW + IND_X) > 255) || (ADDR_TYPE == 7 && (LOW + IND_Y) > 255)))
                WAIT();
            WBACK_ADDR += (ADDR_TYPE == 7) ? IND_X : IND_Y;
            //VAL = FETCH(WBACK_ADDR);
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
            VAL = FETCH(WBACK_ADDR);
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
        case 0x3D:
        case 0x2D:
        case 0x31:
        case 0x35:
        case 0x39:
            VAL = FETCH(WBACK_ADDR);
        case 0x29:
            ACC &= VAL;
            REG_P = &ACC;
            break;
        //ASL
        case 0x1E:
            WAIT();
        case 0x06:
            WAIT();
            WAIT();
        case 0x16:
        case 0x0E:
            VAL = FETCH(WBACK_ADDR);
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
            VAL = FETCH(WBACK_ADDR);
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
        case 0xDD:
        case 0xCD:
        case 0xD1:
        case 0xD5:
        case 0xD9:
            VAL = FETCH(WBACK_ADDR);
        case 0xC9:
            TEMP = ACC - VAL;
            STAT = (ACC >= VAL) ? (STAT | 0x01) : (STAT & 0xFE);
            STAT = (ACC == VAL) ? (STAT | 0x02) : (STAT & 0xFD);
            STAT = (TEMP & 0x80) ? (STAT | 0x80) : (STAT & 0x7F);
            break;
        //CPX
        case 0xEC:
        case 0xE4:
            VAL = FETCH(WBACK_ADDR);
        case 0xE0:
            TEMP = IND_X - VAL;
            STAT = (IND_X >= VAL) ? (STAT | 0x01) : (STAT & 0xFE);
            STAT = (IND_X == VAL) ? (STAT | 0x02) : (STAT & 0xFD);
            STAT = (TEMP & 0x80) ? (STAT | 0x80) : (STAT & 0x7F);
            break;
        //CPY
        case 0xCC:
        case 0xC4:
            VAL = FETCH(WBACK_ADDR);
        case 0xC0:
            TEMP = IND_Y - VAL;
            STAT = (IND_Y >= VAL) ? (STAT | 0x01) : (STAT & 0xFE);
            STAT = (IND_Y == VAL) ? (STAT | 0x02) : (STAT & 0xFD);
            STAT = (TEMP & 0x80) ? (STAT | 0x80) : (STAT & 0x7F);
            break;
        //DEC
        case 0xDE:
            WAIT();
        case 0xD6:
        case 0xCE:
        case 0xC6:
            VAL = FETCH(WBACK_ADDR);
            WAIT();
            WAIT();
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
        case 0x5D:
        case 0x4D:
        case 0x51:
        case 0x55:
        case 0x59:
            VAL = FETCH(WBACK_ADDR);
        case 0x49:
            ACC ^= VAL;
            REG_P = &ACC;
            break;
        //INC
        case 0xFE:
            WAIT();
        case 0xEE:
        case 0xE6:
        case 0xF6:
            VAL = FETCH(WBACK_ADDR);
            WAIT();
            WAIT();
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
        case 0xBD:
        case 0xAD:
        case 0xB1:
        case 0xB5:
        case 0xB9:
            VAL = FETCH(WBACK_ADDR);
        case 0xA9:
            ACC = VAL;
            REG_P = &ACC;
            break;
        //LDX
        case 0xBE:
        case 0xA6:
        case 0xAE:
        case 0xB6:
            VAL = FETCH(WBACK_ADDR);
        case 0xA2:
            IND_X = VAL;
            REG_P = &IND_X;
            break;
        //LDY
        case 0xBC:
        case 0xA4:
        case 0xB4:
        case 0xAC:
            VAL = FETCH(WBACK_ADDR);
        case 0xA0:
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
            WAIT();
        case 0x4E:
        case 0x46:
        case 0x56:
            VAL = FETCH(WBACK_ADDR);
            WAIT();
            WAIT();
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
        case 0x1D:
        case 0x0D:
        case 0x11:
        case 0x15:
        case 0x19:
            VAL = FETCH(WBACK_ADDR);
        case 0x09:
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
            WAIT();
        case 0x2E:
        case 0x26:
        case 0x36:
            VAL = FETCH(WBACK_ADDR);
            WAIT();
            WAIT();
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
            WAIT();
        case 0x6E:
        case 0x66:
        case 0x76:
            VAL = FETCH(WBACK_ADDR);
            WAIT();
            WAIT();
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
        case 0xFD:
        case 0xED:
        case 0xF1:
        case 0xF5:
        case 0xF9:
            VAL = FETCH(WBACK_ADDR);
        case 0xE9:
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
            WAIT();
        case 0x81:
        case 0x85:
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
    WAIT();
}

//Branching is signed!!!
//If FLAG equals VAL then take the branch, another 'oops' cycle here
void CPU::BRANCH(char FLAG, char VAL) {

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
    /*LOG_STREAM << "FLAG: ";
    LOG_STREAM << std::hex << int(FLAG);
    LOG_STREAM << "VAL: ";
    LOG_STREAM << std::hex << int(VAL);*/
    int8_t OPRAND = FETCH(PROG_CNT++, true);
    WAIT();
    
    if (FLAG == VAL) {
        PROG_CNT += OPRAND;
        WAIT();
    }
    
    WAIT();
}