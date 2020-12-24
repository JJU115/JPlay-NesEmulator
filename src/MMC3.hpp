#ifndef MMC3_H
#define MMC3_H


#include "Mapper.hpp"
#include <array>


//iNES mapper #4 - MMC3


class MMC3 : public Mapper {
    public:
        //Assuming IRQs are disabled at startup
        MMC3(uint8_t P, uint8_t C) : Mapper(2*P, 8*C), BankSelect(0), TargetBank(0), BankData(0), ChrInversion(false), PrgInversion(false),
        ChrMode(false), PrgMode(false), IrqLatch(0), IrqReload(false), IrqEnable(false), Set(false)
        { 
            PrgBanks[1] = PRG_BANKS - 2;
            PrgBanks[2] = PRG_BANKS - 2;
            PrgBanks[3] = PRG_BANKS - 1;

            for (int i=1; i<8; i++)
                ChrBanks[i] = i; 

            Irq = false;
            NT_MIRROR = Vertical;
        }
        uint32_t CPU_READ(uint16_t ADDR);
        uint32_t PPU_READ(uint16_t ADDR);
        void CPU_WRITE(uint16_t ADDR, uint8_t VAL);
        void PPU_WRITE(uint16_t ADDR);
        void Scanline();

    private:
        std::array<uint8_t, 8> ChrBanks; //Each index represents a 1KB bank going from $0000 to $1FFF
        std::array<uint8_t, 4> PrgBanks; //Each index is an 8KB PRG bank, last 8KB of PPU address space always fixed to last bank

        //Registers
        uint8_t BankSelect;
        uint8_t TargetBank;
        bool ChrInversion;
        bool PrgInversion;
        bool ChrMode;
        bool PrgMode;
        uint8_t BankData;
        uint8_t IrqLatch;

        //IRQ flags
        bool IrqReload;
        bool IrqEnable;
        bool Set;

        std::array<uint8_t, 8 * 1024> PRG_RAM; 



};



#endif