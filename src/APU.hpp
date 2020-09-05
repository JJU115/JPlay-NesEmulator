#ifndef APU_H
#define APU_H

#include "Cartridge.hpp"
#include <thread>
#include <chrono>
#include <cmath>
#include <array>
#include <SDL.h>
#include <SDL_keyboard.h>
#include <sstream>

//Class data members: Upper camel case
//Class functions: Uppercase underscore separated
//Local vars: lower camel case
//Global vars: 'g_' preceded lower camel case
//Global constants: '_' separated capitals
const int SAMPLE_RATE = 88200; //44100

const std::array<uint8_t, 32> LENGTH_TABLE = {10, 254, 20, 2, 40, 4, 80, 6,
                                                160, 8, 60, 10, 14, 12, 26, 14,
                                                12, 16, 24, 18, 48, 20, 96, 22,
                                                192, 24, 72, 26, 16, 28, 32, 30};

const std::array<uint16_t, 16> NOISE_PERIOD = {4, 8, 16, 32, 64,
                                                96, 128, 160, 202, 254,
                                                380, 508, 762, 1016, 2034, 4068};  

const std::array<uint8_t, 32> TRIANGLE_WAVE = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                                                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

const std::array<uint16_t, 16> DMC_RATE = {428, 380, 340, 320, 286, 254, 226, 214,
                                            190, 160, 142, 128, 106, 84, 72, 54};


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

    uint8_t getVol() {
        if (constVol)
            return constVolLevel;
        else
            return decayLevel;
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


    uint8_t getSample(long sampleNum) {
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
        if (sample <= 0)
            return 0;
        if (sample >= 1)
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
    double lastSamp;
    long currentSamp;
    uint16_t timerReload;
    
    //Experimental
    uint8_t start;
    uint8_t end;
    uint8_t extra;
    double base;
    float period;


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


    void recalculate() {
        period = round(((32 * (timer + 1))/ 1789773.0f) * SAMPLE_RATE * 2);
        base = period / 32.0f;
        extra = round((base - floor(base)) * 32); //extra values to distribute
        start = (extra % 2 == 0) ? 15-(extra/2) : 16-(extra/2);
        end = 15+(extra/2);
        base = floor(base);
    }

    //Might need to relook at this, the NES sends the values 15 -> 0 then 0 -> 15 as values but the 
    //SDL audio sampling rate makes it difficult to send this discrete sequence without repeating a lot of values
    //given the speed at which the APU clock is running resulting in bad audio. The triangle needs to change or
    //the mixer can't strictly operate as nesdev says it should 
    uint8_t getSample() {
        if (!(lengthCount) || !(linearCount) || timer < 2)
            return lastSamp;
        else { 
            //amplitude 1, no dividing by 2 since the Triangle timer ticks at twice the Pulse timer rate
            //Formula has been slightly adjusted so that wave oscillates between 0 and 1           
            //period = round(((32 * (timer + 1))/ 1789773.0f) * SAMPLE_RATE * 2); //# of samples
            //lastSamp = (2.0 / period * abs((currentSamp++ % (int)period) - period/2.0)); 
            //return lastSamp;
            //return (sequencePos < 15) ? (15 - sequencePos) : (sequencePos - 15);

            if (--base <= 0) {
                sequencePos++;
                sequencePos %= 32;
                if (sequencePos >= start && sequencePos <= end)
                    base = floor(period / 32.0f)+1;
                else
                    base = floor(period / 32.0f);
            }
           
            //std::cout << int(sequencePos) << " ";
            currentSamp++;
            
            lastSamp = TRIANGLE_WAVE[sequencePos];
            /*if (extra >= base) {
                sequencePos++;
                sequencePos %= 32;
                extra = 0;
            }*/
            return lastSamp;
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
    double avg;

    void clock() {
        feedBack = (shiftReg & 0x0001) ^ ((mode) ? ((shiftReg & 0x0040) >> 6) : ((shiftReg & 0x0002) >> 1));
        shiftReg = shiftReg >> 1;
        shiftReg &= 0x3FFF;
        shiftReg |= (feedBack << 14);
        timer = reload;
    }

    double getSample() {
        avg = 0;
        for (int j=0; j<20; j++) {
            if (timer-- == 0) {
                timer = reload;
                clock();
            }
            avg += feedBack;
        }
        avg = round(avg / 20.0f);//*noiseEnvelope->getVol();
        if (lengthCount == 0 || shiftReg & 0x0001)
            return 0;
        else {
            return static_cast<double>(avg);
        }
    }
};


struct DMC {
    uint8_t buffer;
    uint8_t output;
    uint8_t sampAddress; //Reg $4012
    uint8_t sampLength; //Reg $4013
    uint8_t bytesRemain;
    uint8_t shiftReg;
    uint8_t bitsRemain;

    uint16_t rate;
    uint16_t reload;
    uint16_t currAddress;

    bool enabled;
    bool IRQEnabled;
    bool loop;
    bool silence;
    bool bufferEmpty;
    bool interrupt;

    Cartridge* ROM;


    //When the timer hits 0 it clocks the output unit
    //Note the timer is decremented every APU cycle
    void clock() {
        rate = reload;
        if (!silence) {
            if (shiftReg & 0x01)
                output += (output < 126) ? 2 : 0;
            else
                output -= (output > 1) ? 2 : 0;
        }    

        shiftReg = shiftReg >> 1;
        if (bitsRemain == 0) {
            bitsRemain = 8;
            if (bufferEmpty) {
                silence = true;
            } else {
                silence = false;
                shiftReg = buffer;
                bufferEmpty = true;
            }
        } else {
            bitsRemain--;
        }

        //std::cout << "clocked\n";
        
    }

    void memoryRead() {
        //Fill buffer with read memory
        buffer = ROM->CPU_ACCESS(currAddress);
        bufferEmpty = false;

        currAddress = (currAddress == 0xFFFF) ? 0x8000 : (currAddress + 1);
        bytesRemain--;
        if (!bytesRemain && loop) {
            currAddress = sampAddress;
            bytesRemain = sampLength;
        } else if (!bytesRemain && IRQEnabled)
            interrupt = true;
    }
};



//The mix audio function has to take inputs from all channels and turn them into a single audio signal
//Lookup tables with values specified by nesdev are used
//Doing the noise channel separately for now since can't produce good enough noise with current model
struct Mixer {
    Pulse *pulseOne;
    Pulse *pulseTwo;
    Triangle *tri;
    Noise *noise;
    DMC *dmc;

    std::array<double, 31> pulseMixTable;
    std::array<double, 203> tndTable; //Reduce size of this since noise channel output no longer used to calculate index 
    double mixAudio(long sampleNum) {
        double pulseOut = pulseMixTable[pulseOne->getSample(sampleNum) + pulseTwo->getSample(sampleNum)]; 
        double tndOut = tndTable[3 * tri->getSample() + dmc->output] + noise->getSample();
        return pulseOut + tndOut;// + noise->getSample();
    }
};


class APU {
    public:
        APU(Cartridge& NES): Pulse1Control(0), Pulse1Sweep(0), Pulse1TimeLow(0), Pulse1TimeHigh(0), 
                Pulse2Control(0), Pulse2Sweep(0), Pulse2TimeLow(0), Pulse2TimeHigh(0),
                TriLinearCount(0), TriTimerHigh(0), TriTimerLow(0), NoiseControl(0),
                NoiseModePeriod(0), NoiseLength(0), ControlStatus(0), FrameCount(0),
                CpuCycles(0), ApuCycles(0), Jitter(0), JitterWrite(false), FrameInterrupt(false), FireIRQ(false) {
              
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
            TriChannel.lastSamp = 15;

            NoiseChannel = {};
            NoiseEnv = {};
            NoiseChannel.shiftReg = 1;
            NoiseChannel.noiseEnvelope = &NoiseEnv;

            DMCChannel = {};
            DMCChannel.output = 0;
            DMCChannel.bufferEmpty = true;
            DMCChannel.ROM = &NES;

            AudioMixer.pulseOne = &PulseOne;
            AudioMixer.pulseTwo = &PulseTwo;
            AudioMixer.tri = &TriChannel;
            AudioMixer.noise = &NoiseChannel;
            AudioMixer.dmc = &DMCChannel;

            want.freq = SAMPLE_RATE; // number of samples per second
            want.format = AUDIO_F32SYS;
            want.channels = 1; // Trying 2
            want.samples = 2048; // buffer-size
            want.userdata = &AudioMixer;

            for (int i=0; i<31; i++)
                AudioMixer.pulseMixTable[i] = 95.52 / (8128.0 / (double)i + 100);
                
            for (int i=0; i<203; i++)
                AudioMixer.tndTable[i] = 163.67 / (24329.0 / ((double)i + 100));
             
            LOG.open("APU_LOG.txt", std::ios::trunc | std::ios::out);
            Open_Audio();
        }

        std::ofstream LOG;
        long CpuCycles;
        uint16_t ApuCycles;
        uint8_t Jitter;
        bool JitterWrite;
        bool FireIRQ;
        void Reg_Write(uint16_t reg, uint8_t data);
        void Channels_Out();
        bool Pulse_Out();
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

        DMC DMCChannel;

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