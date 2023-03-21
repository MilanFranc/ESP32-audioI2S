#include "UrlParser.h"

UrlParser::UrlParser()
{
}

bool UrlParser::parse(const char* url)
{
    enum URLParseState { PROTOCOL, SEPARATOR, HOST, PORT, PATH, ERROR };
    URLParseState state = PROTOCOL;

    if (url[0] == '\0') {
        return false;
    }

    m_protocol.clear();
    m_host.clear();
    m_port.clear();
    m_path = "/";

    for (int i = 0; url[i] != '\0'; i++)
    {
        char chr = url[i];
        switch(state)
        {
        case PROTOCOL:
            if (isAlpha(chr)) {
                m_protocol += chr;
            }
            else if (chr == ':')
                state = SEPARATOR;
            else {
                state = ERROR;
            }
            break;
        
        case SEPARATOR:
            if (chr != '/') {
                state = HOST;
                m_host += chr;
            }
            break;

        case HOST:
            if (isAlphaNumeric(chr) || chr == '.' || chr == '-') {
                m_host += chr;
            }
            else if (chr == ':') { state = PORT; }
            else if (chr == '/') { state = PATH; }
            else { state = ERROR; }
            break;
        
        case PORT:
            if (isdigit(chr)) {
                m_port += chr;
            }
            else if (chr == '/') { state = PATH; }
            break;

        case PATH:
            m_path += chr;
            break;
        }
    }
    return state != ERROR;
}
