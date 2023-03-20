#pragma once

#include <Arduino.h>

class AudioFilter
{
public:
    AudioFilter();

    void IIR_calculateCoefficients(uint32_t sampleRate, int8_t G1, int8_t G2, int8_t G3);
    int16_t* IIR_filterChain0(int16_t iir_in[2], bool clear = false);
    int16_t* IIR_filterChain1(int16_t* iir_in, bool clear = false);
    int16_t* IIR_filterChain2(int16_t* iir_in, bool clear = false);

private:
    typedef enum { LEFTCHANNEL=0, RIGHTCHANNEL=1 } SampleIndex;
    typedef enum { LOWSHELF = 0, PEAKEQ = 1, HIFGSHELF =2 } FilterType;

    typedef struct _filter{
        float a0;
        float a1;
        float a2;
        float b1;
        float b2;
    } filter_t;

    filter_t        m_filter[3];                    // digital filters
    float           m_filterBuff[3][2][2][2];       // IIR filters memory for Audio DSP

};

