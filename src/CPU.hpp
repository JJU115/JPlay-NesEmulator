
class CPU {
    public:
        void RUN();
        char PULSE(char MODE);
        CPU():ACC(0), IND_X(0), IND_Y(0), STAT(0x34), STCK_PNT(0xFD) {}
    private:
        char RAM[2048];
        char ACC, IND_X, IND_Y, STAT, STCK_PNT;
        short PROG_CNT;
};


void CPU::RUN() {

}


char CPU::PULSE(char MODE) {

    //Stack Access Instructions and JMP
    switch (MODE) {
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
    }
    return 0;
}