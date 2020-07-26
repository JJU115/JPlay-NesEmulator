#include <iostream>
#include "APU.hpp"

int APU_Run(void* data);


//Pulse channel sequence lookup - These are based on the output waveform values
//Lookup is: value & (0x80 >> sequence) where sequence counts 0 to 7 then loops 
uint8_t SequenceLookup[4] = {0x40, 0x60, 0x78, 0x9F};


//Function run on separate SDL thread
void APU::Channels_Out() {
    g_sampleNum = 0;
    want.callback = audio_callback;
    std::cout << "channel out\n";
    SDL_PauseAudio(0);
    while (true) {
        while (CpuCycles < 2)
            std::this_thread::yield();

        ApuCycles++; //% based on 4/5 step sequence
        ApuCycles %= (FrameCount & 0x80) ? 18641 : 14915;

        PulseOne_Out();
        CpuCycles -=2;
    }

}


void APU::PulseOne_Out() {

    //This whole section is probably useless
    /*if (PulseOne.timer == 0) {
        PulseOne.timer = (Pulse1TimeLow | ((Pulse1TimeHigh & 0x07) << 8));
        PulseOne.sequence = (PulseOne.sequence + 1) % 8;
    } else {
        PulseOne.timer--;
    }*/

    switch (ApuCycles) {
        case 3728:
            PulseOne.pulseEnvelope->clock();
            PulseTwo.pulseEnvelope->clock();
            break;
        case 7456:
            PulseOne.pulseEnvelope->clock();
            PulseOne.pulseSweep->clock();
            if ((PulseOne.lengthCount != 0) && !(Pulse1Control & 0x20)) {
                PulseOne.lengthCount--;
            }

            PulseTwo.pulseEnvelope->clock();
            PulseTwo.pulseSweep->clock();
            if ((PulseTwo.lengthCount != 0) && !(Pulse2Control & 0x20)) {
                PulseTwo.lengthCount--;
            }
            break;

        case 11185:
            PulseOne.pulseEnvelope->clock();
            PulseTwo.pulseEnvelope->clock();
            break;

        case 14914: //If 4-step
            if (!(FrameCount & 0xC0)) {
                FrameInterrupt = true;
                //Generate an IRQ
            }
            break;

        case 14915: //If 4-step
            if (!(FrameCount & 0x80)) {
                PulseOne.pulseSweep->clock();
                if ((PulseOne.lengthCount != 0) || !(Pulse1Control & 0x20)) {
                    PulseOne.lengthCount--;
                }

                PulseTwo.pulseSweep->clock();
                if ((PulseTwo.lengthCount != 0) || !(Pulse2Control & 0x20)) {
                    PulseTwo.lengthCount--;
                }
            }

            if (!(FrameCount & 0xC0)) {
                FrameInterrupt = true;
                //Generate an IRQ
            }
            break;

        case 18640: //If 5-step
            if (FrameCount & 0x80) {
                PulseOne.pulseSweep->clock();
                if ((PulseOne.lengthCount != 0) && !(Pulse1Control & 0x20)) {
                    PulseOne.lengthCount--;
                }

                PulseTwo.pulseSweep->clock();
                if ((PulseTwo.lengthCount != 0) || !(Pulse2Control & 0x20)) {
                    PulseTwo.lengthCount--;
                }
            }
            break;
    }

}

//Since every register except $4015 is write only, it's probably unnecessary to have data members for each register
//storing the last written value, will see if this can be simplified later on by just storing values in the device structs
void APU::Reg_Write(uint16_t reg, uint8_t data) {
    
    switch (reg % 0x4000) {
        case 0:
            Pulse1Control = data;
            PulseOne.dutyCycle = (data & 0xC0) >> 6;
            PulseOneEnv.loop = (data & 0x20);
            PulseOneEnv.constVol = (data & 0x10);
            PulseOneEnv.constVolLevel = (data & 0x0F);
            break;
        case 1:
            Pulse1Sweep = data;
            PulseOneSwp.reloadFlag = true;
            PulseOneSwp.sweepControl = data;
            break;
        case 2: //If t<8 then pulse is silenced
            Pulse1TimeLow = data;
            PulseOne.timer = PulseOneSwp.rawTimer = (Pulse1TimeLow | ((Pulse1TimeHigh & 0x07) << 8));
            PulseOneSwp.truePeriod = PulseOne.timer;
            PulseOneSwp.calculatePeriod();
            break;
        case 3: //If t<8 then pulse is silenced
            Pulse1TimeHigh = data;
            PulseOne.timer = PulseOneSwp.rawTimer = (Pulse1TimeLow | ((Pulse1TimeHigh & 0x07) << 8));
            PulseOneSwp.truePeriod = PulseOne.timer;
            PulseOneSwp.calculatePeriod();
            PulseOne.lengthCount = LENGTH_TABLE[((data & 0xF8) >> 3)];
            PulseOneEnv.startFlag = true;
            break;
        case 4:
            Pulse2Control = data;
            PulseTwo.dutyCycle = (data & 0xC0) >> 6;
            PulseTwoEnv.loop = (data & 0x20);
            PulseTwoEnv.constVol = (data & 0x10);
            PulseTwoEnv.constVolLevel = (data & 0x0F);
            break;
        case 5:
            Pulse2Sweep = data;
            PulseTwoSwp.reloadFlag = true;
            PulseTwoSwp.sweepControl = data;
            break;
        case 6: //If t<8 then pulse is silenced
            Pulse2TimeLow = data;
            PulseTwo.timer = PulseTwoSwp.rawTimer = (Pulse2TimeLow | ((Pulse2TimeHigh & 0x07) << 8));
            PulseTwoSwp.truePeriod = PulseTwo.timer;
            PulseTwoSwp.calculatePeriod();
            break;
        case 7: //If t<8 then pulse is silenced
            Pulse2TimeHigh = data;
            PulseTwo.timer = PulseTwoSwp.rawTimer = (Pulse2TimeLow | ((Pulse2TimeHigh & 0x07) << 8));
            PulseTwoSwp.truePeriod = PulseTwo.timer;
            PulseTwoSwp.calculatePeriod();
            PulseTwo.lengthCount = LENGTH_TABLE[((data & 0xF8) >> 3)];
            PulseTwoEnv.startFlag = true;
            break;

        case 0x15: //Enables or disables single channels
            ControlStatus = data;
            if (!(data & 0x01))
                PulseOne.lengthCount = 0;
            if (!(data & 0x02))
                PulseTwo.lengthCount = 0;
            break;

        case 0x17: //timer is also reset?
            FrameCount = data;
            FrameInterrupt = (data & 0x40) ? false : FrameInterrupt;
            ApuCycles = 0; //Passes for a frame counter reset?
            if (data & 0x80) {
                //Generate half and quarter frame signals
                PulseOne.pulseEnvelope->clock();
                PulseOne.pulseSweep->clock();
                PulseTwo.pulseEnvelope->clock();
                PulseTwo.pulseSweep->clock();
            }
            break;
    }

}

//Can only read $4015 but read is influenced by channel length counters
uint8_t APU::Reg_Read() {
    FrameInterrupt = false;
    return ControlStatus;
}



//main will do the work of the CPU until APU is working
int main(int argc, char *argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        std::cout << "Error initializing\n";

    APU A;
   
    A.Reg_Write(0x4000, 0x30);
    A.Reg_Write(0x4001, 0xFF);
    A.Reg_Write(0x4002, 0x00);
    A.Reg_Write(0x4003, 0x00);
    A.Reg_Write(0x4015, 0x0F);
    A.Reg_Write(0x4017, 0x40);

    A.Reg_Write(0x4002, 0xFD);
    A.Reg_Write(0x4003, 0x00);
    A.Reg_Write(0x4000, 0xBF);

    SDL_Thread *APU_Thread;
    APU_Thread = SDL_CreateThread(APU_Run, "APU", &A);
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(558));
        A.CpuCycles++;
    }


    /*auto pulseWave = [](double t, double freq) {
        return (2.0f * (2.0f*floor(freq*t) - floor(2.0f*freq*t) + 1));
    };

    float negate = 1;
    float transform = 50;
    float freq = 220.0f;

    for(int i = 0; i < 512; i++, g_sampleNum++)
    {
        double alter = negate*pulseWave((double)(g_sampleNum+transform) / (double)SAMPLE_RATE, freq);
        //g_sampleNum %= SAMPLE_RATE;
        double time = (double)g_sampleNum / (double)SAMPLE_RATE;
        double buffer = pulseWave(time, freq) + alter; //Plays a 2*freq hz square wave
        if (buffer < 0)
            buffer = 0;
        if (buffer > 2)
            buffer = 2;

        std::cout << buffer << '\n';
        
    }*/


    return 0;
}



int APU_Run(void* data) {

    ((APU* )data)->Channels_Out();

    return 0;
}