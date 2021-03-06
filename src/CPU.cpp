//Logging won't always be enabled so calls to FETCH will have a bool extracted from the command line
//Branch instructions can have an oops cycle as well, still need to implement this
//Reconsider using local over global vars for val, temp, low, high....

#include "CPU.hpp"

#include <sstream> //Only include if logging enabled
#include "6502Mnemonics.hpp" //Only if logging enabled


extern bool pause;
extern bool Gamelog;
extern bool quit;
extern long CPUCycleCount;
extern SDL_mutex* CpuPpuMutex;

uint8_t VAL, TEMP, LOW, HIGH, POINT;
uint16_t WBACK_ADDR;
std::ofstream LOG;
std::string LOG_LINE;
std::stringstream LOG_STREAM;



inline void CPU::STACK_PUSH(uint8_t BYTE) {
    RAM[0x0100 + STCK_PNT--] = BYTE;   
}


inline unsigned char CPU::STACK_POP() {
    return RAM[0x0100 + ++STCK_PNT];
}


//Fetch is called a lot by the CPU, needs to be as efficient as possible
uint8_t CPU::FETCH(uint16_t ADDR, bool SAVE) {

    //0x2008 -- 0x3FFF mirrors 0x2000 -- 0x2007 every 8 bytes
    if (ADDR > 0x1FFF && ADDR < 0x4000)
        return P->REG_READ((ADDR & 0x2007) % 0x2000, CPUCycleCount);
    
    //Every 0x0800 bytes of 0x0800 -- 0x1FFF mirrors 0x0000 -- 0x07FF
    if (ADDR < 0x2000)
        return RAM[ADDR & 0x07FF];

    if (ADDR == 0x4015)
        return A->Reg_Read();

    //Controller probe
    if (ADDR == 0x4016) {
        uint8_t TEMP = (CONTROLLER1 & 0x80) >> 7;
        CONTROLLER1 <<= 1;
        return TEMP;
    }


    if ((ADDR > 0x401F) && (ADDR <= 0xFFFF))
        return ROM->CPU_ACCESS(ADDR);

    return 0;
}


void CPU::WRITE(uint8_t VAL, uint16_t ADDR, uint8_t cyc) {
    
    if (ADDR < 0x2000) {
        RAM[ADDR & 0x07FF] = VAL;
        return;
    }

    //PPU registers - WRITE_BUF stores VAL in the lower 8 bits and a number representing the register to write to
    //in the 8th bit. Send cyc to allow PPU to track mid-frame state changes
    if ((ADDR >= 0x2000) && (ADDR <= 0x3FFF)) {
        P->REG_WRITE(VAL, (ADDR & 0x2007) % 0x2000, (cyc + CPUCycleCount)*3);
        return;
    }
        

    //CPU is suspended during OAM DMA transfer, 513 or 514 cycles, need some way to determine odd vs even cpu cycles
    //transfer begins at current OAM write address. Send cyc here as above
    if (ADDR == 0x4014) {
        P->OAMDMA = VAL;
        WAIT();  
        for (uint16_t i=0; i<256; i++) {
            P->REG_WRITE(FETCH((VAL << 8) + i), 4, (cyc + CPUCycleCount)*3);
        }
        WAIT(513);
        return;
    }

    //Controller probe
    if (ADDR == 0x4016) {
        probe = (VAL & 0x01);
        return;
    }

    //APU registers
    if ((ADDR >= 0x4000) && (ADDR <= 0x4017)) {
        A->Reg_Write(ADDR, VAL);
        return;
    }


    if ((ADDR >= 0x4020) && (ADDR <= 0xFFFF))
        ROM->CPU_ACCESS(ADDR, VAL, false);
    
}


void CPU::WAIT(uint16_t N) {
    while (pause)
        std::this_thread::yield();

    ++N;
    while (--N > 0) {
        while (P->cycleCount < 3)
            std::this_thread::yield();    

        SDL_LockMutex(CpuPpuMutex); //Locking helps prevent data races involving cycleCount which could lead to inaccurate timing
        P->cycleCount -= 3;         //between CPU and PPU, BUT... Locking involves heavy performance cost, especially when called this
        SDL_UnlockMutex(CpuPpuMutex);//often.
    
        ++CPUCycleCount;

        if (A->Pulse_Out()) {
            //DMC is reading memory, stall CPU for up to 4 cycles
            //Specific number dependent on CPU operation currently in progress, just use 4 for now
            N += 4;
        }

    }

}


void CPU::RESET() {

    CPUCycleCount = 0;
    STCK_PNT -= 3;
    STAT &= 0xEF;
    PROG_CNT = ((FETCH(0xFFFD) << 8) | FETCH(0xFFFC));
    STAT |= 0x04;
    A->FrameCount = 0;
    
    WAIT(7); 
}


void CPU::IRQ_NMI(uint16_t V) {

    STACK_PUSH(PROG_CNT >> 8);
    STACK_PUSH(PROG_CNT & 0x00FF);
    
    STAT &= 0xEF;
    STACK_PUSH(STAT);
    
    PROG_CNT = ((FETCH(V+1) << 8) | (FETCH(V)));
    STAT |= 0x04;

    WAIT(7);
}


void CPU::RUN() {
    RAM.fill(0); //Clear RAM
    bool unofficial = false;
    uint8_t cycleCount;
    const uint8_t *keyboard = SDL_GetKeyboardState(NULL);
    
    //Enable logging
    LOG.open("CPU_LOG.txt", std::ios::trunc | std::ios::out);
    
    //Reset bool
    bool R = true;

    //First cycle has started
    uint8_t CODE;
    
    //Main loop
    while (true) {
        if (R || keyboard[SDL_SCANCODE_R]) { //Currently, pressing "r" key triggers a reset
            P->Reset = true;
            RESET();
            R = false;
        } else if (P->GEN_NMI && !P->NmiDelay) {
            P->GEN_NMI = 0;
            IRQ_NMI(0xFFFA);
        } else if (A->FireIRQ && !(STAT & 0x04) && !IRQDelay) {
            IRQ_NMI(0xFFFE);
            A->FireIRQ--;
        } else if (IRQPend) {
            IRQ_NMI(0xFFFE);
            IRQPend = false;
        } else if (*(ROM->FireIrq) && !(STAT & 0x04)) {
            *(ROM->FireIrq) = false;
            IRQ_NMI(0xFFFE);
        }

        IRQDelay = (IRQDelay) ? false : IRQDelay;
        P->NmiDelay = (P->NmiDelay) ? false : P->NmiDelay;

        if (Gamelog)
            LOG << std::hex << PROG_CNT << ": ";
        CODE = FETCH(PROG_CNT++);
        cycleCount = 1;


        
        //Compose a string and append after EXEC
        if (Gamelog) {
            LOG_STREAM.str(std::string());
            LOG << std::hex << int(CODE) << " ";
            LOG_STREAM << "ACC:" << std::hex << int(ACC) << " ";
            LOG_STREAM << "X:" << std::hex << int(IND_X) << " ";
            LOG_STREAM << "Y:" << std::hex << int(IND_Y) << " ";
            LOG_STREAM << "STAT:" << std::hex << int(STAT) << " ";
            LOG_STREAM << "SP:" << std::hex << int(STCK_PNT);
        }

        //Stack Access Instructions and JMP
        switch (CODE) {
            //BRK - This causes an interrupt
            case 0x00:
                PROG_CNT++;

                STACK_PUSH(PROG_CNT >> 8);
                STACK_PUSH(PROG_CNT & 0x00FF);
                
                STAT |= 0x10;
                STACK_PUSH(STAT | 0x30);
                
                PROG_CNT = ((FETCH(0xFFFF) << 8) | FETCH(0xFFFE));
                STAT |= 0x04;
 
                cycleCount += 6;
                break;
            //RTI
            case 0x40:
                STAT = STACK_POP();
                
                PROG_CNT = (0 | STACK_POP());
                PROG_CNT |= (STACK_POP() << 8);
                IRQPend = (A->FireIRQ && !(STAT & 0x04));
                cycleCount += 5;
                break;
            //RTS
            case 0x60:
                PROG_CNT = (0 | STACK_POP());
                PROG_CNT |= (STACK_POP() << 8);
                
                PROG_CNT++;
                cycleCount += 5;
                break;
            //PHA
            case 0x48:
                STACK_PUSH(ACC);
                cycleCount += 2;
                break;
            //PHP
            case 0x08:
                STACK_PUSH(STAT | 0x30);
                cycleCount += 2;
                break;
            //PLA
            case 0x68:
                ACC = STACK_POP();
                STAT = (ACC == 0) ? (STAT | 0x02) : (STAT & 0xFD);
                STAT = ((ACC & 0x80) > 0) ? (STAT | 0x80) : (STAT & 0x7F); 

                cycleCount += 3;
                break;
            //PLP
            case 0x28:
                IRQPend = (A->FireIRQ && !(STAT & 0x04));
                STAT = STACK_POP();
                IRQDelay = true;
                cycleCount += 3;
                break;
            //JSR
            case 0x20:
                LOW = FETCH(PROG_CNT++, true);
               
                STACK_PUSH(PROG_CNT >> 8);
                STACK_PUSH(PROG_CNT & 0x00FF);
            
                HIGH = FETCH(PROG_CNT, true);
                PROG_CNT = ((HIGH << 8) | LOW);
               
                cycleCount += 5;
                break;
            //JMP - Absolute
            case 0x4C:
                LOW = FETCH(PROG_CNT++, true);
                HIGH = FETCH(PROG_CNT, true);
                
                PROG_CNT = ((HIGH << 8) | LOW);
                
                cycleCount += 2;
                break;
            //JMP - Indirect Absolute
            case 0x6C:
                LOW = FETCH(PROG_CNT++, true);
                HIGH = FETCH(PROG_CNT++, true);
                
                PROG_CNT &= 0;
                PROG_CNT |= FETCH(((HIGH << 8) | LOW), true);
                if ((LOW & 0xFF) == 0xFF)
                    PROG_CNT |= (FETCH(HIGH << 8, true) << 8);
                else
                    PROG_CNT |= (FETCH(((HIGH << 8) | LOW) + 1) << 8);
                
                cycleCount += 4;
                break;
            default:
                if ((CODE & 0x0F) == 0x0A || (CODE & 0x0F) == 0x08) {
                    cycleCount += EXEC(CODE, 8);
                    break;
                }

                if ((CODE & 0x1F) == 0x10) {
                    cycleCount += BRANCH(CODE >> 6, (CODE >> 5) & 0x01);
                    break;
                }    

                switch (CODE & 0x03) {  //Extract bits 0,1
                    case 0:
                        cycleCount += EXEC(CODE, (CODE & 0x1C) >> 2);
                        break;
                    case 1:
                        switch ((CODE & 0x1C) >> 2) {
                            case 0:
                                cycleCount += EXEC(CODE, 2);
                                break;
                            case 2:
                                cycleCount += EXEC(CODE, 0);
                                break;
                            default:
                                cycleCount += EXEC(CODE, (CODE & 0x1C) >> 2);
                        }
                        break;
                    case 2:
                        if (CODE != 0xBE)
                            cycleCount += EXEC(CODE, (CODE & 0x1C) >> 2);
                        else
                            cycleCount += EXEC(CODE, 6);
                        break;
                    default:
                        unofficial = true;
                        break;
                }
        }
        if (unofficial) {
            std::cout << "Unofficial opcode : " << std::hex << int(CODE) << " at: " << PROG_CNT << "\n";
            break;
        }
        //At this point, all cycles of the instruction have been executed

        //Controller polling if probe is on
        //Mapping of keys to buttons is arbitrary at this point just to get things working
        if (probe) {
            CONTROLLER1 = 0x00;
            CONTROLLER1 = (keyboard[SDL_SCANCODE_A] << 7) | 
            (keyboard[SDL_SCANCODE_S] << 6) | 
            (keyboard[SDL_SCANCODE_RSHIFT] << 5) | 
            (keyboard[SDL_SCANCODE_RETURN] << 4) |
            (keyboard[SDL_SCANCODE_UP] << 3) |
            (keyboard[SDL_SCANCODE_DOWN] << 2) | 
            (keyboard[SDL_SCANCODE_LEFT] << 1) |
            keyboard[SDL_SCANCODE_RIGHT]; 
        }

        WAIT(cycleCount);
        if (Gamelog) {
            LOG << OPCODES[CODE] << "\t\t" << LOG_STREAM.str() << '\n';
        }

        if (quit)
            break;
    }
}


uint8_t CPU::EXEC(uint8_t OP, char ADDR_TYPE) {
  
    uint8_t *REG_P = nullptr;
    uint8_t cycles = 0;

    uint16_t S_TEMP = 0;
    uint16_t S_VAL = 0;
    
    //Create an enum for ADDR_TYPE instead of having random numbers
    switch (ADDR_TYPE) {
        //Immediate
        case 0:
            VAL = FETCH(PROG_CNT++, true);
            break;
        //Zero-Page
        case 1:
            //Fetch the addresss
            VAL = FETCH(PROG_CNT++, true);
            WBACK_ADDR = (0x0000 | VAL);
            cycles++;
            break;
        //Indexed-Indirect
        case 2:
            POINT = FETCH(PROG_CNT++, true);
            WBACK_ADDR = (0 | FETCH((POINT + IND_X) % 256, true));
            WBACK_ADDR |= (FETCH((POINT + IND_X + 1) % 256, true) << 8);
            cycles += 4;
            break;
        //Absolute
        case 3:
            LOW = FETCH(PROG_CNT++, true);
            HIGH = FETCH(PROG_CNT++, true);
            WBACK_ADDR = (LOW | (HIGH << 8));
            cycles += 2;
            break;
        //Indirect-Indexed - Potential 'oops' cycle, only for read instructions, here
        case 4:
            POINT = FETCH(PROG_CNT++, true);
            LOW = FETCH(POINT);
            HIGH = FETCH((POINT + 1) % 256);

            //'oops' cycle
            if ((OP != 0x91) && ((LOW + IND_Y) > 255))
                cycles++;
            
            WBACK_ADDR = ((LOW | (HIGH << 8)) + IND_Y);
            cycles += 3;
            break;
        //Zero-Page Indexed
        case 5:
            VAL = FETCH(PROG_CNT++, true);
            WBACK_ADDR = (OP != 0xB6 && OP != 0x96) ? (VAL + IND_X) % 256 : (VAL + IND_Y) % 256;
            cycles += 2;
            break;
        //Absolute Indexed - Potential 'oops' cycle, only for read instructions, here
        case 6:
        case 7: {
            WBACK_ADDR = (0 | FETCH(PROG_CNT++, true));
            WBACK_ADDR |= (FETCH(PROG_CNT++, true) << 8);
            
            //'Oops' cycle
            bool oops = (((OP & 0xF0) != 0x90) && (((OP & 0x0F) == 0x0D) || ((OP & 0x0F) == 0x09))) || (OP == 0xBC) || (OP == 0xBE);
            if (oops && ((ADDR_TYPE == 6 && (LOW + IND_X) > 255) || (ADDR_TYPE == 7 && (LOW + IND_Y) > 255)))
                cycles++;
              
            WBACK_ADDR += (ADDR_TYPE == 7) ? IND_X : IND_Y;
            cycles += 2;
            break; }
        //Accumulator/Implied - No timed waits needed
        default:
            break;
    }

    
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
            cycles++;
        case 0x06:
        case 0x16:
        case 0x0E:
            cycles += 2;
            VAL = FETCH(WBACK_ADDR);
            STAT &= 0xFE;
            STAT |= (VAL >> 7);
            VAL = VAL << 1;
            WRITE(VAL, WBACK_ADDR, cycles+1); //W_BACK = true;
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
            else
                STAT &= 0xFD;
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
            IRQDelay = true;
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
            cycles++;
        case 0xD6:
        case 0xCE:
        case 0xC6:
            VAL = FETCH(WBACK_ADDR) - 1;
            cycles += 2;
            WRITE(VAL, WBACK_ADDR, cycles+1); //W_BACK = true;
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
            cycles++;
        case 0xEE:
        case 0xF6:
        case 0xE6:
            VAL = FETCH(WBACK_ADDR) + 1;
            cycles += 2;
            WRITE(VAL, WBACK_ADDR, cycles+1);
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
            cycles++;
        case 0x4E:
        case 0x56:
        case 0x46:
            VAL = FETCH(WBACK_ADDR);
            cycles += 2;
            STAT &= 0xFE;
            STAT |= (0x01 & VAL);
            VAL = VAL >> 1;
            WRITE(VAL, WBACK_ADDR, cycles+1); //W_BACK = true;
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
            cycles++;
        case 0x2E:
        case 0x36:   
        case 0x26:
            VAL = FETCH(WBACK_ADDR);
            cycles += 2;
            TEMP = VAL << 1;
            TEMP |= (0x01 & STAT);
            STAT &= 0xFE;
            STAT |= ((0x80 & VAL) >> 7);
            VAL = TEMP;
            WRITE(VAL, WBACK_ADDR, cycles+1); //W_BACK = true;
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
            cycles++;
        case 0x6E:
        case 0x76:
        case 0x66:
            VAL = FETCH(WBACK_ADDR);
            cycles += 2;
            TEMP = STAT;
            STAT &= 0xFE;
            STAT |= (0x01 & VAL);
            VAL = VAL >> 1;
            VAL |= ((TEMP & 0x01) << 7);
            WRITE(VAL, WBACK_ADDR, cycles+1); //W_BACK = true;
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
            S_VAL = uint16_t(VAL) ^ 0xFF;
            S_TEMP = uint16_t(ACC) + S_VAL + (STAT & 0x01);
            STAT &= 0xBE; //Clear V and C flags
            STAT |= (S_TEMP & 0xFF00) ? 1 : 0;
            STAT |= ((S_TEMP ^ uint16_t(ACC)) & (S_TEMP ^ S_VAL) & 0x0080) ? 0x40 : 0;

            ACC = S_TEMP & 0xFF;
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
            IRQPend = (A->FireIRQ && !(STAT & 0x04));
            STAT |= 0x04;
            IRQDelay = true;
            break;
        //STA
        case 0x91:
        case 0x9D:
        case 0x99:
            cycles++;
        case 0x81:
        case 0x85:
        case 0x8D:
        case 0x95:
            WRITE(ACC, WBACK_ADDR, cycles+1); //W_BACK = true;
            break;
        //STX
        case 0x86:
        case 0x8E:
        case 0x96:
            WRITE(IND_X, WBACK_ADDR, cycles+1);
            break;
        //STY
        case 0x84:
        case 0x94:
        case 0x8C:
            WRITE(IND_Y, WBACK_ADDR, cycles+1);
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
   
    //Examine and change status flags if needed
    if (REG_P) {
        STAT = (*REG_P == 0) ?  (STAT | 0x02) : (STAT & 0xFD);
        STAT = ((*REG_P & 0x80) > 0) ?  (STAT | 0x80) : (STAT & 0x7F); 
    }
    
    //Plus one to wait out the last cycle of most instructions
    return (cycles + 1);
}

//Branching is signed!!!
//If FLAG equals VAL then take the branch, another 'oops' cycle here
uint8_t CPU::BRANCH(char FLAG, char VAL) {
    uint8_t cycles = 1;

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
    
    int8_t OPRAND = FETCH(PROG_CNT++, true);
    
    if (FLAG == VAL) {
        if (((PROG_CNT + OPRAND) & 0x0F00) != (PROG_CNT & 0x0F00))
            cycles++; //This should be executed if branch occurs to a new page

        PROG_CNT += OPRAND;
        cycles++;
    }

    
    return cycles;
}