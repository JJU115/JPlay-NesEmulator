#include <iostream>
#include "APU.hpp"


long g_sampleNum;

void audio_callback(void *user_data, uint8_t *raw_buffer, int bytes) {
    float *buffer = (float*)raw_buffer;
    Mixer *output = (Mixer *)user_data;
    
    for(int i = 0; i < bytes/4; i++, g_sampleNum++)
        buffer[i] = output->mixAudio(g_sampleNum);    
}


void APU::Open_Audio() {
    g_sampleNum = 0;
    want.callback = audio_callback;
    SDL_AudioSpec have;
    SDL_OpenAudio(&want, &have);
    SDL_PauseAudio(0);
}


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


//Since every register except $4015 is write only, it's probably unnecessary to have data members for each register
//storing the last written value, will see if this can be simplified later on by just storing values in the device structs
void APU::Reg_Write(uint16_t reg, uint8_t data) {
    
    //Only load a new length value if the channel is enabled
    switch (reg % 0x4000) {
        case 0:
            Pulse1Control = data;
            PulseOne.dutyCycle = PULSE_DUTY[(data & 0xC0) >> 6];
            PulseOneEnv.loop = (data & 0x20);
            PulseOneEnv.constVol = (data & 0x10);
            PulseOneEnv.constVolLevel = (data & 0x0F);

            PulseOne.highAmnt = floor(PulseOne.dutyCycle * PulseOne.period);
            PulseOne.lowAmnt = floor(PulseOne.period - PulseOne.highAmnt);
            break;
        case 1:
            PulseOneSwp.reloadFlag = true;
            PulseOneSwp.sweepControl = data;
            break;
        case 2: //If t<8 then pulse is silenced
            Pulse1TimeLow = data;
            PulseOne.timer = PulseOneSwp.rawTimer = (Pulse1TimeLow | ((Pulse1TimeHigh & 0x07) << 8));
            PulseOneSwp.truePeriod = PulseOne.timer;
            PulseOneSwp.calculatePeriod();

            PulseOne.period = floor((1.0f / (round(1789773 / (16 * (PulseOne.timer + 1))) / 2.0f)) * SAMPLE_RATE) * 2;
            PulseOne.highAmnt = floor(PulseOne.dutyCycle * PulseOne.period);
            PulseOne.lowAmnt = floor(PulseOne.period - PulseOne.highAmnt);
            break;
        case 3: //If t<8 then pulse is silenced
            Pulse1TimeHigh = data;
            PulseOne.timer = PulseOneSwp.rawTimer = (Pulse1TimeLow | ((Pulse1TimeHigh & 0x07) << 8));
            PulseOneSwp.truePeriod = PulseOne.timer;
            PulseOneSwp.calculatePeriod();
            if (PulseOne.enabled)
                PulseOne.lengthCount = LENGTH_TABLE[((data & 0xF8) >> 3)];
            PulseOneEnv.startFlag = true;

            PulseOne.period = floor((1.0f / (round(1789773 / (16 * (PulseOne.timer + 1))) / 2.0f)) * SAMPLE_RATE) * 2;
            PulseOne.highAmnt = floor(PulseOne.dutyCycle * PulseOne.period);
            PulseOne.lowAmnt = floor(PulseOne.period - PulseOne.highAmnt);
            break;
        case 4:
            Pulse2Control = data;
            PulseTwo.dutyCycle = PULSE_DUTY[(data & 0xC0) >> 6];
            PulseTwoEnv.loop = (data & 0x20);
            PulseTwoEnv.constVol = (data & 0x10);
            PulseTwoEnv.constVolLevel = (data & 0x0F);

            PulseTwo.highAmnt = floor(PulseTwo.dutyCycle * PulseTwo.period);
            PulseTwo.lowAmnt = floor(PulseTwo.period - PulseTwo.highAmnt);
            break;
        case 5:
            PulseTwoSwp.reloadFlag = true;
            PulseTwoSwp.sweepControl = data;
            break;
        case 6: //If t<8 then pulse is silenced
            Pulse2TimeLow = data;
            PulseTwo.timer = PulseTwoSwp.rawTimer = (Pulse2TimeLow | ((Pulse2TimeHigh & 0x07) << 8));
            PulseTwoSwp.truePeriod = PulseTwo.timer;
            PulseTwoSwp.calculatePeriod();

            PulseTwo.period = floor((1.0f / (round(1789773 / (16 * (PulseTwo.timer + 1))) / 2.0f)) * SAMPLE_RATE) * 2;
            PulseTwo.highAmnt = floor(PulseTwo.dutyCycle * PulseTwo.period);
            PulseTwo.lowAmnt = floor(PulseTwo.period - PulseTwo.highAmnt);
            break;
        case 7: //If t<8 then pulse is silenced
            Pulse2TimeHigh = data;
            PulseTwo.timer = PulseTwoSwp.rawTimer = (Pulse2TimeLow | ((Pulse2TimeHigh & 0x07) << 8));
            PulseTwoSwp.truePeriod = PulseTwo.timer;
            PulseTwoSwp.calculatePeriod();
            if (PulseTwo.enabled)
                PulseTwo.lengthCount = LENGTH_TABLE[((data & 0xF8) >> 3)];
            PulseTwoEnv.startFlag = true;

            PulseTwo.period = floor((1.0f / (round(1789773 / (16 * (PulseTwo.timer + 1))) / 2.0f)) * SAMPLE_RATE) * 2;
            PulseTwo.highAmnt = floor(PulseTwo.dutyCycle * PulseTwo.period);
            PulseTwo.lowAmnt = floor(PulseTwo.period - PulseTwo.highAmnt);
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
            NoiseChannel.mode = data & 0x80;
            NoiseChannel.timer = 2*NOISE_PERIOD[data & 0x0F];
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

        case 0x17:
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

    status |= (PulseOne.lengthCount > 0) | ((PulseTwo.lengthCount > 0) * 2) | ((TriChannel.lengthCount > 0) * 4) | 
    ((NoiseChannel.lengthCount > 0) * 8) | ((DMCChannel.bytesRemain > 0) * 16);
    
    return (status | (DMCChannel.interrupt << 7));
}