#pragma once

class AudioPlaylist
{
public:
    AudioPlaylist() = default;

    void processPlayListData();

private:
    bool STfromEXTINF(char* str);

private:
    char            chbuf[512 + 128];               // must be greater than m_lastHost #254

};

