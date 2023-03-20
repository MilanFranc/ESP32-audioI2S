#include "AudioFilter.h"

AudioFilter::AudioFilter()
{
    for(int i = 0; i <3; i++) {
        m_filter[i].a0  = 1;
        m_filter[i].a1  = 0;
        m_filter[i].a2  = 0;
        m_filter[i].b1  = 0;
        m_filter[i].b2  = 0;
    }
}


//---------------------------------------------------------------------------------------------------------------------
//            ***     D i g i t a l   b i q u a d r a t i c     f i l t e r     ***
//---------------------------------------------------------------------------------------------------------------------
void AudioFilter::IIR_calculateCoefficients(uint32_t sampleRate, int8_t G0, int8_t G1, int8_t G2) {  // Infinite Impulse Response (IIR) filters

    // G1 - gain low shelf   set between -40 ... +6 dB
    // G2 - gain peakEQ      set between -40 ... +6 dB
    // G3 - gain high shelf  set between -40 ... +6 dB
    // https://www.earlevel.com/main/2012/11/26/biquad-c-source-code/

    if (sampleRate < 1000) return;  // fuse

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if(G0 < -40) G0 = -40;      // -40dB -> Vin*0.01
    if(G0 > 6) G0 = 6;          // +6dB -> Vin*2
    if(G1 < -40) G1 = -40;
    if(G1 > 6) G1 = 6;
    if(G2 < -40) G2 = -40;
    if(G2 > 6) G2 = 6;

    const float FcLS   =  500;  // Frequency LowShelf[Hz]
    const float FcPKEQ = 3000;  // Frequency PeakEQ[Hz]
    const float FcHS   = 6000;  // Frequency HighShelf[Hz]

    float K, norm, Q, Fc, V ;

    // LOWSHELF
    Fc = (float)FcLS / (float)sampleRate; // Cutoff frequency
    K = tanf((float)PI * Fc);
    V = powf(10, fabs(G0) / 20.0);

    if (G0 >= 0) {  // boost
        norm = 1 / (1 + sqrtf(2) * K + K * K);
        m_filter[LOWSHELF].a0 = (1 + sqrtf(2*V) * K + V * K * K) * norm;
        m_filter[LOWSHELF].a1 = 2 * (V * K * K - 1) * norm;
        m_filter[LOWSHELF].a2 = (1 - sqrtf(2*V) * K + V * K * K) * norm;
        m_filter[LOWSHELF].b1 = 2 * (K * K - 1) * norm;
        m_filter[LOWSHELF].b2 = (1 - sqrtf(2) * K + K * K) * norm;
    }
    else {          // cut
        norm = 1 / (1 + sqrtf(2*V) * K + V * K * K);
        m_filter[LOWSHELF].a0 = (1 + sqrtf(2) * K + K * K) * norm;
        m_filter[LOWSHELF].a1 = 2 * (K * K - 1) * norm;
        m_filter[LOWSHELF].a2 = (1 - sqrtf(2) * K + K * K) * norm;
        m_filter[LOWSHELF].b1 = 2 * (V * K * K - 1) * norm;
        m_filter[LOWSHELF].b2 = (1 - sqrtf(2*V) * K + V * K * K) * norm;
    }

    // PEAK EQ
    Fc = (float)FcPKEQ / (float)sampleRate; // Cutoff frequency
    K = tanf((float)PI * Fc);
    V = powf(10, fabs(G1) / 20.0);
    Q = 2.5; // Quality factor
    if (G1 >= 0) { // boost
        norm = 1 / (1 + 1/Q * K + K * K);
        m_filter[PEAKEQ].a0 = (1 + V/Q * K + K * K) * norm;
        m_filter[PEAKEQ].a1 = 2 * (K * K - 1) * norm;
        m_filter[PEAKEQ].a2 = (1 - V/Q * K + K * K) * norm;
        m_filter[PEAKEQ].b1 = m_filter[PEAKEQ].a1;
        m_filter[PEAKEQ].b2 = (1 - 1/Q * K + K * K) * norm;
    }
    else {    // cut
        norm = 1 / (1 + V/Q * K + K * K);
        m_filter[PEAKEQ].a0 = (1 + 1/Q * K + K * K) * norm;
        m_filter[PEAKEQ].a1 = 2 * (K * K - 1) * norm;
        m_filter[PEAKEQ].a2 = (1 - 1/Q * K + K * K) * norm;
        m_filter[PEAKEQ].b1 = m_filter[PEAKEQ].a1;
        m_filter[PEAKEQ].b2 = (1 - V/Q * K + K * K) * norm;
    }

    // HIGHSHELF
    Fc = (float)FcHS / (float)sampleRate; // Cutoff frequency
    K = tanf((float)PI * Fc);
    V = powf(10, fabs(G2) / 20.0);
    if (G2 >= 0) {  // boost
        norm = 1 / (1 + sqrtf(2) * K + K * K);
        m_filter[HIFGSHELF].a0 = (V + sqrtf(2*V) * K + K * K) * norm;
        m_filter[HIFGSHELF].a1 = 2 * (K * K - V) * norm;
        m_filter[HIFGSHELF].a2 = (V - sqrtf(2*V) * K + K * K) * norm;
        m_filter[HIFGSHELF].b1 = 2 * (K * K - 1) * norm;
        m_filter[HIFGSHELF].b2 = (1 - sqrtf(2) * K + K * K) * norm;
    }
    else {
        norm = 1 / (V + sqrtf(2*V) * K + K * K);
        m_filter[HIFGSHELF].a0 = (1 + sqrtf(2) * K + K * K) * norm;
        m_filter[HIFGSHELF].a1 = 2 * (K * K - 1) * norm;
        m_filter[HIFGSHELF].a2 = (1 - sqrtf(2) * K + K * K) * norm;
        m_filter[HIFGSHELF].b1 = 2 * (K * K - V) * norm;
        m_filter[HIFGSHELF].b2 = (V - sqrtf(2*V) * K + K * K) * norm;
    }

//    log_i("LS a0=%f, a1=%f, a2=%f, b1=%f, b2=%f", m_filter[0].a0, m_filter[0].a1, m_filter[0].a2,
//                                                  m_filter[0].b1, m_filter[0].b2);
//    log_i("EQ a0=%f, a1=%f, a2=%f, b1=%f, b2=%f", m_filter[1].a0, m_filter[1].a1, m_filter[1].a2,
//                                                  m_filter[1].b1, m_filter[1].b2);
//    log_i("HS a0=%f, a1=%f, a2=%f, b1=%f, b2=%f", m_filter[2].a0, m_filter[2].a1, m_filter[2].a2,
//                                                  m_filter[2].b1, m_filter[2].b2);
}
//---------------------------------------------------------------------------------------------------------------------
int16_t* AudioFilter::IIR_filterChain0(int16_t iir_in[2], bool clear) {  // Infinite Impulse Response (IIR) filters

    uint8_t z1 = 0, z2 = 1;
    enum: uint8_t {in = 0, out = 1};
    float inSample[2];
    float outSample[2];
    static int16_t iir_out[2];

    if(clear){
        memset(m_filterBuff, 0, sizeof(m_filterBuff));            // zero IIR filterbuffer
        iir_out[0] = 0;
        iir_out[1] = 0;
        iir_in[0]  = 0;
        iir_in[1]  = 0;
    }

    inSample[LEFTCHANNEL]  = (float)(iir_in[LEFTCHANNEL]);
    inSample[RIGHTCHANNEL] = (float)(iir_in[RIGHTCHANNEL]);

    outSample[LEFTCHANNEL] =   m_filter[0].a0  * inSample[LEFTCHANNEL]
                             + m_filter[0].a1  * m_filterBuff[0][z1][in] [LEFTCHANNEL]
                             + m_filter[0].a2  * m_filterBuff[0][z2][in] [LEFTCHANNEL]
                             - m_filter[0].b1  * m_filterBuff[0][z1][out][LEFTCHANNEL]
                             - m_filter[0].b2  * m_filterBuff[0][z2][out][LEFTCHANNEL];

    m_filterBuff[0][z2][in] [LEFTCHANNEL]  = m_filterBuff[0][z1][in][LEFTCHANNEL];
    m_filterBuff[0][z1][in] [LEFTCHANNEL]  = inSample[LEFTCHANNEL];
    m_filterBuff[0][z2][out][LEFTCHANNEL]  = m_filterBuff[0][z1][out][LEFTCHANNEL];
    m_filterBuff[0][z1][out][LEFTCHANNEL]  = outSample[LEFTCHANNEL];
    iir_out[LEFTCHANNEL] = (int16_t)outSample[LEFTCHANNEL];


    outSample[RIGHTCHANNEL] =  m_filter[0].a0 * inSample[RIGHTCHANNEL]
                             + m_filter[0].a1 * m_filterBuff[0][z1][in] [RIGHTCHANNEL]
                             + m_filter[0].a2 * m_filterBuff[0][z2][in] [RIGHTCHANNEL]
                             - m_filter[0].b1 * m_filterBuff[0][z1][out][RIGHTCHANNEL]
                             - m_filter[0].b2 * m_filterBuff[0][z2][out][RIGHTCHANNEL];

    m_filterBuff[0][z2][in] [RIGHTCHANNEL] = m_filterBuff[0][z1][in][RIGHTCHANNEL];
    m_filterBuff[0][z1][in] [RIGHTCHANNEL] = inSample[RIGHTCHANNEL];
    m_filterBuff[0][z2][out][RIGHTCHANNEL] = m_filterBuff[0][z1][out][RIGHTCHANNEL];
    m_filterBuff[0][z1][out][RIGHTCHANNEL] = outSample[RIGHTCHANNEL];
    iir_out[RIGHTCHANNEL] = (int16_t) outSample[RIGHTCHANNEL];

    return iir_out;
}
//---------------------------------------------------------------------------------------------------------------------
int16_t* AudioFilter::IIR_filterChain1(int16_t iir_in[2], bool clear){  // Infinite Impulse Response (IIR) filters

    uint8_t z1 = 0, z2 = 1;
    enum: uint8_t {in = 0, out = 1};
    float inSample[2];
    float outSample[2];
    static int16_t iir_out[2];

    if(clear){
        memset(m_filterBuff, 0, sizeof(m_filterBuff));            // zero IIR filterbuffer
        iir_out[0] = 0;
        iir_out[1] = 0;
        iir_in[0]  = 0;
        iir_in[1]  = 0;
    }

    inSample[LEFTCHANNEL]  = (float)(iir_in[LEFTCHANNEL]);
    inSample[RIGHTCHANNEL] = (float)(iir_in[RIGHTCHANNEL]);

    outSample[LEFTCHANNEL] =   m_filter[1].a0  * inSample[LEFTCHANNEL]
                             + m_filter[1].a1  * m_filterBuff[1][z1][in] [LEFTCHANNEL]
                             + m_filter[1].a2  * m_filterBuff[1][z2][in] [LEFTCHANNEL]
                             - m_filter[1].b1  * m_filterBuff[1][z1][out][LEFTCHANNEL]
                             - m_filter[1].b2  * m_filterBuff[1][z2][out][LEFTCHANNEL];

    m_filterBuff[1][z2][in] [LEFTCHANNEL]  = m_filterBuff[1][z1][in][LEFTCHANNEL];
    m_filterBuff[1][z1][in] [LEFTCHANNEL]  = inSample[LEFTCHANNEL];
    m_filterBuff[1][z2][out][LEFTCHANNEL]  = m_filterBuff[1][z1][out][LEFTCHANNEL];
    m_filterBuff[1][z1][out][LEFTCHANNEL]  = outSample[LEFTCHANNEL];
    iir_out[LEFTCHANNEL] = (int16_t)outSample[LEFTCHANNEL];


    outSample[RIGHTCHANNEL] =  m_filter[1].a0 * inSample[RIGHTCHANNEL]
                             + m_filter[1].a1 * m_filterBuff[1][z1][in] [RIGHTCHANNEL]
                             + m_filter[1].a2 * m_filterBuff[1][z2][in] [RIGHTCHANNEL]
                             - m_filter[1].b1 * m_filterBuff[1][z1][out][RIGHTCHANNEL]
                             - m_filter[1].b2 * m_filterBuff[1][z2][out][RIGHTCHANNEL];

    m_filterBuff[1][z2][in] [RIGHTCHANNEL] = m_filterBuff[1][z1][in][RIGHTCHANNEL];
    m_filterBuff[1][z1][in] [RIGHTCHANNEL] = inSample[RIGHTCHANNEL];
    m_filterBuff[1][z2][out][RIGHTCHANNEL] = m_filterBuff[1][z1][out][RIGHTCHANNEL];
    m_filterBuff[1][z1][out][RIGHTCHANNEL] = outSample[RIGHTCHANNEL];
    iir_out[RIGHTCHANNEL] = (int16_t) outSample[RIGHTCHANNEL];

    return iir_out;
}
//---------------------------------------------------------------------------------------------------------------------
int16_t* AudioFilter::IIR_filterChain2(int16_t iir_in[2], bool clear){  // Infinite Impulse Response (IIR) filters

    uint8_t z1 = 0, z2 = 1;
    enum: uint8_t { in = 0, out = 1 };
    float inSample[2];
    float outSample[2];
    static int16_t iir_out[2];

    if(clear){
        memset(m_filterBuff, 0, sizeof(m_filterBuff));            // zero IIR filterbuffer
        iir_out[0] = 0;
        iir_out[1] = 0;
        iir_in[0]  = 0;
        iir_in[1]  = 0;
    }

    inSample[LEFTCHANNEL]  = (float)(iir_in[LEFTCHANNEL]);
    inSample[RIGHTCHANNEL] = (float)(iir_in[RIGHTCHANNEL]);

    outSample[LEFTCHANNEL] =   m_filter[2].a0  * inSample[LEFTCHANNEL]
                             + m_filter[2].a1  * m_filterBuff[2][z1][in] [LEFTCHANNEL]
                             + m_filter[2].a2  * m_filterBuff[2][z2][in] [LEFTCHANNEL]
                             - m_filter[2].b1  * m_filterBuff[2][z1][out][LEFTCHANNEL]
                             - m_filter[2].b2  * m_filterBuff[2][z2][out][LEFTCHANNEL];

    m_filterBuff[2][z2][in] [LEFTCHANNEL]  = m_filterBuff[2][z1][in][LEFTCHANNEL];
    m_filterBuff[2][z1][in] [LEFTCHANNEL]  = inSample[LEFTCHANNEL];
    m_filterBuff[2][z2][out][LEFTCHANNEL]  = m_filterBuff[2][z1][out][LEFTCHANNEL];
    m_filterBuff[2][z1][out][LEFTCHANNEL]  = outSample[LEFTCHANNEL];
    iir_out[LEFTCHANNEL] = (int16_t)outSample[LEFTCHANNEL];


    outSample[RIGHTCHANNEL] =  m_filter[2].a0 * inSample[RIGHTCHANNEL]
                             + m_filter[2].a1 * m_filterBuff[2][z1][in] [RIGHTCHANNEL]
                             + m_filter[2].a2 * m_filterBuff[2][z2][in] [RIGHTCHANNEL]
                             - m_filter[2].b1 * m_filterBuff[2][z1][out][RIGHTCHANNEL]
                             - m_filter[2].b2 * m_filterBuff[2][z2][out][RIGHTCHANNEL];

    m_filterBuff[2][z2][in] [RIGHTCHANNEL] = m_filterBuff[2][z1][in][RIGHTCHANNEL];
    m_filterBuff[2][z1][in] [RIGHTCHANNEL] = inSample[RIGHTCHANNEL];
    m_filterBuff[2][z2][out][RIGHTCHANNEL] = m_filterBuff[2][z1][out][RIGHTCHANNEL];
    m_filterBuff[2][z1][out][RIGHTCHANNEL] = outSample[RIGHTCHANNEL];
    iir_out[RIGHTCHANNEL] = (int16_t) outSample[RIGHTCHANNEL];

    return iir_out;
}



