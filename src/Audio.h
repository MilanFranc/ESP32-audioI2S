/*
 * Audio.h
 *
 *  Created on: Oct 26,2018
 *  Updated on: May 19,2022
 *      Author: Wolle (schreibfaul1)
 */

//#define SDFATFS_USED  // activate for SdFat


#pragma once
#pragma GCC optimize ("Ofast")

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <libb64/cencode.h>

#include <driver/i2s.h>

#ifndef AUDIO_NO_SD_FS
#include <SPI.h>
#ifdef SDFATFS_USED
#include <SdFat.h>  // https://github.com/greiman/SdFat
#else
#include <SD.h>
#include <SD_MMC.h>
#include <SPIFFS.h>
#include <FS.h>
#include <FFat.h>
#endif // SDFATFS_USED

#include "AudioBuffer.h"
#include "AudioFilter.h"


#ifdef SDFATFS_USED
//typedef File32 File;
typedef FsFile File;

namespace fs {
    class FS : public SdFat {
    public:
        bool begin(SdCsPin_t csPin = SS, uint32_t maxSck = SD_SCK_MHZ(25)) { return SdFat::begin(csPin, maxSck); }
    };

    class SDFATFS : public fs::FS {
    public:
        // sdcard_type_t cardType();
        uint64_t cardSize() {
            return totalBytes();
        }
        uint64_t usedBytes() {
            // set SdFatConfig MAINTAIN_FREE_CLUSTER_COUNT non-zero. Then only the first call will take time.
            return (uint64_t)(clusterCount() - freeClusterCount()) * (uint64_t)bytesPerCluster();
        }
        uint64_t totalBytes() {
            return (uint64_t)clusterCount() * (uint64_t)bytesPerCluster();
        }
    };
}

extern fs::SDFATFS SD_SDFAT;

using namespace fs;
#define SD SD_SDFAT
#endif //SDFATFS_USED
#endif // AUDIO_NO_SD_FS

extern __attribute__((weak)) void audio_info(const char*);
extern __attribute__((weak)) void audio_id3data(const char*); //ID3 metadata
#ifndef AUDIO_NO_SD_FS
extern __attribute__((weak)) void audio_id3image(File& file, const size_t pos, const size_t size); //ID3 metadata image
#endif                                                                                             // AUDIO_NO_SD_FS
extern __attribute__((weak)) void audio_eof_mp3(const char*); //end of mp3 file
extern __attribute__((weak)) void audio_showstreamtitle(const char*);
extern __attribute__((weak)) void audio_showstation(const char*);
extern __attribute__((weak)) void audio_bitrate(const char*);
extern __attribute__((weak)) void audio_commercial(const char*);
extern __attribute__((weak)) void audio_icyurl(const char*);
extern __attribute__((weak)) void audio_icydescription(const char*);
extern __attribute__((weak)) void audio_lasthost(const char*);
extern __attribute__((weak)) void audio_eof_speech(const char*);
extern __attribute__((weak)) void audio_eof_stream(const char*); // The webstream comes to an end
extern __attribute__((weak)) void audio_process_extern(int16_t* buff, uint16_t len, bool *continueI2S); // record audiodata or send via BT


//----------------------------------------------------------------------------------------------------------------------

class Audio : private AudioBuffer{

    AudioBuffer InBuff; // instance of input buffer

public:
    Audio(bool internalDAC = false, uint8_t channelEnabled = 3, uint8_t i2sPort = I2S_NUM_0); // #99
    ~Audio();
    void setBufsize(int rambuf_sz, int psrambuf_sz);
    bool connecttohost(const char* host, const char* user = "", const char* pwd = "");
    bool connecttospeech(const char* speech, const char* lang);
#ifndef AUDIO_NO_SD_FS
    bool connecttoFS(fs::FS &fs, const char* path, uint32_t resumeFilePos = 0);
    bool connecttoSD(const char* path, uint32_t resumeFilePos = 0);
#endif                           // AUDIO_NO_SD_FS
    bool setFileLoop(bool input);//TEST loop
    void setConnectionTimeout(uint16_t timeout_ms, uint16_t timeout_ms_ssl);
    bool setAudioPlayPosition(uint16_t sec);
    bool setFilePos(uint32_t pos);
    bool audioFileSeek(const float speed);
    bool setTimeOffset(int sec);
    bool setPinout(uint8_t BCLK, uint8_t LRC, uint8_t DOUT, int8_t DIN=I2S_PIN_NO_CHANGE);
    bool pauseResume();
    bool isRunning() {return m_f_running;}
    void loop();
    uint32_t stopSong();
    void forceMono(bool m);
    void setBalance(int8_t bal = 0);
    void setVolume(uint8_t vol);
    uint8_t getVolume();
	uint8_t getI2sPort();

    uint32_t getAudioDataStartPos();
    uint32_t getFileSize();
    uint32_t getFilePos();
    uint32_t getSampleRate();
    uint8_t  getBitsPerSample();
    uint8_t  getChannels();
    uint32_t getBitRate(bool avg = false);
    uint32_t getAudioFileDuration();
    uint32_t getAudioCurrentTime();
    uint32_t getTotalPlayingTime();

    esp_err_t i2s_mclk_pin_select(const uint8_t pin);
    uint32_t inBufferFilled(); // returns the number of stored bytes in the inputbuffer
    uint32_t inBufferFree();   // returns the number of free bytes in the inputbuffer
    void setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass);
    void setI2SCommFMT_LSB(bool commFMT);
    int getCodec() const { return m_codec;}
    const char* getCodecname() const;
    enum : int { CODEC_NONE, CODEC_WAV, CODEC_MP3, CODEC_AAC, CODEC_M4A, CODEC_FLAC, CODEC_OGG,
                 CODEC_OGG_FLAC, CODEC_OGG_OPUS};

private:
    void httpPrint(const char* url);
    void setDefaults(); // free buffers and set defaults
    void initInBuff();
#ifndef AUDIO_NO_SD_FS
    void processLocalFile();
#endif // AUDIO_NO_SD_FS
    void processWebStream();
    void processPlayListData();
    void processM3U8entries(uint8_t nrOfEntries = 0, uint32_t seqNr = 0, uint8_t pos = 0, uint16_t targetDuration = 0);
    bool STfromEXTINF(char* str);
    void showCodecParams();
    int  findNextSync(uint8_t* data, size_t len);
    int  sendBytes(uint8_t* data, size_t len);
    void compute_audioCurrentTime(int bd);
    void printDecodeError(int r);
    void showID3Tag(const char* tag, const char* val);
    int  read_WAV_Header(uint8_t* data, size_t len);
    int  read_FLAC_Header(uint8_t *data, size_t len);
    int  read_MP3_Header(uint8_t* data, size_t len);
    int  read_M4A_Header(uint8_t* data, size_t len);
    int  read_OGG_Header(uint8_t *data, size_t len);
    bool setSampleRate(uint32_t hz);
    bool setBitsPerSample(int bits);
    bool setChannels(int channels);
    bool setBitrate(int br);
    bool playChunk();
    bool playSample(int16_t sample[2]) ;
    void playI2Sremains();
    int32_t Gain(int16_t s[2]);
    bool fill_InputBuf();
    void showstreamtitle(const char* ml);
    bool parseContentType(const char* ct);
    void processAudioHeaderData();
    bool readMetadata(uint8_t b, bool first = false);
    esp_err_t I2Sstart(uint8_t i2s_num);
    esp_err_t I2Sstop(uint8_t i2s_num);
    inline void setDatamode(uint8_t dm) { m_datamode = dm; }
    inline uint8_t getDatamode() { return m_datamode; }
    inline uint32_t streamavail() { return _client ? _client->available() : 0; }


private:
    enum : int { APLL_AUTO = -1, APLL_ENABLE = 1, APLL_DISABLE = 0 };
    enum : int { EXTERNAL_I2S = 0, INTERNAL_DAC = 1, INTERNAL_PDM = 2 };
    enum : int { FORMAT_NONE = 0, FORMAT_M3U = 1, FORMAT_PLS = 2, FORMAT_ASX = 3, FORMAT_M3U8 = 4};
    enum : int { AUDIO_NONE, AUDIO_HEADER, AUDIO_DATA,
                 AUDIO_PLAYLISTINIT, AUDIO_PLAYLISTHEADER,  AUDIO_PLAYLISTDATA};
    enum : int { FLAC_BEGIN = 0, FLAC_MAGIC = 1, FLAC_MBH =2, FLAC_SINFO = 3, FLAC_PADDING = 4, FLAC_APP = 5,
                 FLAC_SEEK = 6, FLAC_VORBIS = 7, FLAC_CUESHEET = 8, FLAC_PICTURE = 9, FLAC_OKAY = 100};
    enum : int { M4A_BEGIN = 0, M4A_FTYP = 1, M4A_CHK = 2, M4A_MOOV = 3, M4A_FREE = 4, M4A_TRAK = 5, M4A_MDAT = 6,
                 M4A_ILST = 7, M4A_MP4A = 8, M4A_AMRDY = 99, M4A_OKAY = 100};
    enum : int { OGG_BEGIN = 0, OGG_MAGIC = 1, OGG_HEADER = 2, OGG_FIRST = 3, OGG_AMRDY = 99, OGG_OKAY = 100};
    typedef enum { LEFTCHANNEL=0, RIGHTCHANNEL=1 } SampleIndex;

    const uint8_t volumetable[22]={   0,  1,  2,  3,  4 , 6 , 8, 10, 12, 14, 17,
                                     20, 23, 27, 30 ,34, 38, 43 ,48, 52, 58, 64}; //22 elements


#ifndef AUDIO_NO_SD_FS
    File              audiofile;    // @suppress("Abstract class cannot be instantiated")
#endif                              // AUDIO_NO_SD_FS
    WiFiClient        client;       // @suppress("Abstract class cannot be instantiated")
    WiFiClientSecure  clientsecure; // @suppress("Abstract class cannot be instantiated")
    WiFiClient*       _client = nullptr;
    i2s_config_t      m_i2s_config; // stores values for I2S driver
    i2s_pin_config_t  m_pin_config;

    const size_t    m_frameSizeWav  = 1600;
    const size_t    m_frameSizeMP3  = 1600;
    const size_t    m_frameSizeAAC  = 1600;
    const size_t    m_frameSizeFLAC = 4096 * 4;

    char            chbuf[512 + 128];               // must be greater than m_lastHost #254
    char            m_lastHost[512];                // Store the last URL to a webstream
    char*           m_playlistBuff = NULL;          // stores playlistdata
    const uint16_t  m_plsBuffEntryLen = 256;        // length of each entry in playlistBuff

    int             m_LFcount = 0;                  // Detection of end of header
    uint32_t        m_sampleRate=16000;
    uint32_t        m_bitRate=0;                    // current bitrate given fom decoder
    uint32_t        m_avr_bitrate = 0;              // average bitrate, median computed by VBR
    int             m_readbytes=0;                  // bytes read
    int             m_metalen=0;                    // Number of bytes in metadata
    int             m_controlCounter = 0;           // Status within readID3data() and readWaveHeader()
    int8_t          m_balance = 0;                  // -16 (mute left) ... +16 (mute right)
    uint8_t         m_vol=64;                       // volume
    uint8_t         m_bitsPerSample = 16;           // bitsPerSample
    uint8_t         m_channels=2;
    uint8_t         m_i2s_num = I2S_NUM_0;          // I2S_NUM_0 or I2S_NUM_1
    uint8_t         m_playlistFormat = 0;           // M3U, PLS, ASX
    uint8_t         m_m3u8codec = CODEC_NONE;       // M4A
    uint8_t         m_codec = CODEC_NONE;           //
    uint8_t         m_filterType[2];                // lowpass, highpass
    int16_t         m_outBuff[2048*2];              // Interleaved L/R
    int16_t         m_validSamples = 0;
    int16_t         m_curSample = 0;
    uint16_t        m_datamode = 0;                 // Statemaschine
    uint16_t        m_streamTitleHash = 0;          // remember streamtitle, ignore multiple occurence in metadata
    uint16_t        m_streamUrlHash = 0;            // remember streamURL, ignore multiple occurence in metadata
    uint16_t        m_timeout_ms = 250;
    uint16_t        m_timeout_ms_ssl = 2700;
    uint8_t         m_flacBitsPerSample = 0;        // bps should be 16
    uint8_t         m_flacNumChannels = 0;          // can be read out in the FLAC file header
    uint32_t        m_flacSampleRate = 0;           // can be read out in the FLAC file header
    uint16_t        m_flacMaxFrameSize = 0;         // can be read out in the FLAC file header
    uint16_t        m_flacMaxBlockSize = 0;         // can be read out in the FLAC file header
    uint32_t        m_flacTotalSamplesInStream = 0; // can be read out in the FLAC file header
    uint32_t        m_metaint = 0;                  // Number of databytes between metadata
    uint32_t        m_chunkcount = 0 ;              // Counter for chunked transfer
    uint32_t        m_t0 = 0;                       // store millis(), is needed for a small delay
    uint32_t        m_contentlength = 0;            // Stores the length if the stream comes from fileserver
    uint32_t        m_bytesNotDecoded = 0;          // pictures or something else that comes with the stream
    uint32_t        m_PlayingStartTime = 0;         // Stores the milliseconds after the start of the audio
    uint32_t        m_resumeFilePos = 0;            // the return value from stopSong() can be entered here
    bool            m_f_swm = true;                 // Stream without metadata
    bool            m_f_unsync = false;             // set within ID3 tag but not used
    bool            m_f_exthdr = false;             // ID3 extended header
    bool            m_f_localfile = false ;         // Play from local mp3-file
    bool            m_f_webstream = false ;         // Play from URL
    bool            m_f_ssl = false;
    bool            m_f_running = false;
    bool            m_f_firstCall = false;          // InitSequence for processWebstream and processLokalFile
    bool            m_f_ctseen = false;             // First line of header seen or not
    bool            m_f_chunked = false ;           // Station provides chunked transfer
    bool            m_f_firstmetabyte = false;      // True if first metabyte (counter)
    bool            m_f_playing = false;            // valid mp3 stream recognized
    bool            m_f_webfile = false;            // assume it's a radiostream, not a podcast
    bool            m_f_tts = false;                // text to speech
    bool            m_f_loop = false;               // Set if audio file should loop
    bool            m_f_forceMono = false;          // if true stereo -> mono
    bool            m_f_internalDAC = false;        // false: output vis I2S, true output via internal DAC
    bool            m_f_rtsp = false;               // set if RTSP is used (m3u8 stream)
    bool            m_f_m3u8data = false;           // used in processM3U8entries
    bool            m_f_Log = true;                 // if m3u8: log is cancelled
    bool            m_f_continue = false;           // next m3u8 chunk is available
    uint8_t         m_f_channelEnabled = 3;         // internal DAC, both channels
    uint32_t        m_audioFileDuration = 0;
    float           m_audioCurrentTime = 0;
    uint32_t        m_audioDataStart = 0;           // in bytes
    size_t          m_audioDataSize = 0;            //
    size_t          m_i2s_bytesWritten = 0;         // set in i2s_write() but not used
    size_t          m_file_size = 0;                // size of the file
    uint16_t        m_filterFrequency[2];
    int8_t          m_gain0 = 0;                    // cut or boost filters (EQ)
    int8_t          m_gain1 = 0;
    int8_t          m_gain2 = 0;

    AudioFilter     m_filter;

};

//----------------------------------------------------------------------------------------------------------------------
