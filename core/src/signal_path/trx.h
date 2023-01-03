#pragma once
#include <map>
#include <string>
#include <dsp/stream.h>
#include <dsp/types.h>
#include "../dsp/routing/splitter.h"
#include "../dsp/audio/volume.h"
#include "../dsp/sink/null_sink.h"
#include <mutex>
#include <utils/event.h>
#include <vector>


class Transmitter {


    // for stream transmission
    virtual int getInputStreamFramerate() = 0;      // 48000 hz for hermes lite 2 input stream
    virtual void setTransmitStatus(bool status) = 0;
    virtual void setTransmitStream(dsp::stream<dsp::complex_t> *stream) = 0;
    virtual void setTransmitGain() = 0;

    // for tone transmission
    virtual void startGenerateTone(int frequency) = 0;
    virtual void stopGenerateTone() = 0;
    virtual void setToneGain() = 0;

    virtual int getTXStatus() = 0;      // tone or stream

    virtual float getTransmitPower() = 0;
    virtual float getTransmitSWR() = 0;
    virtual std::string &getTransmitterName() = 0;

};