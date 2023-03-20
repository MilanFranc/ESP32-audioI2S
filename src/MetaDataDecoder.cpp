#include "MetaDataDecoder.h"
#include "Utils.h"

MetaDataDecoder::MetaDataDecoder()
{
    reset();
}

void MetaDataDecoder::reset()
{
    m_pos_ml = 0;
    m_metalen = 0;
}

bool MetaDataDecoder::addData(uint8_t b)
{
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if (!m_metalen) {                                       // First byte of metadata?
        m_metalen = b * 16 + 1;                            // New count for metadata including length byte
        Serial.print("Meta: length=");
        Serial.println(String(m_metalen));

        if (m_metalen > 512) {
            //  if(audio_info) audio_info("Metadata block to long! Skipping all Metadata from now on.");
            m_f_swm = true;                              // expect stream without metadata
        }
        m_pos_ml = 0; m_buffer[m_pos_ml] = 0;                   // Prepare for new line
    }
    else {
        m_buffer[m_pos_ml] = (char) b;                        // Put new char in +++++
        if (m_pos_ml < 510) m_pos_ml++;
        m_buffer[m_pos_ml] = 0;
        if (m_pos_ml == 509) { log_e("metaline overflow in AUDIO_METADATA! metaline=%s", m_buffer); }
        if (m_pos_ml == 510) { ; /* last current char in b */}

    }
    m_metalen--;
    if (m_metalen == 0) {
        if (strlen(m_buffer) > 0) {                        // Any info present?
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

//            if (!m_f_localfile) 
//                showstreamtitle(chbuf);   // Show artist and title if present in metadata
        }
        return true;
    }

    return false;// end_METADATA
}
