#include "MMC3.hpp"



inline uint32_t MMC3::CPU_READ(uint16_t ADDR) {
    if ((ADDR >= 0x6000) && (ADDR <= 0x7FFF))
        return PRG_RAM[ADDR % 0x6000];
   
    return PrgBanks[(ADDR - 0x8000) / 0x2000] * (PRG_BANK_SIZE / 2) + (ADDR % 0x2000);

}


//For nametable fetches, need to consider what happens when 4-screen VRAM is dictated by the iNES header thus ignoring
//the mirroring register. Only one game for MMC3 does this however - Rad Racer 2
inline uint32_t MMC3::PPU_READ(uint16_t ADDR, bool NT) {

    if (NT)
        return SelectNameTable(ADDR, Mirroring);

    return (ChrBanks[ADDR / 0x0400] * 1024 + (ADDR % 0x0400));
}


//Mesen performs an inversion right after the 2nd even register write
void MMC3::CPU_WRITE(uint16_t ADDR, uint8_t VAL) {

     if ((ADDR >= 0x6000) && (ADDR <= 0x7FFF)) {
        PRG_RAM[ADDR % 0x6000] = VAL;
        return;
    }

    switch (ADDR & 0xE000) {
        case 0x8000:
            if (ADDR & 1) {
                //Odd       
                switch (TargetBank) {
                    case 0:
                    case 1:
                        ChrBanks[ChrMode*4 + TargetBank*2] = (VAL & 0xFE);
                        ChrBanks[ChrMode*4 + TargetBank*2 + 1] = ((VAL & 0xFE) + 1);
                        break;
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        ChrBanks[(!ChrMode)*4 + (TargetBank - 2)] = VAL;
                        break;                 
                    case 6:
                        PrgBanks[PrgMode*2] = (VAL & 0x3F) % PRG_BANKS;
                        break;
                    case 7:
                        PrgBanks[1] = (VAL & 0x3F) % PRG_BANKS;
                        break;

                }
                
            } else {
                //Even
                TargetBank = VAL & 0x07;
                if (PrgMode ^ bool(VAL & 0x40)) {
                    PrgBanks[0] ^= PrgBanks[2];
                    PrgBanks[2] ^= PrgBanks[0];
                    PrgBanks[0] ^= PrgBanks[2];
                }
                
                if (ChrMode ^ bool(VAL & 0x80)) {
                    for (int i=0; i<4; i++) {
                        ChrBanks[i] ^= ChrBanks[i+4];
                        ChrBanks[i+4] ^= ChrBanks[i];
                        ChrBanks[i] ^= ChrBanks[i+4];
                    }
                }

                ChrMode = VAL & 0x80;
                PrgMode = VAL & 0x40;
            }
            break;
        case 0xA000:
            if ((ADDR % 2) == 0)
                Mirroring = (VAL & 0x01) ? Horizontal : Vertical;
            break;
        case 0xC000:
            if (ADDR & 1) {
                IrqReload = true;
                Counter = 0;
            } else
                IrqLatch = VAL;
               
            break;
        case 0xE000:
            IrqEnable = (ADDR & 1);
            Irq = (IrqEnable) ? Irq : false;
            break;
    }


}


void MMC3::PPU_WRITE(uint16_t ADDR) {

}


void MMC3::Scanline() {

    if (IrqReload) {
        Counter = IrqLatch;
        IrqReload = false;
        Irq = (Counter == 0);
    } else if (Counter == 0) {
        Counter = IrqLatch;
        Irq = IrqEnable && (!Set);
        Set = Irq;
    } else if (--Counter == 0) {
        Irq = IrqEnable;
    }
}