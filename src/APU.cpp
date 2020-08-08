#include <iostream>
#include "APU.hpp"

//int APU_Run(void* data);
long g_sampleNum;

void audio_callback(void *user_data, uint8_t *raw_buffer, int bytes) {
    float *buffer = (float*)raw_buffer;
    Mixer *output = (Mixer *)user_data;
    
    //Can call individual channel sample output functions but using the mixer function doesn't work yet
    for(int i = 0; i < bytes/4; i++, g_sampleNum++)
    {
        buffer[i] = output->mixAudio(g_sampleNum);
        //buffer[i] = output->pulseOne->getSample(g_sampleNum);
        //output->pulseOne->getSample(g_sampleNum) + output->pulseTwo->getSample(g_sampleNum);
        //std::cout << buffer[i] << '\n';
    }
}


void APU::Open_Audio() {
    g_sampleNum = 0;
    want.callback = audio_callback;
    SDL_AudioSpec have;
    SDL_OpenAudio(&want, &have);
}


//Function run on separate SDL thread
void APU::Channels_Out() {
    SDL_PauseAudio(0);
    while (true) {
        //Many of the clocking operations actually happen in between APU cycles, i.e. at 3728.5 instead of 3728
        //Setting the frame IRQ also does this once, 
        while (CpuCycles < 2)
            std::this_thread::yield();

        ApuCycles++; //% based on 4/5 step sequence
        ApuCycles %= (FrameCount & 0x80) ? 18641 : 14915;
        
        Pulse_Out();
        CpuCycles -=2;
    }

}

//Can include triangle channel clocking in here as well since only their timers tick at different rates
//This implementation doesn't actually operate on the timers, it just uses their values to determine the period
//However, the noise channel produces one "random" bit per timer clock thus operating on it 
void APU::Pulse_Out() {
    
    switch (ApuCycles) {
        case 3728:
            PulseOne.pulseEnvelope->clock();
            PulseTwo.pulseEnvelope->clock();
            NoiseEnv.clock();
            TriChannel.clock();
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

            TriChannel.clock();
            if ((TriChannel.lengthCount != 0) && !(TriLinearCount & 0x80)) {
                TriChannel.lengthCount--;
            }
            
            NoiseEnv.clock();
            if ((NoiseChannel.lengthCount != 0) && !(NoiseControl & 0x20)) {
                NoiseChannel.lengthCount--;
            }
            break;

        case 11185:
            PulseOne.pulseEnvelope->clock();
            PulseTwo.pulseEnvelope->clock();
            TriChannel.clock();
            NoiseEnv.clock();
            break;

        case 14914: //If 4-step
            if (!(FrameCount & 0xC0)) {
                //FrameInterrupt = true; //Fire IRQ
                //FireIRQ = true;
            }
            break;

        case 14915: //If 4-step
            if (!(FrameCount & 0x80)) {
                PulseOne.pulseSweep->clock();
                if ((PulseOne.lengthCount != 0) && !(Pulse1Control & 0x20)) {
                    PulseOne.lengthCount--;
                }

                PulseTwo.pulseSweep->clock();
                if ((PulseTwo.lengthCount != 0) && !(Pulse2Control & 0x20)) {
                    PulseTwo.lengthCount--;
                }

                TriChannel.clock();
                if ((TriChannel.lengthCount != 0) && !(TriLinearCount & 0x80)) {
                    TriChannel.lengthCount--;
                }

                NoiseEnv.clock();
                if ((NoiseChannel.lengthCount != 0) && !(NoiseControl & 0x20)) {
                    NoiseChannel.lengthCount--;
                }
            }

            if (!(FrameCount & 0xC0)) {
                FrameInterrupt = true; //Fire IRQ
                FireIRQ = true;
            }
            break;

        case 18640: //If 5-step
            if (FrameCount & 0x80) {
                PulseOne.pulseSweep->clock();
                if ((PulseOne.lengthCount != 0) && !(Pulse1Control & 0x20)) {
                    PulseOne.lengthCount--;
                }

                PulseTwo.pulseSweep->clock();
                if ((PulseTwo.lengthCount != 0) && !(Pulse2Control & 0x20)) {
                    PulseTwo.lengthCount--;
                }

                TriChannel.clock();
                if ((TriChannel.lengthCount != 0) && !(TriLinearCount & 0x80)) {
                    TriChannel.lengthCount--;
                }

                NoiseEnv.clock();
                if ((NoiseChannel.lengthCount != 0) && !(NoiseControl & 0x20)) {
                    NoiseChannel.lengthCount--;
                }
            }
            break;
    }

}

//Since every register except $4015 is write only, it's probably unnecessary to have data members for each register
//storing the last written value, will see if this can be simplified later on by just storing values in the device structs
void APU::Reg_Write(uint16_t reg, uint8_t data) {
    
    //Only load a new length value if the channel is enabled
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
            if (PulseOne.enabled)
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
            if (PulseTwo.enabled)
                PulseTwo.lengthCount = LENGTH_TABLE[((data & 0xF8) >> 3)];
            PulseTwoEnv.startFlag = true;
            break;
        case 8:
            TriLinearCount = data;
            TriChannel.reloadVal = data & 0x7F;
            TriChannel.controlFlag = data & 0x80;
            break;
        case 0x0A:
            TriTimerLow = data;
            TriChannel.timer = (TriTimerLow | ((TriTimerHigh & 0x07) << 8));
            break;
        case 0x0B:
            TriTimerHigh = data & 0x07;
            if (TriChannel.enabled)
                TriChannel.lengthCount = LENGTH_TABLE[((data & 0xF8) >> 3)];
            TriChannel.timer = (TriTimerLow | ((TriTimerHigh & 0x07) << 8));
            TriChannel.counterReload = true;
            break;
        case 0x0C:
            NoiseControl = data;
            NoiseEnv.loop = data & 0x20;
            NoiseEnv.constVol = data & 0x10;
            NoiseEnv.constVolLevel = data & 0x0F;
            break;
        case 0x0E:
            NoiseModePeriod = data;
            NoiseChannel.mode = data & 0x80;
            NoiseChannel.timer = NOISE_PERIOD[data & 0x0F];
            NoiseChannel.reload = NoiseChannel.timer;
            break;
        case 0x0F:
            NoiseLength = (data & 0xF8) >> 3;
            if (NoiseChannel.enabled)
                NoiseChannel.lengthCount = LENGTH_TABLE[(data & 0xF8) >> 3];
            NoiseEnv.startFlag = true;
            break;

        case 0x15: //Enables or disables single channels
            ControlStatus = data;
            if (!(data & 0x01)) {
                PulseOne.lengthCount = 0;
                PulseOne.enabled = false;
            } else
                PulseOne.enabled = true;
            
            if (!(data & 0x02)) {
                PulseTwo.lengthCount = 0;
                PulseTwo.enabled = false;
            } else
                PulseTwo.enabled = true;

            if (!(data & 0x04)) {
                TriChannel.lengthCount = 0;
                TriChannel.enabled = false;
            } else
                TriChannel.enabled = true;

            if (!(data & 0x08)) {
                NoiseChannel.lengthCount = 0;
                NoiseChannel.enabled = false;
            } else
                NoiseChannel.enabled = true;
            break;

        case 0x17: //timer is also reset?
            FrameCount = data;
            FrameInterrupt = (data & 0x40) ? false : FrameInterrupt;
            ApuCycles = 0; //Passes for a frame counter reset?
            if (data & 0x80) {
                //Generate half and quarter frame signals
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

                TriChannel.clock();
                if ((TriChannel.lengthCount != 0) && !(TriLinearCount & 0x80)) {
                    TriChannel.lengthCount--;
                }

                NoiseEnv.clock();
                if ((NoiseChannel.lengthCount != 0) && !(NoiseControl & 0x20)) {
                    NoiseChannel.lengthCount--;
                }
            }
            break;
    }

}

//Can only read $4015 but read is influenced by channel length counters
uint8_t APU::Reg_Read() {
    uint8_t status = FrameInterrupt << 6;
    FrameInterrupt = false;

    if (PulseOne.lengthCount > 0)
        status |= 1;
    if (PulseTwo.lengthCount > 0)
        status |= 2;
    if (TriChannel.lengthCount > 0)
        status |= 4;
    if (NoiseChannel.lengthCount > 0)
        status |= 8;

    return status;
}



//main will do the work of the CPU until APU is working
/*int main(int argc, char *argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        std::cout << "Error initializing\n";

    APU A;
    long cyc = 0;
   
    A.Reg_Write(0x4015, 0x0F);
    A.Reg_Write(0x4017, 0xC0);

    

    SDL_Thread *APU_Thread;
    APU_Thread = SDL_CreateThread(APU_Run, "APU", &A);
    
    //To emulate the start of DK music
    while (true) {
        SDL_Delay(0.01);
        A.CpuCycles++;
        switch (++cyc) {
            ////PULSE ONE
            case 118316:
                 A.Reg_Write(0x4000, 0x00);
                break;
            case 118320:
                A.Reg_Write(0x4001, 0x7F);
                break;
            case 1279568:
                A.Reg_Write(0x4002, 0x80);
                break;
            case 1279579:
                A.Reg_Write(0x4003, 0x0A);
                break;


            case 1279616:
                A.Reg_Write(0x4000, 0x06);
                break;
            case 1279620:
                A.Reg_Write(0x4001, 0x7F);
                break;
            case 1666676:
                A.Reg_Write(0x4002, 0xFC);
                break;
            case 1666687:
                A.Reg_Write(0x4003, 0x09);
                break;


            case 1666724:
                A.Reg_Write(0x4000, 0x06);
                break;
            case 1666728:
                A.Reg_Write(0x4001, 0x7F);
                break;
            case 2053818:
                A.Reg_Write(0x4002, 0x80);
                break;
            case 2053829:
                A.Reg_Write(0x4003, 0x0A);
                break;


            case 2053866:
                A.Reg_Write(0x4000, 0x06);
                break;
            case 2053870:
                A.Reg_Write(0x4001, 0x7F);
                break;
            case 2441010:
                A.Reg_Write(0x4002, 0xFC);
                break;
            case 2441021:
                A.Reg_Write(0x4003, 0x09);
                break;
            
            case 2441058:
                A.Reg_Write(0x4000, 0x06);
                break;
            case 2441062:
                A.Reg_Write(0x4001, 0x7F);
                break;



            //// PULSE TWO
            case 118422:
                 A.Reg_Write(0x4005, 0x7F);
                break;
            case 118439:
                A.Reg_Write(0x4006, 0xAB);
                break;
            case 118450:
                A.Reg_Write(0x4007, 0x09);
                break;
            case 118482:
                A.Reg_Write(0x4004, 0x84);
                break;

            case 505288:
                A.Reg_Write(0x4005, 0x7F);
                break;
            case 505305:
                A.Reg_Write(0x4006, 0xAB);
                break;
            case 505316:
                A.Reg_Write(0x4007, 0x09);
                break;
            case 505348:
                A.Reg_Write(0x4004, 0x84);
                break;

            case 892434:
                A.Reg_Write(0x4005, 0x7F);
                break;
            case 892451:
                A.Reg_Write(0x4006, 0xFD);
                break;
            case 892462:
                A.Reg_Write(0x4007, 0x08);
                break;
            case 892487:
                A.Reg_Write(0x4004, 0x89);
                break;



        }
    }


    /*auto pulseWave = [](double t, double freq) {
        return (2.0f * (2.0f*floor(freq*t) - floor(2.0f*freq*t) + 1));
    };
    g_sampleNum = 0;
    float negate = 1;
    float transform = 50;
    float freq = 220.0f;
    float timer = 139.0f; //400hz 
    float period = floor((4480 / 1789773.0f) * SAMPLE_RATE);

    for(int i = 0; i < 512; i++, g_sampleNum++)
    {
        float out = (2.0 / period * abs((g_sampleNum % (int)period) - period/2.0));
        std::cout << out << '\n';
        
    }


    return 0;
}



int APU_Run(void* data) {

    ((APU* )data)->Channels_Out();

    return 0;
}*/