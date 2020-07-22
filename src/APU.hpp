#ifndef APU_H
#define APU_H

#include <thread>
#include <chrono>
#include <cmath>
#include <array>
#include <SDL.h>
#include <SDL_keyboard.h>

//Class data members: Upper camel case
//Class functions: Uppercase underscore separated
//Local vars: lower camel case
//Global vars: 'g_' preceded lower camel case
//Global constants: '_' separated capitals
const int SAMPLE_RATE = 44100;
long g_sampleNum;

const std::array<uint8_t, 32> LENGTH_TABLE = {10, 254, 20, 2, 40, 4, 80, 6,
                                                160, 8, 60, 10, 14, 12, 26, 14,
                                                12, 16, 24, 18, 48, 20, 96, 22,
                                                192, 24, 72, 26, 16, 28, 32, 30}; 

//All these structs may be restructured with different data members to reuduce # of calulations
//in callback, for example instead of giving the entire control register to the envelope have loop flag, constant vol flag,
// and volume to use for constant mode
struct Envelope {
    bool startFlag;
    uint8_t divider;
    uint8_t decayLevel;
    bool loop;
    bool constVol;
    uint8_t constVolLevel;

    void clock() {
        if (startFlag) {
            startFlag = false;
            decayLevel = 15;
            divider = constVolLevel;
        } else if (divider == 0) {
            divider = constVolLevel;
            if ((decayLevel == 0) && loop) {
                decayLevel = 15;
            } else {
                decayLevel--;
            }
        } else {
            divider--;
        }
    }
};


struct SweepUnit {
    uint8_t divider;
    uint16_t targetPeriod;
    uint16_t truePeriod;
    uint16_t rawTimer;
    uint8_t sweepControl;
    bool reloadFlag;
    bool channelNum; //true = 1, false = 2

    void calculatePeriod() {
        int16_t shift = (rawTimer >> (sweepControl & 0x07));
        targetPeriod = truePeriod + ((sweepControl & 0x08) ? shift*(-1) - channelNum : shift);
    }

    void clock() {
        if (divider == 0) {
            if ((targetPeriod <= 0x7FF) && (sweepControl & 0x80) && (rawTimer >> (sweepControl & 0x07))) {
                truePeriod = targetPeriod;
                calculatePeriod();
            }       
            divider = (0x70 >> 4);
            reloadFlag = false;  
        } else if (reloadFlag) {
            divider = (0x70 >> 4);
            reloadFlag = false; 
        } else {
            divider--;
        }

        if ((targetPeriod <= 0x7FF) && (sweepControl & 0x80) && (rawTimer >> (sweepControl & 0x07)))
            truePeriod = targetPeriod;
    }
}; 

struct Pulse {
    uint8_t lengthCount;
    uint16_t timer;
    uint8_t dutyCycle;
    uint8_t sequence;
    Envelope *pulseEnvelope;
    SweepUnit *pulseSweep;
    bool silenced;
};

struct Mixer {
    Pulse *pulseOne;

    std::array<double, 31> pulseMixTable;
    std::array<double, 203> tndTable;
    double mixAudio() {
        //Pulse one
        if (pulseOne->pulseEnvelope->constVol)
            return pulseMixTable[pulseOne->pulseEnvelope->constVolLevel];
        else
            return pulseMixTable[pulseOne->pulseEnvelope->decayLevel];
    }
};


//Looking at mixer formula suggests to me that the final output is applied to all channels as the volume
//Will implement as that for now and see if it sounds alright
void audio_callback(void *user_data, uint8_t *raw_buffer, int bytes) {
    float *buffer = (float*)raw_buffer;
    Mixer *output = (Mixer *)user_data;
    float freq = round(1789773 / (16 * (output->pulseOne->pulseSweep->rawTimer + 1))) / 2.0f; //Provided sweep is disabled
    float period = floor((1.0f / freq) * SAMPLE_RATE); //number of data points in a period
    float negate;
    float transform;
    float vol = output->mixAudio();

    auto pulseWave = [](double t, double freq) {
        return (2.0f * (2.0f*floor(freq*t) - floor(2.0f*freq*t) + 1));
    };

    switch (output->pulseOne->dutyCycle) {
        case 0:
            negate = -1;
            transform = period / 8.0f;
            break;
        case 1:
            negate = -1;
            transform = period / 4.0f;
            break;
        case 2:
            negate = 0;
            transform = 0;
            break;
        case 3:
            negate = 1;
            transform = period / 4.0f;
            break;
    }

    for(int i = 0; i < bytes/4; i++, g_sampleNum++)
    {
        double alter = negate*(pulseWave((double)(g_sampleNum+transform) / (double)SAMPLE_RATE, freq));
        //std::cout << alter << '\n';
        //g_sampleNum %= SAMPLE_RATE;
        //double time = (double)g_sampleNum / (double)SAMPLE_RATE;
        //buffer[i] = 2.0f * (2.0f*floor(freq*time) - floor(2.0f*freq*time)) + 1; //Plays a 2*freq hz square wave
        buffer[i] = pulseWave((double)g_sampleNum / (double)SAMPLE_RATE, freq) + alter;
        if (buffer[i] < 0)
            buffer[i] = 0;
        if (buffer[i] > 2)
            buffer[i] = 2;
        buffer[i] *= vol;
    }
}


class APU {
    friend class CPU;
    public:
        APU(): Pulse1Control(0), Pulse1Sweep(0), Pulse1TimeLow(0), Pulse1TimeHigh(0), 
                Pulse2Control(0), Pulse2Sweep(0), Pulse2TimeLow(0), Pulse2TimeHigh(0),
                ControlStatus(0), FrameCount(0), CpuCycles(0), ApuCycles(0), FrameInterrupt(false) {

            AudioMixer = {};
            PulseOneEnv = {};
            PulseOneSwp = {};
            PulseOneSwp.channelNum = true;
            PulseOne = {};
            PulseOne.pulseEnvelope = &PulseOneEnv;
            PulseOne.pulseSweep = &PulseOneSwp;
            g_sampleNum = 0;

            AudioMixer.pulseOne = &PulseOne;

            want.freq = SAMPLE_RATE; // number of samples per second
            want.format = AUDIO_F32SYS;
            want.channels = 1; // only one channel
            want.samples = 2048; // buffer-size
            want.userdata = &AudioMixer;
            want.callback = audio_callback;

            for (int i=0; i<31; i++)
                AudioMixer.pulseMixTable[i] = 95.52 / (8128.0 / i + 100);

            SDL_AudioSpec have;
            SDL_OpenAudio(&want, &have);
        }


        uint8_t CpuCycles;
        uint16_t ApuCycles;
        void Reg_Write(uint16_t reg, uint8_t data);
        void Channels_Out();
        void PulseOne_Out();        
        uint8_t Reg_Read();

    private:
        //Channel and component structs
        Mixer AudioMixer;
        Envelope PulseOneEnv;
        SweepUnit PulseOneSwp;
        Pulse PulseOne;

        //Pulse 1 channel registers
        uint8_t Pulse1Control; //$4000
        uint8_t Pulse1Sweep; //$4001
        uint8_t Pulse1TimeLow; //$4002
        uint8_t Pulse1TimeHigh; //$4003

        //Pulse 2 channel registers
        uint8_t Pulse2Control; //$4004
        uint8_t Pulse2Sweep; //$4005
        uint8_t Pulse2TimeLow; //$4006
        uint8_t Pulse2TimeHigh; //$4007

        //Triangle channel


        //Noise channel


        //DMC 


        //Control registers
        uint8_t ControlStatus; //$4015
        uint8_t FrameCount; //$4017
        
        //Other
        SDL_AudioSpec want; //Change name!!!
        bool FrameInterrupt;

        std::array<double, 31> PulseMixTable;
        std::array<double, 203> TndTable;
};


#endif