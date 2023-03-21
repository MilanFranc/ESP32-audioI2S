#pragma once
#include <Arduino.h>

namespace utils {

void trim(char *s);
bool startsWith (const char* base, const char* str);
bool endsWith (const char* base, const char* str);
int indexOf (const char* base, const char* str, int startIndex);
int indexOf (const char* base, char ch, int startIndex);
int lastIndexOf(const char* haystack, const char* needle);
int lastIndexOf(const char* haystack, const char needle);

int specialIndexOf (uint8_t* base, const char* str, int baselen, bool exact = false);


size_t bigEndian(uint8_t* base, uint8_t numBytes, uint8_t shiftLeft = 8);
char* b64encode(const char* source, uint16_t sourceLength);
size_t urlencode_expected_len(const char* source);
void urlencode(char* buff, uint16_t buffLen, bool spacesOnly = false);

void unicode2utf8(char* buff, uint32_t len);
bool latinToUTF8(char* buff, size_t bufflen);
void UTF8toASCII(char* str);

}




