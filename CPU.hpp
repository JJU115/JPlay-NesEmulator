

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
    return 0;
}