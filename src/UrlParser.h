#pragma once

#include <Arduino.h>

class UrlParser
{
public:
    UrlParser();
    bool parse(const char* url);

    String protocol() const { return m_protocol; }
    String host() const { return m_host; }
    String port() const { return m_port; }
    String path() const { return m_path; }

private:
    String m_protocol;
    String m_host;
    String m_port;
    String m_path;
};


