#include "AudioPlaylist.h"
#include "Utils.h"

#include <Arduino.h>

//---------------------------------------------------------------------------------------------------------------------
void AudioPlaylist::processPlayListData() {

#if 0 //WORK in progress...
    static bool f_entry     = false;                            // entryflag for asx playlist
    static bool f_title     = false;                            // titleflag for asx playlist
    static bool f_ref       = false;                            // refflag   for asx playlist
    static bool f_begin     = false;
    static bool f_end       = false;
    static bool f_ct        = false;

    (void)f_title;  // is unused yet

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_datamode == AUDIO_PLAYLISTINIT) {                  // Initialize for receive .m3u file
        // We are going to use metadata to read the lines from the .m3u file
        // Sometimes this will only contain a single line
        f_entry     = false;
        f_title     = false;
        f_ref       = false;
        f_begin     = false;
        f_end       = false;
        f_ct        = false;

        m_datamode = AUDIO_PLAYLISTHEADER;                  // Handle playlist data
        //if(audio_info) audio_info("Read from playlist");
    } // end AUDIO_PLAYLISTINIT

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int av = 0;
    av = _client->available();
    if(av < 1){
        if(f_end) return;
        if(f_begin) {f_end = true;}
        else return;
    }

    char pl[256]; // playlistline
    uint8_t b = 0;
    int16_t pos = 0;

    while(true){
        b = _client->read();
        if(b == 0xff) b = '\n'; // no more to read? send new line
        if(b == '\n') {pl[pos] = 0; break;}
        if(b < 0x20 || b > 0x7E) continue;
        pl[pos] = b;
        if(pos < 255) pos++;
        if(pos == 254){pl[pos] = '\0'; /*log_e("playlistline overflow");*/}
    }

    if(strlen(pl) == 0 && m_datamode == AUDIO_PLAYLISTHEADER) {
        if(m_f_Log) if(audio_info) audio_info("Switch to PLAYLISTDATA");
        m_datamode = AUDIO_PLAYLISTDATA;                    // Expecting data now
        return;
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_datamode == AUDIO_PLAYLISTHEADER) {                    // Read header

        if(m_f_Log) {AUDIO_INFO(sprintf(chbuf, "Playlistheader: %s", pl);)} // Show playlistheader

        if(utils::indexOf(pl, "Content-Type:", 0)){
            f_ct = true;                                        // found ContentType in pl
        }

        if(utils::indexOf(pl, "content-type:", 0)){
            f_ct = true;                                        // found ContentType in pl
        }

        if((utils::indexOf(pl, "Connection:close", 0) >= 0) && !f_ct){ // #193 is not a playlist if no ct found
            m_datamode = AUDIO_HEADER;
        }

        int pos = utils::indexOf(pl, "400 Bad Request", 0);
        if(pos >= 0) {
            m_datamode = AUDIO_NONE;
            if(audio_info) audio_info("Error 400 Bad Request");
            stopSong();
            return;
        }

        pos = utils::indexOf(pl, "404 Not Found", 0);
        if(pos >= 0) {
            m_datamode = AUDIO_NONE;
            if(audio_info) audio_info("Error 404 Not Found");
            stopSong();
            return;
        }

        pos = utils::indexOf(pl, "404 File Not Found", 0);
        if(pos >= 0) {
            m_datamode = AUDIO_NONE;
            if(audio_info) audio_info("Error 404 File Not Found");
            stopSong();
            return;
        }

        pos = utils::indexOf(pl, "HTTP/1.0 404", 0);
        if(pos >= 0) {
            m_datamode = AUDIO_NONE;
            if(audio_info) audio_info("Error 404 Not Available");
            stopSong();
            return;
        }

        pos = utils::indexOf(pl, "HTTP/1.1 401", 0);
        if(pos >= 0) {
            m_datamode = AUDIO_NONE;
            if(audio_info) audio_info("Error 401 Unauthorized");
            stopSong();
            return;
        }

        pos = utils::indexOf(pl, "HTTP/1.1 403", 0);
        if(pos >= 0) {
            m_datamode = AUDIO_NONE;
            if(audio_info) audio_info("Error 403 Forbidden");
            stopSong();
            return;
        }

        pos = utils::indexOf(pl, ":", 0);                          // lowercase all letters up to the colon
        if(pos >= 0) {
            for(int i=0; i < pos; i++) {
                pl[i] = toLowerCase(pl[i]);
            }
        }

        if(utils::startsWith(pl, "icy-")){                         // icy-data in playlist? that can not be
            m_datamode = AUDIO_HEADER;
            if(audio_info) audio_info("playlist is not valid, switch to AUDIO_HEADER");
            return;
        }


        if(utils::startsWith(pl, "location:") || utils::startsWith(pl, "Location:")) {
            char* host;
            pos = utils::indexOf(pl, "http", 0);
            host = (pl + pos);
//            sprintf(chbuf, "redirect to new host %s", host);
//            if(m_f_Log) if(audio_info) audio_info(chbuf);
            pos = utils::indexOf(pl, "/", 10);
            if(strncmp(host, m_lastHost, pos) == 0){                                    // same host?
                _client->stop(); _client->flush();
                httpPrint(host);
            }
            else connecttohost(host);                                                   // different host,
        }

    } // end AUDIO_PLAYLISTHEADER

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if(m_datamode == AUDIO_PLAYLISTDATA) {                  // Read next byte of .m3u file data
        if(m_f_Log) { AUDIO_INFO(sprintf(chbuf, "Playlistdata: %s", pl);) }   // Show playlistdata

        pos = utils::indexOf(pl, "<!DOCTYPE", 0);                  // webpage found
        if(pos >= 0) {
            m_datamode = AUDIO_NONE;
            if(audio_info) audio_info("Not Found");
            stopSong();
            return;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_playlistFormat == FORMAT_M3U) {

            if(!f_begin) f_begin = true;                    // first playlistdata received
            if(utils::indexOf(pl, "#EXTINF:", 0) >= 0) {           // Info?
               pos = utils::indexOf(pl, ",", 0);                   // Comma in this line?
               if(pos > 0) {
                   // Show artist and title if present in metadata
                   if(audio_info) audio_info(pl + pos + 1);
               }
               return;
           }
           if(utils::startsWith(pl, "#")) {                        // Commentline?
               return;
           }

           pos = utils::indexOf(pl, "http://:@", 0); // ":@"??  remove that!
           if(pos >= 0) {
               AUDIO_INFO(sprintf(chbuf, "Entry in playlist found: %s", (pl + pos + 9));)
               connecttohost(pl + pos + 9);
               return;
           }
           //sprintf(chbuf, "Entry in playlist found: %s", pl);
           //if(audio_info) audio_info(chbuf);
           pos = utils::indexOf(pl, "http", 0);                    // Search for "http"
           const char* host;
           if(pos >= 0) {                                   // Does URL contain "http://"?
               host = (pl + pos);
               connecttohost(host);
           }                                                // Yes, set new host
           return;
        } //m3u

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_playlistFormat == FORMAT_PLS) {

            if(!f_begin){
                if(strlen(pl) == 0) return;                 // empty line
                if(strcmp(pl, "[playlist]") == 0){          // first entry in valid pls
                    f_begin = true;                         // we have first playlistdata received
                    return;
                }
                else{
                    m_datamode = AUDIO_HEADER;                // pls is not valid
                    if(audio_info) audio_info("pls is not valid, switch to AUDIO_HEADER");
                    return;
                }
            }

            if(utils::startsWith(pl, "File1")) {
                pos = utils::indexOf(pl, "http", 0);                   // File1=http://streamplus30.leonex.de:14840/;
                if(pos >= 0) {                                  // yes, URL contains "http"?
                    memcpy(m_lastHost, pl + pos, strlen(pl) + 1);   // http://streamplus30.leonex.de:14840/;
                    // Now we have an URL for a stream in host.
                    f_ref = true;
                }
            }
            if(utils::startsWith(pl, "Title1")) {                      // Title1=Antenne Tirol
                const char* plsStationName = (pl + 7);
                if(audio_showstation) audio_showstation(plsStationName);
                AUDIO_INFO(sprintf(chbuf, "StationName: \"%s\"", plsStationName);)
                f_title = true;
            }
            if(utils::startsWith(pl, "Length1")) f_title = true;               // if no Title is available
            if((f_ref == true) && (strlen(pl) == 0)) f_title = true;

            if(utils::indexOf(pl, "Invalid username", 0) >= 0){ // Unable to access account: Invalid username or password
                m_f_running = false;
                stopSong();
                m_datamode = AUDIO_NONE;
                return;
            }

            if(f_end) {                                      // we have both StationName and StationURL
                log_d("connect to new host %s", m_lastHost);
                connecttohost(m_lastHost);                              // Connect to it
            }
            return;
        } // pls

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_playlistFormat == FORMAT_ASX) { // Advanced Stream Redirector
            if(!f_begin) f_begin = true;                        // first playlistdata received
            int p1 = utils::indexOf(pl, "<", 0);
            int p2 = utils::indexOf(pl, ">", 1);
            if(p1 >= 0 && p2 > p1){                                 // #196 set all between "< ...> to lowercase
                for(uint8_t i = p1; i < p2; i++){
                    pl[i] = toLowerCase(pl[i]);
                }
            }

            if(utils::indexOf(pl, "<entry>", 0) >= 0) f_entry = true;      // found entry tag (returns -1 if not found)

            if(f_entry) {
                if(utils::indexOf(pl, "ref href", 0) > 0) {                // <ref href="http://87.98.217.63:24112/stream" />
                    pos = utils::indexOf(pl, "http", 0);
                    if(pos > 0) {
                        char* plsURL = (pl + pos);                  // http://87.98.217.63:24112/stream" />
                        int pos1 = utils::indexOf(plsURL, "\"", 0);        // http://87.98.217.63:24112/stream
                        if(pos1 > 0) {
                            plsURL[pos1] = '\0';
                        }
                        memcpy(m_lastHost, plsURL, strlen(plsURL) + 1); // save url in array
                        log_d("m_lastHost = %s",m_lastHost);
                        // Now we have an URL for a stream in host.
                        f_ref = true;
                    }
                }
                pos = utils::indexOf(pl, "<title>", 0);
                if(pos < 0) pos = utils::indexOf(pl, "<Title>", 0);
                if(pos >= 0) {
                    char* plsStationName = (pl + pos + 7);          // remove <Title>
                    pos = utils::indexOf(plsStationName, "</", 0);
                    if(pos >= 0){
                            *(plsStationName +pos) = 0;             // remove </Title>
                    }
                    if(audio_showstation) audio_showstation(plsStationName);
                    AUDIO_INFO(sprintf(chbuf, "StationName: \"%s\"", plsStationName);)
                    f_title = true;
                }
            } //entry
            if(utils::indexOf(pl, "http", 0) == 0 && !f_entry) { //url only in asx
                memcpy(m_lastHost, pl, strlen(pl)); // save url in array
                m_lastHost[strlen(pl)] = '\0';
                log_d("m_lastHost = %s",m_lastHost);
                connecttohost(pl);
            }
            if(f_end) {   //we have both StationName and StationURL
                connecttohost(m_lastHost);                          // Connect to it
            }
            return;
        }  //asx
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_playlistFormat == FORMAT_M3U8) {

            static bool f_StreamInf = false;                            // set if  #EXT-X-STREAM-INF in m3u8
            static bool f_ExtInf    = false;                            // set if  #EXTINF in m3u8
            static uint8_t plsEntry = 0;                                // used in m3u8, counts url entries
            static uint8_t seqNrPos = 0;                                // position at which the SeqNr is found

            if(!f_begin){
                if(strlen(pl) == 0) return;                             // empty line
                if(strcmp(pl, "#EXTM3U") == 0){                         // what we expected
                    f_begin      = true;
                    f_StreamInf  = false;
                    f_ExtInf     = false;
                    plsEntry     = 0;
                    return;
                }
                else{
                    m_datamode = AUDIO_HEADER;                          // m3u8 is not valid
                    m_playlistFormat = FORMAT_NONE;
                    if(audio_info) audio_info("m3u8 is not valid, switch to AUDIO_HEADER");
                    return;
                }
            }

            // example: redirection
            // #EXTM3U
            // #EXT-X-STREAM-INF:BANDWIDTH=22050,CODECS="mp4a.40.2"
            // http://ample.revma.ihrhls.com/zc7729/63_sdtszizjcjbz02/playlist.m3u8

            // example: audio chunks
            // #EXTM3U
            // #EXT-X-TARGETDURATION:10
            // #EXT-X-MEDIA-SEQUENCE:163374040
            // #EXT-X-DISCONTINUITY
            // #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
            // http://n3fa-e2.revma.ihrhls.com/zc7729/63_sdtszizjcjbz02/main/163374038.aac
            // #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
            // http://n3fa-e2.revma.ihrhls.com/zc7729/63_sdtszizjcjbz02/main/163374039.aac

            if(utils::startsWith(pl,"#EXT-X-STREAM-INF:")){
                int pos = utils::indexOf(pl, "CODECS=\"mp4a", 18);
                if(pos < 0){ // not found
                    m_m3u8codec  = CODEC_NONE;
                    log_e("codec %s in m3u8 playlist not supportet", pl);
                    stopSong();
                    return;
                }
                f_StreamInf = true;
                m_m3u8codec = CODEC_M4A;
                return;
            }

            if(f_StreamInf){                                                // it's a redirection, a new m3u8 playlist
                if(utils::startsWith(pl, "http")) {
                    strcpy(m_lastHost, pl);
                }
                else{
                    pos = utils::lastIndexOf(m_lastHost, "/");
                    strcpy(m_lastHost + pos + 1, pl);
                }
                f_StreamInf = false;
                connecttohost(m_lastHost);
                return;
            }

            if(m_m3u8codec == CODEC_NONE){                                                  // second guard
                if(!f_end) return;
                else {connecttohost(m_lastHost); return;}
            }

            static uint32_t seqNr = 0;
            if(utils::startsWith(pl, "#EXT-X-MEDIA-SEQUENCE:")){
                // do nothing, because MEDIA-SECUENCE is not set sometimes
            }

            static uint16_t targetDuration = 0;
            if(utils::startsWith(pl, "#EXT-X-TARGETDURATION:")) {targetDuration = atoi(pl + 22);}

            if(utils::startsWith(pl,"#EXTINF")) {
                f_ExtInf = true;
                if(STfromEXTINF(pl)) showstreamtitle(pl);
                return;
            }

            if(f_ExtInf){
                f_ExtInf = false;
//                log_i("ExtInf=%s", pl);
                int16_t lastSlash = utils::lastIndexOf(m_lastHost, "/");

                if(!m_playlistBuff){ // will  be freed in setDefaults()
                    m_playlistBuff = (char*)malloc(2 * m_plsBuffEntryLen);
                    strcpy(m_playlistBuff, m_lastHost); // save the m3u8 url at pos 0
                }

                if(plsEntry == 0){
                    seqNrPos = 0;
                    char* entryPos = m_playlistBuff + m_plsBuffEntryLen;
                    if(utils::startsWith(pl, "http")){
                        strcpy(entryPos , pl);
                    }
                    else{
                        strcpy(entryPos , m_lastHost); // if not start with http complete with host from m_lasthost
                        strcpy(entryPos + lastSlash + 1 , pl);
                    }
                    // now the url is completed, we have a look at the sequenceNumber
                    if(m_m3u8codec == CODEC_M4A){
                        int p1 = utils::lastIndexOf(entryPos, "/");
                        int p2 = utils::indexOf(entryPos, ".aac", 0);
                        if(p1<0 || p2<0){
                            log_e("sequenceNumber not found");
                            stopSong();
                            return;
                        }
                        // seqNr must be between p1 and p2
                        for(int i = p1; i < p2; i++){
                            if(entryPos[i] >= 48 && entryPos[i] <=57){ // numbers only
                                if(!seqNrPos) seqNrPos = i;
                            }
                            else{
                                seqNrPos = 0; // in case ...52397ae8f_1.aac?sid=5193 seqNr=1
                            }
                        }
                        seqNr = atoi(&entryPos[seqNrPos]);
                        //log_i("entryPos=%s", entryPos);
                        //log_i("p1=%i, p2=%i, seqNrPos =%i, seqNr=%d", p1, p2, seqNrPos, seqNr);
                    }
                }
                plsEntry++;
                return;
            }

            if(f_end){
                if(plsEntry > 0){ // we have found some (url) entries
                    processM3U8entries(plsEntry, seqNr, seqNrPos, targetDuration);
                }
                else{
                    connecttohost(m_lastHost);
                }
            }
        } //end m3u8
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        return;
    } // end AUDIO_PLAYLISTDATA

#endif

}

//---------------------------------------------------------------------------------------------------------------------
bool AudioPlaylist::STfromEXTINF(char* str) {
    // extraxt StreamTitle from m3u #EXTINF line to icy-format
    // orig: #EXTINF:10,title="text="TitleName",artist="ArtistName"
    // conv: StreamTitle=TitleName - ArtistName
    // orig: #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
    // conv: StreamTitle=text=\"Spot Block End\" amgTrackId=\"9876543\" -

    if (!utils::startsWith(str,"#EXTINF")) return false;
    int t1, t2, t3, n0 = 0, n1 = 0, n2 = 0;

    t1 = utils::indexOf(str, "title", 0);
    if(t1 > 0){
        strcpy(chbuf, "StreamTitle="); n0 = 12;
        t2 = t1 + 7; // title="
        t3 = utils::indexOf(str, "\"", t2);
        while(str[t3 - 1] == '\\'){
            t3 = utils::indexOf(str, "\"", t3 + 1);
        }
        if(t2 < 0 || t2 > t3) return false;
        n1 = t3 - t2;
        strncpy(chbuf + n0, str + t2, n1);
    }

    t1 = utils::indexOf(str, "artist", 0);
    if(t1 > 0){
        strcpy(chbuf + n0 + n1, " - ");   n1 += 3;
        t2 = utils::indexOf(str, "=\"", t1); t2 += 2;
        t3 = utils::indexOf(str, "\"", t2);
        if(t2 < 0 || t2 > t3) return false;
        n2 = t3 - t2;
        strncpy(chbuf + n0 + n1, str + t2, n2);
        chbuf[n0 + n1 + n2] = '\0';
    }
    strcpy(str, chbuf);

    return true;
}

