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

const std::array<uint8_t, 32> LENGTH_TABLE = {10, 254, 20, 2, 40, 4, 80, 6,
                                                160, 8, 60, 10, 14, 12, 26, 14,
                                                12, 16, 24, 18, 48, 20, 96, 22,
                                                192, 24, 72, 26, 16, 28, 32, 30};

const std::array<uint16_t, 16> NOISE_PERIOD = {4, 8, 16, 32, 64,
                                                96, 128, 160, 202, 254,
                                                380, 508, 762, 1016, 2034, 4068};  


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
            if (decayLevel != 0) {
                decayLevel--;
            } else if (loop) {
                decayLevel = 15;
            }
        } else {
            divider--;
        }
    }

    double getVol() {
        if (constVol)
            return static_cast<double>(constVolLevel);
        else
            return static_cast<double>(decayLevel);
    }
};


//Put a bool here to determine if sweep is silencing channel, note the t < 8 condition
struct SweepUnit {
    uint8_t divider;
    uint16_t targetPeriod;
    uint16_t truePeriod;
    uint16_t rawTimer;
    uint8_t sweepControl;
    bool reloadFlag;
    bool channelNum; //true = 1, false = 2
    bool silence;

    //Check muting here, when subtracting does underflow from 0 to ffff happen? If so sweep would mute everytime
    //the negate flag is set and the shift amount is 0
    void calculatePeriod() {
        int16_t shift = (rawTimer >> (sweepControl & 0x07));
        targetPeriod = truePeriod + ((sweepControl & 0x08) ? shift*(-1) - channelNum : shift);
        silence = (targetPeriod > 0x7FF || truePeriod < 8);
    }

    void clock() {
        if (divider == 0) {
            if (!silence && (sweepControl & 0x80) && (rawTimer >> (sweepControl & 0x07))) {
                truePeriod = targetPeriod;
                calculatePeriod();
            }       
            divider = ((sweepControl & 0x70) >> 4);
            reloadFlag = false;  
        } else if (reloadFlag) {
            divider = ((sweepControl & 0x70) >> 4);
            reloadFlag = false; 
        } else {
            divider--;
        }

        if (!silence && (sweepControl & 0x80) && (rawTimer >> (sweepControl & 0x07))) {
            truePeriod = targetPeriod;
            calculatePeriod();
        }
    }
}; 


struct Pulse {
    bool enabled;
    uint8_t lengthCount;
    uint16_t timer;
    uint8_t dutyCycle;
    uint8_t sequence;
    Envelope *pulseEnvelope;
    SweepUnit *pulseSweep;

    double getSample(long sampleNum) {
        if (lengthCount == 0 || pulseSweep->silence)
            return 0;

        float freq = round(1789773 / (16 * (pulseSweep->truePeriod + 1))) / 2.0f;
        float period = floor((1.0f / freq) * SAMPLE_RATE);
        float alter;
        double sample;

        auto pulseWave = [](double t, double freq) {
            return (2.0f*floor(freq*t) - floor(2.0f*freq*t) + 1);
        };


        switch (dutyCycle) {
            case 0:
                alter = (-1)*(pulseWave((double)(sampleNum + period / 8.0f) / (double)SAMPLE_RATE, freq));
                break;
            case 1:
                alter = (-1)*(pulseWave((double)(sampleNum + period / 4.0f) / (double)SAMPLE_RATE, freq));
                break;
            case 2:
                alter = 0;
                break;
            case 3:
                alter = pulseWave((double)(sampleNum + period / 4.0f) / (double)SAMPLE_RATE, freq);
                break;
        }

        sample = pulseWave((double)sampleNum / (double)SAMPLE_RATE, freq) + alter;
        if (sample < 0 || sample == 0)
            return 0;
        if (sample > 1 || sample == 1)
            return pulseEnvelope->getVol();

    }
};


struct Triangle {
    bool enabled;
    uint8_t sequencePos;
    uint8_t lengthCount;
    uint16_t timer;
    uint8_t reloadVal;
    uint8_t linearCount;
    bool controlFlag;
    bool counterReload;


    void clock() {
        if (counterReload) {
            linearCount = reloadVal;
        } else if (linearCount != 0) {
            linearCount--;
        }

        if (!controlFlag) {
            counterReload = false;
        }
    }

    //Might need to relook at this, the NES sends the values 15 -> 0 then 0 -> 15 as values but the 
    //SDL audio sampling rate makes it difficult to send this discrete sequence without repeating a lot of values
    //given the speed at which the APU clock is running resulting in bad audio. The triangle needs to change or
    //the mixer can't strictly operate as nesdev says it should 
    double getSample(long sampleNum) {
        if (!(lengthCount) || !(linearCount) || timer < 2)
            return 0;
        else { 
            //amplitude 1, no dividing by 2 since the Triangle timer ticks at twice the Pulse timer rate
            //Formula has been slightly adjusted so that wave oscillates between 0 and 1
            float period = floor(((32 * (timer + 1))/ 1789773.0f) * SAMPLE_RATE) * 2;
            return (2.0 / period * abs((sampleNum % (int)period) - period/2.0));
        }
    }
};


struct Noise {
    bool enabled;
    uint8_t lengthCount;
    uint16_t shiftReg;
    uint16_t timer;
    uint16_t reload;
    bool mode;
    bool feedBack;
    Envelope *noiseEnvelope;


    void clock() {
        feedBack = (shiftReg & 0x0001) ^ ((mode) ? ((shiftReg & 0x0040) >> 6) : ((shiftReg & 0x0002) >> 1));
        shiftReg = shiftReg >> 1;
        shiftReg &= 0xBFFF;
        shiftReg |= (feedBack << 14);
    }

    double getSample() {
        if (timer == 0) {
            clock();
            timer = reload;
        } else {
            timer--;
        }
        
        if (lengthCount == 0 || (shiftReg & 0x0001))
            return 0;
        else {
            return static_cast<double>(feedBack * noiseEnvelope->getVol());
        }
    }
};

//The mix audio function has to take inputs from all channels and turn them into a single audio signal
//The formula on given on nesdev may not be applicable, also still confused as to if there are multiple outputs
//played at once or if they are combined somehow
struct Mixer {
    Pulse *pulseOne;
    Pulse *pulseTwo;
    Triangle *tri;
    Noise *noise;

    std::array<double, 31> pulseMixTable;
    std::array<double, 203> tndTable;
    double mixAudio(long sampleNum) {
        double pulseOut = pulseMixTable[pulseOne->getSample(sampleNum) + pulseTwo->getSample(sampleNum)]; 
        //float tndOut = tndTable[3 * tri->getSample(sampleNum) + 2 * noise->getSample()];

        return pulseOut;
    }
};


class APU {
    public:
        APU(): Pulse1Control(0), Pulse1Sweep(0), Pulse1TimeLow(0), Pulse1TimeHigh(0), 
                Pulse2Control(0), Pulse2Sweep(0), Pulse2TimeLow(0), Pulse2TimeHigh(0),
                TriLinearCount(0), TriTimerHigh(0), TriTimerLow(0), NoiseControl(0),
                NoiseModePeriod(0), NoiseLength(0), ControlStatus(0), FrameCount(0),
                CpuCycles(0), ApuCycles(0), FrameInterrupt(false), FireIRQ(false) {
              
            AudioMixer = {};
            PulseOneEnv = {};
            PulseOneSwp = {};
            PulseOneSwp.channelNum = true;
            PulseOne = {};
            PulseOne.pulseEnvelope = &PulseOneEnv;
            PulseOne.pulseSweep = &PulseOneSwp;

            PulseTwoEnv = {};
            PulseTwoSwp = {};
            PulseTwo = {};
            PulseTwo.pulseEnvelope = &PulseTwoEnv;
            PulseTwo.pulseSweep = &PulseTwoSwp;

            TriChannel = {};

            NoiseChannel = {};
            NoiseEnv = {};
            NoiseChannel.shiftReg = 1;
            NoiseChannel.noiseEnvelope = &NoiseEnv;

            AudioMixer.pulseOne = &PulseOne;
            AudioMixer.pulseTwo = &PulseTwo;
            AudioMixer.tri = &TriChannel;
            AudioMixer.noise = &NoiseChannel;

            want.freq = SAMPLE_RATE; // number of samples per second
            want.format = AUDIO_F32SYS;
            want.channels = 1; // Trying 2
            want.samples = 2048; // buffer-size
            want.userdata = &AudioMixer;

            for (int i=0; i<31; i++)
                AudioMixer.pulseMixTable[i] = 95.52 / (8128.0 / (double)i + 100);
             

            Open_Audio();
        }


        long CpuCycles;
        uint16_t ApuCycles;
        bool FireIRQ;
        void Reg_Write(uint16_t reg, uint8_t data);
        void Channels_Out();
        void Pulse_Out();
        void Open_Audio();        
        uint8_t Reg_Read();

    private:
        //Channel and component structs
        Mixer AudioMixer;

        Pulse PulseOne;
        Envelope PulseOneEnv;
        SweepUnit PulseOneSwp;

        Pulse PulseTwo;
        Envelope PulseTwoEnv;
        SweepUnit PulseTwoSwp;

        Triangle TriChannel;

        Noise NoiseChannel;
        Envelope NoiseEnv;

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
        uint8_t TriLinearCount; //$4008
        uint8_t TriTimerLow; //$400A
        uint8_t TriTimerHigh; //$400B

        //Noise channel
        uint8_t NoiseControl; //$400C
        uint8_t NoiseModePeriod; //$400E
        uint8_t NoiseLength; //$400F

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