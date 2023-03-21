#include "Utils.h"
#include <Arduino.h>
#include <libb64/cencode.h>


namespace utils {

// implement several function with respect to the index of string
void trim(char *s) {
//fb   trim in place
    char *pe;
    char *p = s;
    while ( isspace(*p) ) p++; //left
    pe = p; //right
    while ( *pe != '\0' ) pe++;
    do {
        pe--;
    } while ( (pe > p) && isspace(*pe) );
    if (p == s) {
        *++pe = '\0';
    } else {  //move
        while ( p <= pe ) *s++ = *p++;
        *s = '\0';
    }
}

bool startsWith (const char* base, const char* str) {
//fb
    char c;
    while ( (c = *str++) != '\0' )
        if (c != *base++) return false;
    return true;
}

bool endsWith (const char* base, const char* str) {
//fb
    int slen = strlen(str) - 1;
    const char *p = base + strlen(base) - 1;
    while(p > base && isspace(*p)) p--;  // rtrim
    p -= slen;
    if (p < base) return false;
    return (strncmp(p, str, slen) == 0);
}

int indexOf(const char* base, const char* str, int startIndex) {
//fb
    const char *p = base;
    for (; startIndex > 0; startIndex--)
        if (*p++ == '\0') return -1;
    char* pos = strstr(p, str);
    if (pos == nullptr) return -1;
    return pos - base;
}

int indexOf (const char* base, char ch, int startIndex) {
//fb
    const char *p = base;
    for (; startIndex > 0; startIndex--)
        if (*p++ == '\0') return -1;
    char *pos = strchr(p, ch);
    if (pos == nullptr) return -1;
    return pos - base;
}

int lastIndexOf(const char* haystack, const char* needle) {
//fb
    int nlen = strlen(needle);
    if (nlen == 0) return -1;
    const char *p = haystack - nlen + strlen(haystack);
    while (p >= haystack) {
        int i = 0;
        while (needle[i] == p[i])
        if (++i == nlen) return p - haystack;
        p--;
    }
    return -1;
}

int lastIndexOf(const char* haystack, const char needle) {
//fb
    const char *p = strrchr(haystack, needle);
    return (p ? p - haystack : -1);
}

int specialIndexOf (uint8_t* base, const char* str, int baselen, bool exact) {
    int result;  // seek for str in buffer or in header up to baselen, not nullterninated
    if (strlen(str) > baselen) return -1; // if exact == true seekstr in buffer must have "\0" at the end
    for (int i = 0; i < baselen - strlen(str); i++){
        result = i;
        for (int j = 0; j < strlen(str) + exact; j++){
            if (*(base + i + j) != *(str + j)){
                result = -1;
                break;
            }
        }
        if (result >= 0) break;
    }
    return result;
}
size_t bigEndian(uint8_t* base, uint8_t numBytes, uint8_t shiftLeft) {
    size_t result = 0;
    if(numBytes < 1 or numBytes > 4) return 0;
    for (int i = 0; i < numBytes; i++) {
            result += *(base + i) << (numBytes -i - 1) * shiftLeft;
    }
    return result;
}

char* b64encode(const char* source, uint16_t sourceLength) {
    size_t size = base64_encode_expected_len(sourceLength) + 1;
    char* buffer = (char *) malloc(size);
    if (buffer == NULL) {
        return NULL;
    }

    base64_encodestate _state;
    base64_init_encodestate(&_state);
    int len = base64_encode_block(source, sourceLength, buffer, &_state);
    len = base64_encode_blockend((buffer + len), &_state);
    return buffer;
}

size_t urlencode_expected_len(const char* source) {
    size_t expectedLen = strlen(source);
    for(int i = 0; i < strlen(source); i++) {
        if(isalnum(source[i])){;}
        else expectedLen += 2;
    }
    return expectedLen;
}

//---------------------------------------------------------------------------------------------------------------------
void urlencode(char* buff, uint16_t buffLen, bool spacesOnly) {

    uint16_t len = strlen(buff);
    uint8_t* tmpbuff = (uint8_t*)malloc(buffLen);
    if (!tmpbuff) { log_e("out of memory"); return; }
    char c;
    char code0;
    char code1;
    uint16_t j = 0;
    for (int i = 0; i < len; i++) {
        c = buff[i];
        if (isalnum(c)) tmpbuff[j++] = c;
        else if(spacesOnly) {
            if (c == ' ') {
                tmpbuff[j++] = '%';
                tmpbuff[j++] = '2';
                tmpbuff[j++] = '0';
            }
            else {
                tmpbuff[j++] = c;
            }
        }
        else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) code0 = c - 10 + 'A';
            tmpbuff[j++] = '%';
            tmpbuff[j++] = code0;
            tmpbuff[j++] = code1;
        }
        if (j == buffLen - 1) {
            log_e("out of memory");
            break;
        }
    }
    memcpy(buff, tmpbuff, j);
    buff[j] ='\0';
    free(tmpbuff);
}

//---------------------------------------------------------------------------------------------------------------------
void unicode2utf8(char* buff, uint32_t len) {
    // converts unicode in UTF-8, buff contains the string to be converted up to len
    // range U+1 ... U+FFFF
    uint8_t* tmpbuff = (uint8_t*)malloc(len * 2);
    if(!tmpbuff) {log_e("out of memory"); return;}
    bool bitorder = false;
    uint16_t j = 0;
    uint16_t k = 0;
    uint16_t m = 0;
    uint8_t uni_h = 0;
    uint8_t uni_l = 0;

    while(m < len - 1) {
        if((buff[m] == 0xFE) && (buff[m + 1] == 0xFF)) {
            bitorder = true;
            j = m + 2;
        }  // LSB/MSB
        if((buff[m] == 0xFF) && (buff[m + 1] == 0xFE)) {
            bitorder = false;
            j = m + 2;
        }  // MSB/LSB
        m++;
    } // seek for last bitorder
    m = 0;
    if(j > 0) {
        for(k = j; k < len; k += 2) {
            if(bitorder == true) {
                uni_h = (uint8_t)buff[k];
                uni_l = (uint8_t)buff[k + 1];
            }
            else {
                uni_l = (uint8_t)buff[k];
                uni_h = (uint8_t)buff[k + 1];
            }

            uint16_t uni_hl = ((uni_h << 8) | uni_l);

            if (uni_hl < 0X80){
                tmpbuff[m] = uni_l;
                m++;
            }
            else if (uni_hl < 0X800) {
                tmpbuff[m]= ((uni_hl >> 6) | 0XC0);
                m++;
                tmpbuff[m] =((uni_hl & 0X3F) | 0X80);
                m++;
            }
            else {
                tmpbuff[m] = ((uni_hl >> 12) | 0XE0);
                m++;
                tmpbuff[m] = (((uni_hl >> 6) & 0X3F) | 0X80);
                m++;
                tmpbuff[m] = ((uni_hl & 0X3F) | 0X80);
                m++;
            }
        }
    }
    buff[m] = 0;
    memcpy(buff, tmpbuff, m);
    free(tmpbuff);
}

//---------------------------------------------------------------------------------------------------------------------
bool latinToUTF8(char* buff, size_t bufflen) {
    // most stations send  strings in UTF-8 but a few sends in latin. To standardize this, all latin strings are
    // converted to UTF-8. If UTF-8 is already present, nothing is done and true is returned.
    // A conversion to UTF-8 extends the string. Therefore it is necessary to know the buffer size. If the converted
    // string does not fit into the buffer, false is returned
    // utf8 bytelength: >=0xF0 3 bytes, >=0xE0 2 bytes, >=0xC0 1 byte, e.g. e293ab is ⓫

    uint16_t pos = 0;
    uint8_t  ext_bytes = 0;
    uint16_t len = strlen(buff);
    uint8_t  c;

    while(pos < len){
        c = buff[pos];
        if(c >= 0xC2) {    // is UTF8 char
            pos++;
            if(c >= 0xC0 && buff[pos] < 0x80) {ext_bytes++; pos++;}
            if(c >= 0xE0 && buff[pos] < 0x80) {ext_bytes++; pos++;}
            if(c >= 0xF0 && buff[pos] < 0x80) {ext_bytes++; pos++;}
        }
        else pos++;
    }
    if(!ext_bytes) return true; // is UTF-8, do nothing

    pos = 0;

    while(buff[pos] != 0){
        len = strlen(buff);
        if(buff[pos] >= 0x80 && buff[pos+1] < 0x80){       // is not UTF8, is latin?
            for(int i = len+1; i > pos; i--){
                buff[i+1] = buff[i];
            }
            uint8_t c = buff[pos];
            buff[pos++] = 0xc0 | ((c >> 6) & 0x1f);      // 2+1+5 bits
            buff[pos++] = 0x80 | ((char)c & 0x3f);       // 1+1+6 bits
        }
        pos++;
        if(pos > bufflen -3){
            buff[bufflen -1] = '\0';
            return false; // do not overwrite
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void UTF8toASCII(char* str) {

#ifdef SDFATFS_USED
    //UTF8->UTF16 (lowbyte)
    const uint8_t ascii[60] = {
    //129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148  // UTF8(C3)
    //                Ä    Å    Æ    Ç         É                                       Ñ                  // CHAR
      000, 000, 000, 0xC4, 143, 0xC6,0xC7, 000,0xC9,000, 000, 000, 000, 000, 000, 000, 0xD1, 000, 000, 000, // ASCII (Latin1)
    //149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168
    //      Ö                             Ü              ß    à                   ä    å    æ         è
      000, 0xD6,000, 000, 000, 000, 000, 0xDC, 000, 000, 0xDF,0xE0, 000, 000, 000,0xE4,0xE5,0xE6, 000,0xE8,
    //169, 170, 171, 172. 173. 174. 175, 176, 177, 179, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188
    //      ê    ë    ì         î    ï         ñ    ò         ô         ö              ù         û    ü
      000, 0xEA, 0xEB,0xEC, 000,0xEE,0xEB, 000,0xF1,0xF2, 000,0xF4, 000,0xF6, 000, 000,0xF9, 000,0xFB,0xFC};
#else
    const uint8_t ascii[60] = {
    //129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148  // UTF8(C3)
    //                Ä    Å    Æ    Ç         É                                       Ñ                  // CHAR
      000, 000, 000, 142, 143, 146, 128, 000, 144, 000, 000, 000, 000, 000, 000, 000, 165, 000, 000, 000, // ASCII
    //149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168
    //      Ö                             Ü              ß    à                   ä    å    æ         è
      000, 153, 000, 000, 000, 000, 000, 154, 000, 000, 225, 133, 000, 000, 000, 132, 134, 145, 000, 138,
    //169, 170, 171, 172. 173. 174. 175, 176, 177, 179, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188
    //      ê    ë    ì         î    ï         ñ    ò         ô         ö              ù         û    ü
      000, 136, 137, 141, 000, 140, 139, 000, 164, 149, 000, 147, 000, 148, 000, 000, 151, 000, 150, 129};
#endif

    uint16_t i = 0, j=0, s = 0;
    bool f_C3_seen = false;

    while(str[i] != 0) {                                     // convert UTF8 to ASCII
        if(str[i] == 195){                                   // C3
            i++;
            f_C3_seen = true;
            continue;
        }
        str[j] = str[i];
        if(str[j] > 128 && str[j] < 189 && f_C3_seen == true) {
            s = ascii[str[j] - 129];
            if(s != 0) str[j] = s;                         // found a related ASCII sign
            f_C3_seen = false;
        }
        i++; j++;
    }
    str[j] = 0;
}






}  //end namespace