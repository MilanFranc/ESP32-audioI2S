#include "MetaDataDecoder.h"
#include "Utils.h"

MetaDataDecoder::MetaDataDecoder()
{
    reset();
}

void MetaDataDecoder::reset()
{
    m_index = 0;
    m_datalen = 0;
    Serial.println("Meta: reset.");
}

bool MetaDataDecoder::addData(uint8_t b)
{
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if (m_datalen == 0) {                                  // First byte of metadata?
        if (b == 0) {
            return true;
        }

        Serial.println("Meta: byte:" + String(b, HEX));

        m_datalen = b * 16 + 1;                            // New count for metadata including length byte
        //Serial.print("Meta: length=");
        //Serial.println(String(m_metalen));

        if (m_datalen > 512) {
            //  if(audio_info) audio_info("Metadata block to long! Skipping all Metadata from now on.");
            m_f_swm = true;                              // expect stream without metadata
        }
        m_index = 0; m_buffer[m_index] = 0;                   // Prepare for new line
    }
    else {
        Serial.println("Meta: byte:" + String(b, HEX));

        m_buffer[m_index] = (char) b;                        // Put new char in +++++
        m_index++;

//        m_buffer[m_index] = 0;
//        if (m_index == 509) { log_e("metaline overflow in AUDIO_METADATA! metaline=%s", m_buffer); }
//        if (m_index == 510) { ; /* last current char in b */}

    }
    
    m_datalen--;
    if (m_datalen == 0) {
        m_buffer[m_index] = '\0';
        if (strlen(m_buffer) > 0) {                     // Any info present?
            // metaline contains artist and song name.  For example:
            // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
            // Sometimes it is just other info like:
            // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
            // Isolate the StreamTitle, remove leading and trailing quotes if present.
            // log_i("ST %s", metaline);

            utils::latinToUTF8(m_buffer, sizeof(m_buffer)); // convert to UTF-8 if necessary

            int pos = utils::indexOf(m_buffer, "song_spot", 0);    // remove some irrelevant infos
            if (pos > 3) {                                // e.g. song_spot="T" MediaBaseId="0" itunesTrackId="0"
                m_buffer[pos] = 0;
            }

            Serial.print("Meta:");
            Serial.println(m_buffer);
            Serial.print("Heap:");
            Serial.println(String(ESP.getFreeHeap()));

//            if (!m_f_localfile) 
//                showstreamtitle(chbuf);   // Show artist and title if present in metadata
        }
        return true;
    }

    return false;// end_METADATA
}
