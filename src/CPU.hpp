#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <chrono>
#include <thread>
#include <SDL_keyboard.h>
#include "PPU.hpp"
#include "APU.hpp"
#include <mutex>
#include <condition_variable>


enum AddrMode {IMP, ABS, ZERO_PG, ZERO_PG_X, ABS_X, IMMED, ABS_Y, INDR_X, INDR_Y, REL, ACCM, ZERO_PG_Y, INDR};

class CPU {
    typedef void(CPU::*Instr)();
    public:
        void RUN();
        void Start();
        CPU(Cartridge& NES, PPU& P1, APU& A1):ACC(0), IND_X(0), IND_Y(0), STAT(0x34), STCK_PNT(0xFD), PROG_CNT(0xFFFC), 
        CONTROLLER1(0), CONTROLLER2(0), probe(false), IRQDelay(false), IRQPend(false), ROM(&NES), P(&P1), A(&A1) { 
            keyboard = SDL_GetKeyboardState(NULL); 
            
            ModeLookup = {IMP, INDR_X, IMP, IMP, IMP, ZERO_PG, ZERO_PG, IMP, IMP, IMMED, ACCM, IMP, IMP, ABS, ABS, IMP,
                        REL, INDR_Y, IMP, IMP, IMP, ZERO_PG_X, ZERO_PG_X, IMP, IMP, ABS_Y, IMP, IMP, IMP, ABS_X, ABS_X, IMP,
                        ABS, INDR_X, IMP, IMP, ZERO_PG, ZERO_PG, ZERO_PG, IMP, IMP, IMMED, ACCM, IMP, ABS, ABS, ABS, IMP,
                        REL, INDR_Y, IMP, IMP, IMP, ZERO_PG_X, ZERO_PG_X, IMP, IMP, ABS_Y, ACCM, IMP, IMP, ABS_X, ABS_X, IMP,
                        IMP, INDR_X, IMP, IMP, IMP, ZERO_PG, ZERO_PG, IMP, IMP, IMMED, ACCM, IMP, ABS, ABS, ABS, IMP,
                        REL, INDR_Y, IMP, IMP, IMP, ZERO_PG_X, ZERO_PG_X, IMP, IMP, ABS_Y, ACCM, IMP, IMP, ABS_X, ABS_X, IMP,
                        IMP, INDR_X, IMP, IMP, IMP, ZERO_PG, ZERO_PG, IMP, IMP, IMMED, ACCM, IMP, INDR, ABS, ABS, IMP,
                        REL, INDR_Y, IMP, IMP, IMP, ZERO_PG_X, ZERO_PG_X, IMP, IMP, ABS_Y, ACCM, IMP, IMP, ABS_X, ABS_X, IMP,
                        IMP, INDR_X, IMP, IMP, ZERO_PG, ZERO_PG, ZERO_PG, IMP, IMP, IMMED, IMP, IMP, ABS, ABS, ABS, IMP,
                        REL, INDR_Y, IMP, IMP, ZERO_PG_X, ZERO_PG_X, ZERO_PG_Y, IMP, IMP, ABS_Y, IMP, IMP, IMP, ABS_X, ABS_X, IMP,
                        IMMED, INDR_X, IMMED, IMP, ZERO_PG, ZERO_PG, ZERO_PG, IMP, IMP, IMMED, IMP, IMP, ABS, ABS, ABS, IMP,
                        REL, INDR_Y, IMP, IMP, ZERO_PG_X, ZERO_PG_X, ZERO_PG_Y, IMP, IMP, ABS_Y, IMP, IMP, ABS_X, ABS_X, ABS_Y, IMP,
                        IMMED, INDR_X, IMP, IMP, ZERO_PG, ZERO_PG, ZERO_PG, IMP, IMP, IMMED, IMP, IMP, ABS, ABS, ABS, IMP,
                        REL, INDR_Y, IMP, IMP, IMP, ZERO_PG_X, ZERO_PG_X, IMP, IMP, ABS_Y, ACCM, IMP, IMP, ABS_X, ABS_X, IMP,
                        IMMED, INDR_X, IMP, IMP, ZERO_PG, ZERO_PG, ZERO_PG, IMP, IMP, IMMED, IMP, IMP, ABS, ABS, ABS, IMP,
                        REL, INDR_Y, IMP, IMP, IMP, ZERO_PG_X, ZERO_PG_X, IMP, IMP, ABS_Y, ACCM, IMP, IMP, ABS_X, ABS_X, IMP};

            InstructionLookup = {&CPU::BRK, &CPU::ORA, NULL, NULL, NULL, &CPU::ORA, &CPU::ASL, NULL, &CPU::PHP, &CPU::ORA, &CPU::ASL, NULL, NULL, &CPU::ORA, &CPU::ASL, NULL,
                                &CPU::BPL, &CPU::ORA, NULL, NULL, NULL, &CPU::ORA, &CPU::ASL, NULL, &CPU::CLC, &CPU::ORA, NULL, NULL, NULL, &CPU::ORA, &CPU::ASL, NULL,
                                &CPU::JSR, &CPU::AND, NULL, NULL, &CPU::BIT, &CPU::AND, &CPU::ROL, NULL, &CPU::PLP, &CPU::AND, &CPU::ROL, NULL, &CPU::BIT, &CPU::AND, &CPU::ROL, NULL,
                                &CPU::BMI, &CPU::AND, NULL, NULL, NULL, &CPU::AND, &CPU::ROL, NULL, &CPU::SEC, &CPU::AND, NULL, NULL, NULL, &CPU::AND, &CPU::ROL, NULL,
                                &CPU::RTI, &CPU::EOR, NULL, NULL, NULL, &CPU::EOR, &CPU::LSR, NULL, &CPU::PHA, &CPU::EOR, &CPU::LSR, NULL, &CPU::JMP_Abs, &CPU::EOR, &CPU::LSR, NULL,
                                &CPU::BVC, &CPU::EOR, NULL, NULL, NULL, &CPU::EOR, &CPU::LSR, NULL, &CPU::CLI, &CPU::EOR, NULL, NULL, NULL, &CPU::EOR, &CPU::LSR, NULL,
                                &CPU::RTS, &CPU::ADC, NULL, NULL, NULL, &CPU::ADC, &CPU::ROR, NULL, &CPU::PLA, &CPU::ADC, &CPU::ROR, NULL, &CPU::JMP_IndAbs, &CPU::ADC, &CPU::ROR, NULL,
                                &CPU::BVS, &CPU::ADC, NULL, NULL, NULL, &CPU::ADC, &CPU::ROR, NULL, &CPU::SEI, &CPU::ADC, NULL, NULL, NULL, &CPU::ADC, &CPU::ROR, NULL,
                                NULL, &CPU::STA, NULL, NULL, &CPU::STY, &CPU::STA, &CPU::STX, NULL, &CPU::DEY, NULL, &CPU::TXA, NULL, &CPU::STY, &CPU::STA, &CPU::STX, NULL,
                                &CPU::BCC, &CPU::STA, NULL, NULL, &CPU::STY, &CPU::STA, &CPU::STX, NULL, &CPU::TYA, &CPU::STA, &CPU::TXS, NULL, NULL, &CPU::STA, NULL, NULL,
                                &CPU::LDY, &CPU::LDA, &CPU::LDX, NULL, &CPU::LDY, &CPU::LDA, &CPU::LDX, NULL, &CPU::TAY, &CPU::LDA, &CPU::TAX, NULL, &CPU::LDY, &CPU::LDA, &CPU::LDX, NULL,
                                &CPU::BCS, &CPU::LDA, NULL, NULL, &CPU::LDY, &CPU::LDA, &CPU::LDX, NULL, &CPU::CLV, &CPU::LDA, &CPU::TSX, NULL, &CPU::LDY, &CPU::LDA, &CPU::LDX, NULL,
                                &CPU::CPY, &CPU::CMP, NULL, NULL, &CPU::CPY, &CPU::CMP, &CPU::DEC, NULL, &CPU::INY, &CPU::CMP, &CPU::DEX, NULL, &CPU::CPY, &CPU::CMP, &CPU::DEC, NULL,
                                &CPU::BNE, &CPU::CMP, NULL, NULL, NULL, &CPU::CMP, &CPU::DEC, NULL, &CPU::CLD, &CPU::CMP, NULL, NULL, NULL, &CPU::CMP, &CPU::DEC, NULL,
                                &CPU::CPX, &CPU::SBC, NULL, NULL, &CPU::CPX, &CPU::SBC, &CPU::INC, NULL, &CPU::INX, &CPU::SBC, &CPU::NOP, NULL, &CPU::CPX, &CPU::SBC, &CPU::INC, NULL,
                                &CPU::BEQ, &CPU::SBC, NULL, NULL, NULL, &CPU::SBC, &CPU::INC, NULL, &CPU::SED, &CPU::SBC, NULL, NULL, NULL, &CPU::SBC, &CPU::INC, NULL};

            Cycles = {7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
                        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
                        6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
                        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
                        6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
                        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
                        6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
                        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
                        2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
                        2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
                        2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
                        2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
                        2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
                        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
                        2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
                        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7};
            
            PageCrossCycles = {
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0};

            
        }


        
    private:
        uint8_t FETCH(uint16_t ADDR, bool SAVE=true);
        void WRITE(uint8_t VAL, uint16_t ADDR, uint8_t cyc=0);
        uint8_t EXEC(uint8_t OP, char ADDR_TYPE);
        uint8_t BRANCH(char FLAG, char VAL);
        void STACK_PUSH(uint8_t BYTE);
        unsigned char STACK_POP();
        void RESET();
        void IRQ_NMI(uint16_t V);
        void WAIT(uint16_t N=1);

        Cartridge *ROM;
        PPU *P;
        APU *A;

        std::array<uint8_t, 2048> RAM;
        uint8_t ACC, IND_X, IND_Y, STAT, STCK_PNT, CONTROLLER1, CONTROLLER2;
        uint16_t PROG_CNT;

        bool probe;
        bool IRQDelay;
        bool IRQPend;
        const uint8_t *keyboard;

        std::array<AddrMode, 256> ModeLookup;
        std::array<Instr, 256> InstructionLookup;
        std::array<uint8_t, 256> Cycles;
        std::array<bool, 256> PageCrossCycles;

        void ExamineStatus(uint8_t Reg);

        /* All the instructions */
        void ADC();
        void AND();
        void ASL();
        void BCC();
        void BCS();
        void BEQ();
        void BIT();
        void BMI();
        void BNE();
        void BPL();
        void BRK();
        void BVC();
        void BVS();
        void CLC();
        void CLD();
        void CLI();
        void CLV();
        void CMP();
        void CPX();
        void CPY();
        void DEC();
        void DEX();
        void DEY();
        void EOR();
        void INC();
        void INX();
        void INY();
        void JMP_Abs();
        void JMP_IndAbs();
        void JSR();
        void LDA();
        void LDX();
        void LDY();
        void LSR();
        void NOP();
        void ORA();
        void PHA();
        void PHP();
        void PLA();
        void PLP();
        void ROL();
        void ROR();
        void RTI();
        void RTS();
        void SBC();
        void SEC();
        void SED();
        void SEI();
        void STA();
        void STX();
        void STY();
        void TAX();
        void TAY();
        void TSX();
        void TXA();
        void TXS();
        void TYA();

};


#endif