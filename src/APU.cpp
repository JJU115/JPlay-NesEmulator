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
    SDL_PauseAudio(0);
}


/*
void APU::Channels_Out() {
    SDL_PauseAudio(0);
    while (true) {
        //Many of the clocking operations actually happen in between APU cycles, i.e. at 3728.5 instead of 3728
        //Setting the frame IRQ also does this once, 
        while (!CpuCycle)
            std::this_thread::yield();

        CpuCycle = false;
        CpuCycles++; //% based on 4/5 step sequence
        CpuCycles %= (FrameCount & 0x80) ? 37282 : 29830;
        
        Pulse_Out();
        //CpuCycles -=2;
    }

}*/


//Alternative Pulse out method which is called directly by the cpu every one of its cycles
bool APU::Pulse_Out() {
    switch (++CpuCycles) {
        case 7457:
            PulseOneEnv.clock();
            PulseTwoEnv.clock();
            NoiseEnv.clock();
            TriChannel.clock();
            break;
        case 14913:
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

        case 22371:
            PulseOne.pulseEnvelope->clock();
            PulseTwo.pulseEnvelope->clock();
            TriChannel.clock();
            NoiseEnv.clock();
            break;

        case 29831: //If 4-step
            if (!(FrameCount & 0xC0)) {
                FrameInterrupt = true; //Fire IRQ
                FireIRQ++;
            }
            break;

        case 29832: //If 4-step
            if (!(FrameCount & 0x80)) {
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

            if (!(FrameCount & 0xC0)) {
                FrameInterrupt = true; //Fire IRQ
                FireIRQ++;
            }
            break;
        
        case 29833:
            if (!(FrameCount & 0xC0)) {
                FrameInterrupt = true; //Fire IRQ
                FireIRQ++;
            }
            break;

        case 37281: //If 5-step
            if (FrameCount & 0x80) {
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
    CpuCycles %= (FrameCount & 0x80) ? 37282 : 29833;

    if (--DMCChannel.rate == 0) {
        DMCChannel.rate = DMCChannel.reload;
        if (DMCChannel.enabled)
            DMCChannel.clock();
    }
    
    //Try checking for an empty DMC buffer here, then trigger the memory reader if necessary
    if (DMCChannel.bufferEmpty && DMCChannel.bytesRemain && DMCChannel.enabled) {
        DMCChannel.memoryRead();
        if (DMCChannel.interrupt)
            FireIRQ++;
        return true;
    } else {
        return false;
    } 
    
}






//Function run on separate SDL thread
//A separate thread for the APU could be eliminated entirely as calling Pulse_out directly from the CPU wait function
//seems to give better performance
void APU::Channels_Out() {
    //SDL_PauseAudio(0);
    while (true) {
        //Many of the clocking operations actually happen in between APU cycles, i.e. at 3728.5 instead of 3728
        //Setting the frame IRQ also does this once, 
        while (CpuCycles < 2)
            std::this_thread::yield();

        ApuCycles++; //% based on 4/5 step sequence
        ApuCycles %= (FrameCount & 0x80) ? 18641 : 14915;
        
        //Pulse_Out();
        //CpuCycles -=2;
    }

}

//Can include triangle channel clocking in here as well since only their timers tick at different rates
//This implementation doesn't actually operate on the timers, it just uses their values to determine the period
//However, the noise channel produces one "random" bit per timer clock thus operating on it 
/*void APU::Pulse_Out() {
    
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

}*/

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
            TriChannel.timerReload = TriChannel.timer; //Added
            TriChannel.recalculate();
            break;
        case 0x0B:
            TriTimerHigh = data & 0x07;
            if (TriChannel.enabled)
                TriChannel.lengthCount = LENGTH_TABLE[((data & 0xF8) >> 3)];
            TriChannel.timer = (TriTimerLow | ((TriTimerHigh & 0x07) << 8));
            TriChannel.timerReload = TriChannel.timer; //Added
            TriChannel.counterReload = true;
            TriChannel.recalculate();
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

        case 0x10:
            DMCChannel.IRQEnabled = data & 0x80;
            if (!(data & 0x80))
                DMCChannel.interrupt = false;

            DMCChannel.loop = data & 0x40;
            DMCChannel.rate = DMC_RATE[data & 0x0F];
            DMCChannel.reload = DMCChannel.rate;
            break;
        case 0x11:
            DMCChannel.output = data & 0x7F;
            break;

        case 0x12:
            DMCChannel.sampAddress = 0xC000 + (data * 64);
            break;

        case 0x13:
            DMCChannel.sampLength = (16 * data) + 1;
            break;

        case 0x15: //Enables or disables single channels
            ControlStatus = data;
            DMCChannel.interrupt = false;
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

            if (!(data & 0x10)) {
                DMCChannel.bytesRemain = 0;
                DMCChannel.enabled = false;
            } else { //If bytes remain, channel will not be enabled at all, need to fix
                DMCChannel.enabled = true;
                if (DMCChannel.bytesRemain) {
                    DMCChannel.memoryRead();
                } else {
                    DMCChannel.currAddress = DMCChannel.sampAddress;
                    DMCChannel.bytesRemain = DMCChannel.sampLength;
                }
            }

            break;

        case 0x17: //timer is also reset?
            FrameCount = data;
            FrameInterrupt = (data & 0x40) ? false : FrameInterrupt;
            CpuCycles = 0; //Passes for a frame counter reset?
            FireIRQ = 0;
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

    status |= (PulseOne.lengthCount > 0);
    if (PulseTwo.lengthCount > 0)
        status |= 2;
    if (TriChannel.lengthCount > 0)
        status |= 4;
    if (NoiseChannel.lengthCount > 0)
        status |= 8;
    if (DMCChannel.bytesRemain > 0)
        status |= 16;
    
    return (status | (DMCChannel.interrupt << 7));
}



//main will do the work of the CPU until APU is working
/*int main(int argc, char *argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        std::cout << "Error initializing\n";

    APU A;

    SDL_Thread *APU_Thread;
    APU_Thread = SDL_CreateThread(APU_Run, "APU", &A);
    
    while (true) {
        SDL_Delay(0.01);
        A.Pulse_Out();
        

    }


    return 0;
}



int APU_Run(void* data) {

    ((APU* )data)->Channels_Out();

    return 0;
}*/