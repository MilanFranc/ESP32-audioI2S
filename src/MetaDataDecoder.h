#pragma once

#include <Arduino.h>

class MetaDataDecoder
{
public:
    MetaDataDecoder();
    void reset();
    bool addData(uint8_t b);

private:
    uint16_t m_index;                          // determines the current position in metaline
    uint16_t m_datalen;
    bool m_f_swm = false;

    char  m_buffer[512];
};

