#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <fstream>

#define PRG_BANK_SIZE 16*1024
#define CHR_BANK_SIZE 4*1024

class Mapper {
    public:
        Mapper() {}
        Mapper(uint16_t P, uint16_t C): PRG_BANKS(P), CHR_BANKS(C), Irq(false), Counter(0) {}
        virtual uint32_t CPU_READ(uint16_t ADDR) = 0;
        virtual uint32_t PPU_READ(uint16_t ADDR, bool NT) = 0;
        virtual void CPU_WRITE(uint16_t ADDR, uint8_t VAL) = 0;
        virtual void PPU_WRITE(uint16_t ADDR) = 0;
        virtual void Scanline() { }

        uint8_t Counter;
        bool Irq;
    protected:
        uint8_t PRG_BANKS;  //In 16KB
        uint8_t CHR_BANKS;  //In 8KB

        enum MirrorMode {SingleLower, SingleUpper, Vertical, Horizontal} NT_MIRROR;

        //Right now the value 1 is vertical and the value 0 is horizontal, should use an enum instead for clarity
        uint16_t SelectNameTable(uint16_t ADDR, MirrorMode M) {

            switch (M) {
                case Vertical:
                    return (ADDR > 0x27FF) ? (ADDR % 0x2800) : (ADDR % 0x2000);
                    break;
                case Horizontal:
                    if ((ADDR > 0x23FF && ADDR < 0x2800) || (ADDR > 0x27FF && ADDR < 0x2C00))
                        return (ADDR - 0x0400) % 0x2000;
                    else if (ADDR > 0x2BFF)
                        return (ADDR - 0x0800) % 0x2000;
                    else
                        return (ADDR % 0x2000);
                    break;
                case SingleLower:
                    if (ADDR > 0x2BFF)
                        return ((ADDR - 0x0C00) % 0x2000);
                    else if (ADDR > 0x27FF)
                        return ((ADDR - 0x0800) % 0x2000);
                    else if (ADDR > 0x23FF)
                        return ((ADDR - 0x0400) % 0x2000);
                    else
                        return (ADDR % 0x2000);
                    break;
                case SingleUpper:
                    if (ADDR < 0x2400)
                        return ((ADDR + 0x0400) % 0x2000);
                    else if (ADDR > 0x27FF && ADDR < 0x2C00)
                        return ((ADDR - 0x0400) % 0x2000);
                    else if (ADDR > 0x2BFF)
                        return ((ADDR - 0x0800) % 0x2000);
                    else
                        return (ADDR % 0x2000);
                    break;
            }

        }

};

#endif