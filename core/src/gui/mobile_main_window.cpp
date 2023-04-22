#define IMGUI_DEFINE_MATH_OPERATORS
#define _USE_MATH_DEFINES // NOLINT(bugprone-reserved-identifier)
#include <gui/mobile_main_window.h>
#include <gui/gui.h>
#include "imgui_internal.h"
#include <volk/volk_malloc.h>
#include "imgui.h"
#include <cstdio>
#include <thread>
#include <algorithm>
#include <gui/widgets/waterfall.h>
#include <gui/icons.h>
#include <gui/widgets/bandplan.h>
#include <gui/style.h>
#include <core.h>
#include <gui/menus/display.h>
#include <gui/menus/theme.h>
#include <filesystem>
#include <gui/tuner.h>
#include "../../decoder_modules/radio/src/radio_module.h"
#include "../../misc_modules/noise_reduction_logmmse/src/arrays.h"
#include "../../misc_modules/noise_reduction_logmmse/src/af_nr.h"
#include "dsp/convert/stereo_to_mono.h"
#include "dsp/convert/real_to_complex.h"
#include "dsp/convert/complex_to_real.h"
#include "dsp/channel/frequency_xlator.h"
#include "ctm.h"
#include <cmath>
#include <utility>

using namespace ::dsp::arrays;

static bool doFingerButton(const std::string &title) {
    const ImVec2& labelWidth = ImGui::CalcTextSize(title.c_str(), nullptr, true, -1);
    return ImGui::Button(title.c_str(), ImVec2(labelWidth.x + style::baseFont->FontSize, style::baseFont->FontSize * 3));
};

struct Decibelometer {
    float maxSignalPeak = -1000.0;
    long long maxSignalPeakTime = 0.0;
    std::vector<float> lastMeasures;
    void addSamples(dsp::stereo_t *samples, int count) {
        float summ = 0;
        for (int i = 0; i < count; i++) {
            summ += samples[i].l * samples[i].l;
        }
        summ = sqrt(summ / count);
        lastMeasures.emplace_back(20 * log10(summ));
        if (lastMeasures.size() > 100) {
            lastMeasures.erase(lastMeasures.begin());
        }
        updateMax();
    }

    float getAvg(int len) {
        if (len > lastMeasures.size()) {
            len = lastMeasures.size();
        }
        float sum = 0;
        for (int i = 0; i < len; i++) {
            sum += lastMeasures[lastMeasures.size()-1-i];
        }
        return sum / len;
    }

    float getMax(int len) {
        if (len > lastMeasures.size()) {
            len = lastMeasures.size();
        }
        float maxx = -1000;
        for (int i = 0; i < len; i++) {
            maxx = std::max(maxx, lastMeasures[lastMeasures.size()-1-i]);
        }
        return maxx;
    }

    void addSamples(float *samples, int count) {
        float summ = 0;
        for (int i = 0; i < count; i++) {
            summ += samples[i] * samples[i];
        }
        summ = sqrt(summ / count);
        lastMeasures.emplace_back(20 * log10(summ));
        if (lastMeasures.size() > 100) {
            lastMeasures.erase(lastMeasures.begin());
        }
        updateMax();
    }

    float getPeak() {
        return maxSignalPeak;
    }

    void updateMax() {
        auto mx = *lastMeasures.end();
        if (mx > maxSignalPeak || maxSignalPeakTime < currentTimeMillis() - 500) {
            maxSignalPeak = mx;
            maxSignalPeakTime = currentTimeMillis();;
        }
    }
};


struct SubWaterfall {
    ImGui::WaterFall waterfall;
    fftwf_complex* fft_in;
    fftwf_complex* fft_out;
    fftwf_plan fftwPlan;
    float *spectrumLine;
    float hiFreq = 3000;
    int fftSize;
    float waterfallRate = 10;

    SubWaterfall() {
        waterfall.WATERFALL_NUMBER_OF_SECTIONS = 5;
        fftSize = hiFreq / waterfallRate;
        waterfall.setRawFFTSize(fftSize);
        waterfall.setBandwidth(2 * hiFreq);
        waterfall.setViewBandwidth(hiFreq);
        waterfall.setViewOffset(hiFreq);
        waterfall.setFFTMin(-150);
        waterfall.setFFTMax(0);
        waterfall.setWaterfallMin(-150);
        waterfall.setWaterfallMax(0);
        waterfall.setFullWaterfallUpdate(false);

        fft_in = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize);
        fft_out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize);
        fftwPlan = fftwf_plan_dft_1d(fftSize, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
        spectrumLine = (float *)volk_malloc(fftSize * sizeof(float), 16);
    }

    ~SubWaterfall() {
        fftwf_destroy_plan(fftwPlan);
        fftwf_free(fft_in);
        fftwf_free(fft_out);
        volk_free(spectrumLine);
    }

    dsp::multirate::RationalResampler<dsp::stereo_t> res;

    void init() {
        waterfall.init();
        res.init(nullptr, 48000, 2 * hiFreq);

    }

    void draw(ImVec2 loc, ImVec2 wfSize) {
        ImGui::SetCursorPos(loc);
        ImGui::BeginChild("audio_waterfall", wfSize, false,
                          ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDecoration
                              | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
        waterfall.draw();
        ImGui::EndChild();
        flushDrawUpdates();
    }

    std::vector<dsp::stereo_t> inputBuffer;
    std::mutex inputBufferMutex;
    std::vector<dsp::stereo_t> resampledV;
    std::vector<std::pair<float, float>> minMaxQueue;

    void addAudioSamples(dsp::stereo_t * samples, int count, int sampleRate) {
        int newSize = 1000 + (res.getOutSampleRate() * count / sampleRate);
        if (resampledV.size() < newSize) {
            resampledV.resize(newSize);
        }
        if (res.getInSampleRate() != sampleRate) {
            res.setInSamplerate(sampleRate);
        }
        count = res.process(count, samples, resampledV.data());
        samples = resampledV.data();

        std::lock_guard<std::mutex> lock(inputBufferMutex);
        int curr = inputBuffer.size();
        inputBuffer.resize(curr + count);
        memcpy(inputBuffer.data() + curr, samples, count * sizeof(dsp::stereo_t));
    }

    void flushDrawUpdates() {
        std::lock_guard<std::mutex> lock(inputBufferMutex);
        while (inputBuffer.size() > fftSize) {
            for (int i = 0; i < fftSize; i++) {
                fft_in[i][0] = inputBuffer[i].l;
                fft_in[i][1] = 0;
            }
            fftwf_execute(fftwPlan);
            volk_32fc_s32f_power_spectrum_32f(spectrumLine, (const lv_32fc_t*)fft_out, fftSize, fftSize);
            float* dest = waterfall.getFFTBuffer();
            memcpy(dest, spectrumLine, fftSize * sizeof(float));
            for(int q=0; q<fftSize/2; q++) {
                std::swap(dest[q], dest[q + fftSize / 2]);
            }
            // bins are located
            // -hiFreq .. 0 ... hiFreq  // total fftSize
            int startFreq = 50;
            int stopFreq = 2300;
            int startBin = fftSize / 2 + (startFreq / hiFreq * fftSize / 2);
            int endBin = fftSize / 2 + (stopFreq / hiFreq * fftSize / 2);
            float minn = dest[startBin];
            float maxx = dest[startBin];
            for(int q=startBin; q<endBin; q++) {
                if (dest[q] < minn) {
                    minn = dest[q];
                }
                if (dest[q] > maxx) {
                    maxx = dest[q];
                }
            }
            minMaxQueue.emplace_back(minn, maxx);

            int AVERAGE_SECONDS = 5;
            waterfall.pushFFT();
            inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + fftSize);
            int start = (int)minMaxQueue.size() - waterfallRate * AVERAGE_SECONDS;
            if (start < 0) {
                start = 0;
            }
            float bmin = 0;
            float bmax = 0;
            for(int z = start; z < minMaxQueue.size(); z++) {
                bmin += minMaxQueue[z].first;
                bmax += minMaxQueue[z].second;
            }
            bmin /= (minMaxQueue.size() - start);
            bmax /= (minMaxQueue.size() - start);
            waterfall.setWaterfallMin(bmin);
            waterfall.setWaterfallMax(bmax+30);
            if (bmin < waterfall.getFFTMin()) {
                waterfall.setFFTMin(bmin);
            } else {
                waterfall.setFFTMin(waterfall.getFFTMin()+1);
            }
            if (bmax > waterfall.getFFTMax()) {
                waterfall.setFFTMax(bmax);
            } else {
                waterfall.setFFTMax(waterfall.getFFTMax()-1);
            }
            while(minMaxQueue.size() > waterfallRate * AVERAGE_SECONDS) {
                minMaxQueue.erase(minMaxQueue.begin());
            }
        }
    }


};

static bool withinRect(ImVec2 size, ImVec2 point) {
    if (isnan(point.x)) {
        return false;
    }
    if (point.x < 0 || point.y < 0) return false;
    if (point.x > size.x || point.y > size.y) return false;
    return true;
}


struct AudioInToTransmitter : dsp::Processor<dsp::stereo_t, dsp::complex_t> {

    std::string submode;
    dsp::convert::StereoToMono s2m;
    dsp::convert::RealToComplex r2c;
    dsp::channel::FrequencyXlator xlator1;
    dsp::channel::FrequencyXlator xlator2;
    dsp::tap<dsp::complex_t> ftaps;
    dsp::filter::FIR<dsp::complex_t, dsp::complex_t> filter;
    int micGain = 0;
    int sampleCount = 0;
    int tuneFrequency = 0;

    AudioInToTransmitter(dsp::stream<dsp::stereo_t>* in) {
        init(in);
        totalRead = 0;
    }

    int totalRead;

    void setSubmode(const std::string& _submode) {
        if (this->submode != _submode) {
            this->submode = _submode;
        }
    }

    void setMicGain(int _micGain) {
        this->micGain = _micGain;
    }

    void start() override {
        flog::info("AudioInToTransmitter: Start");
        auto bandwidth = 2700.0;
        if (this->submode == "LSB") {
            xlator1.init(nullptr, -bandwidth / 2, 48000);
            xlator2.init(nullptr, bandwidth / 2, 48000);
        }
        else {
            xlator1.init(nullptr, bandwidth / 2, 48000);
            xlator2.init(nullptr, -bandwidth / 2, 48000);
        }
        ftaps = dsp::taps::lowPass0<dsp::complex_t>(bandwidth / 2, bandwidth / 2 * 0.1, 48000);
        filter.init(nullptr, ftaps);
        dsp::Processor<dsp::stereo_t, dsp::complex_t>::start();
    }

    void stop() override {
        dsp::Processor<dsp::stereo_t, dsp::complex_t>::stop();
        dsp::taps::free(ftaps);
    }

    int run() override {
        int rd = this->_in->read();
        if (rd < 0) {
            return rd;
        }
        if (tuneFrequency != 0) {
            // generate sine wave for soft tune
            int period = 48000 / tuneFrequency;
            auto thisStart = sampleCount % period;
            for (auto q = 0; q < rd; q++) {
                auto angle = (thisStart + q) / (double)period * 2 * M_PI;
                out.writeBuf[q].re = (float)sin(angle);
                out.writeBuf[q].im = (float)cos(angle);
            }
        }
        else {
            dsp::convert::StereoToMono::process(rd, this->_in->readBuf, s2m.out.writeBuf);
            int gain = 4;
            if (this->postprocess) {
                dsp::convert::RealToComplex::process(rd, s2m.out.writeBuf, r2c.out.writeBuf);
                xlator1.process(rd, r2c.out.writeBuf, xlator1.out.writeBuf);
                filter.process(rd, xlator1.out.writeBuf, filter.out.writeBuf);
                xlator2.process(rd, filter.out.writeBuf, out.writeBuf);
                for(int q=0; q<rd; q++) {
                    out.writeBuf[q].re *= gain;
                    out.writeBuf[q].im *= gain;
                }
            }
            else {
                dsp::convert::RealToComplex::process(rd, s2m.out.writeBuf, out.writeBuf);
            }
        }
        sampleCount += rd;
        this->_in->flush();
        if (!out.swap(rd)) {
            flog::info("Does not write to output stream", totalRead);
            return -1;
        }
        return rd;
    };

    bool postprocess = true;
};

struct AudioFFTForDisplay : dsp::Processor<dsp::stereo_t, dsp::complex_t> {

    template <class X>
    using Arg = std::shared_ptr<X>;

    using base_type = dsp::Processor<dsp::stereo_t, dsp::complex_t>;


    std::vector<dsp::stereo_t> inputData;
    const int FREQUENCY = 48000;
    const int FRAMERATE = 60;
    Arg<fftwPlan> plan;
    const int windowSize = FREQUENCY / FRAMERATE;
    ComplexArray inputArray;
    FloatArray hanningWin;
    std::atomic<int> receivedSamplesCount = 0;
    dsp::stream<dsp::stereo_t> strm;
    dsp::routing::Splitter<dsp::stereo_t>* in;


    explicit AudioFFTForDisplay(dsp::routing::Splitter<dsp::stereo_t>* in) {
        init(&strm);
        this->in = in;
        inputData.reserve(4096);
        plan = dsp::arrays::allocateFFTWPlan(false, windowSize);
        inputArray = npzeros_c(windowSize);
        hanningWin = nphanning(windowSize);
        hanningWin = div(mul(hanningWin, (float)windowSize), npsum(hanningWin));
    }

    ~AudioFFTForDisplay() override {
        if (running) {
            stop();
        }
    }

//    bool started

    void start() override {
        if (!running) {
            base_type::start();
            in->bindStream(&strm);
        }
    }

    void stop() override {
        if (running) {
            in->unbindStream(&strm);
            base_type::stop();
        }
    }

    int run() override {
        //        flog::info("AudioFFTForDisplay.read...()");
        int rd = 0;
        if (inputData.size() < windowSize) {
            rd = this->strm.read();
//            flog::info("AudioFFTForDisplay.read rd={}", rd);
            if (rd < 0) {
                return rd;
            }
        }
        receivedSamplesCount.fetch_add(rd);
        auto offset = inputData.size();
        inputData.resize(inputData.size() + rd);
        std::copy(strm.readBuf + 0, strm.readBuf + rd, inputData.begin() + offset);
        if (inputData.size() >= windowSize) {
            for (int q = 0; q < windowSize; q++) {
                (*inputArray)[q] = dsp::complex_t{ inputData[q].l, 0 };
            }
            //            static int counter = 0;
            //            if (counter++ % 30 == 0) {
            //                auto ib = (float*)(*inputArray).data();
            //                char buf[1000];
            //                sprintf(buf, "input: %f %f %f %f %f %f %f %f", ib[0], ib[1], ib[2], ib[3], ib[4], ib[5], ib[6], ib[7]);
            //                std::string sbuf = buf;
            //                flog::info("in qsopanel: {}", sbuf);
            //            }
            auto inputMul = muleach(hanningWin, inputArray);
            auto result = dsp::arrays::npfftfft(inputMul, plan);
            std::copy(result->begin(), result->end(), this->out.writeBuf);
            this->out.swap((int)result->size());
            //            flog::info("input data size: {} erase {}", inputData.size(), result->size());
            inputData.erase(inputData.begin(), inputData.begin() + windowSize);
        }
        this->strm.flush();
        return rd;
    };
};

struct CWPanel {

    bool leftPressed = false;
    bool rightPressed = false;

    long long currentTime = 0;

    const int DOT = 1;
    const int DA = 2;
    int dot = 4;
    int state = 0; // 0 = nothing 1 = dot 2 = das
    int stateTime = 0;

    bool daWanted = false;
    bool dotWanted = false;


    void draw() {
        ImVec2 p1 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        ImGui::GetWindowDrawList()->AddRectFilled(p1, p1 + canvas_size, IM_COL32(40, 40, 40, 255));
        ImGui::Text("CW Panel here");

        auto windowPos = ImGui::GetWindowPos();
        auto widgetPos = windowPos + ImGui::GetCursorPos();
        auto avail = ImGui::GetContentRegionAvail();
        //        auto window = ImGui::GetCurrentWindow();

#ifndef __ANDROID__
        auto mouseCoord = ImVec2(ImGui::GetMousePos().x - widgetPos.x, ImGui::GetMousePos().y - widgetPos.y);
        bool inside = withinRect(avail, mouseCoord);
        bool newLeftPressed = inside && ImGui::IsMouseDown(1); // inverted: mouse faces the palm with eyes, two fingers (big and pointer) lay on the mouse like on paddle.
        bool newRightPressed = inside && ImGui::IsMouseDown(0);
#else
        static int fingers[10]; // 0 = nothing, 1 = left, 2 = right
        bool newLeftPressed = false;
        bool newRightPressed = false;
        for (int i = 0; i < 10; i++) {
            if (ImGui::IsFingerDown(i) && fingers[i] == 0) {
                auto coord = ImVec2(ImGui::GetFingerPos(i).x - widgetPos.x, ImGui::GetFingerPos(i).y - widgetPos.y);
                bool inside = withinRect(avail, coord);
                if (inside) {
                    auto halfSize = avail;
                    halfSize.x /= 2;
                    bool left = withinRect(halfSize, coord);
                    if (left) {
                        fingers[i] = 1;
                    }
                    else {
                        fingers[i] = 2;
                    }
                }
                // just pressed
            }
            if (!ImGui::IsFingerDown(i) && fingers[i] != 0) {
                fingers[i] = 0;
                // just released
            }
            if (fingers[i] == 1) newLeftPressed = true;
            if (fingers[i] == 2) newRightPressed = true;
        }
#endif
        if (newLeftPressed != leftPressed) {
            leftPressed = newLeftPressed;
            if (newLeftPressed) {
                sendClickSound();
            }
        }
        if (newRightPressed != rightPressed) {
            rightPressed = newRightPressed;
            if (rightPressed) {
                sendClickSound();
            }
        }

        bool sendDa = leftPressed || daWanted;
        bool sendDot = rightPressed || dotWanted;

        if (sendDa) daWanted = true;
        if (sendDot) dotWanted = true;

        stateTime++;
        if (state == DOT && stateTime == dot * 2                    // dot tone, dot silence
            || state == DA && stateTime == dot * 4 || state == 0) { // da 3*tone, da silence
            // time to make a decision
            if (sendDot && sendDa) {
                if (state == DA) {
                    state = DOT;
                }
                else {
                    state = DA;
                }
                // nothing
            }
            else if (sendDot) {
                dotWanted = false;
                state = DOT;
            }
            else if (sendDa) {
                daWanted = false;
                state = DA;
            }
            else {
                state = 0;
            }
            stateTime = 0;
        }
        if (stateTime == 0 && state != 0) {
            // start any tone
            flog::info("Set Tone Enabled: true, time={}, ctm={}", (int64_t)currentTime, (int64_t)currentTimeMillis());
            setToneEnabled(true);
        }
        if (state == DOT && stateTime == dot || state == DA && stateTime == 3 * dot) {
            // stop tone at specified time
            flog::info("Set Tone Enabled: OFF , time={}, ctm={}", (int64_t)currentTime, (int64_t)currentTimeMillis());
            setToneEnabled(false);
        }
        if (state == DA) {
            daWanted = false;
        }
        if (state == DOT) {
            dotWanted = false;
        }

        if (clickSendTime == currentTime) {
            sigpath::sinkManager.toneGenerator.store(2);
        }
        else if (toneEnabled) {
            sigpath::sinkManager.toneGenerator.store(1);
        }
        else {
            sigpath::sinkManager.toneGenerator.store(0);
        }
        currentTime++;
    }


    bool toneEnabled = false;
    long long clickSendTime = 0;
    void setToneEnabled(bool enabled) {
        toneEnabled = enabled;
    }

    void sendClickSound() {
        clickSendTime = currentTime;
    }
};

struct SimpleRecorder {
    enum {
        RECORDING_IDLE,
        RECORDING_STARTED,
        PLAYING_STARTED,
    } mode = RECORDING_IDLE;
    dsp::stream<dsp::stereo_t> recorderIn;
    dsp::stream<dsp::stereo_t> recorderOut;
    std::vector<dsp::stereo_t> data;
    bool keepPlaying = true;

    dsp::routing::Splitter<dsp::stereo_t>* inputAudio;

    explicit SimpleRecorder(dsp::routing::Splitter<dsp::stereo_t>* inputAudio) : inputAudio(inputAudio) {
        recorderOut.origin = "simpleRecorder.recorderOut";
        recorderIn.origin = "simpleRecorder.recorderIn";
    }

    void startRecording() {
        if (mode == RECORDING_IDLE) {
            data.clear();
            mode = RECORDING_STARTED;
            sigpath::sinkManager.setAllMuted(true);
            recorderIn.clearReadStop();
            recorderIn.clearWriteStop();
            inputAudio->bindStream(&recorderIn);
            std::thread x([this] {
                while (true) {
                    auto rd = recorderIn.read();
                    if (rd < 0) {
                        flog::info("recorder.read() causes loop break, rd={0}", rd);
                        break;
                    }
                    auto dest = data.size();
                    data.resize(data.size() + rd);
                    std::copy(recorderIn.readBuf, recorderIn.readBuf + rd, data.begin() + (long)dest);
                    recorderIn.flush();
//                    flog::info("recorder.read() rd={0}, dest={1}, data.size()={2}", rd, dest, data.size());
                }
            });
            x.detach();
        }
    }

    void startPlaying() {
        if (!data.empty()) {
            auto radioName = gui::waterfall.selectedVFO;
            if (!radioName.empty()) {
                mode = PLAYING_STARTED;
                keepPlaying = true;
                std::thread x([this] {
                    flog::info("startPlaying: start");
                    dsp::routing::Merger<dsp::stereo_t>* merger = sigpath::sinkManager.getMerger(gui::waterfall.selectedVFO);
                    recorderOut.clearReadStop();
                    recorderOut.clearWriteStop();
                    merger->bindStream(-10, &recorderOut);
                    auto waitTil = (double)currentTimeMillis();
                    for (int i = 0; i < data.size() && keepPlaying; i += 1024) {
                        auto ctm = currentTimeMillis();
                        if (ctm < (long long)waitTil) {
                            std::this_thread::sleep_for(std::chrono::milliseconds((int64_t)waitTil - ctm));
                        }
                        size_t blockEnd = i + 1024;
                        if (blockEnd > data.size()) {
                            blockEnd = data.size();
                        }
                        std::copy(data.data() + i, data.data() + blockEnd, recorderOut.writeBuf);
                        waitTil += 1000 * (blockEnd - i) / 48000.0;
//                        flog::info("player Swapping to {}: {0} - {1} = {}, wait til {}", audioOut.origin, blockEnd, i, blockEnd - i, (int64_t)waitTil);
                        recorderOut.swap(blockEnd - i);
//                        flog::info("player Swapped to {}: {} - {} = {}", audioOut.origin, blockEnd, i, blockEnd - i);
                    }
                    flog::info("startPlaying: stop");
                    merger->unbindStream(&recorderOut);
                    mode = RECORDING_IDLE;
                });
                x.detach();
            }
        }
    }
    void stop() {
        switch (mode) {
        case PLAYING_STARTED:
            keepPlaying = false;
            break;
        case RECORDING_STARTED:
            sigpath::sinkManager.setAllMuted(false);
            inputAudio->unbindStream(&recorderIn);
            recorderIn.stopReader();
            mode = RECORDING_IDLE;
            break;
        default:
            break;
        }
    }
};

struct Equalizer : public dsp::Processor<dsp::complex_t, dsp::complex_t> {

    dsp::filter::FIR<dsp::complex_t, dsp::complex_t> flt;

    std::vector<dsp::complex_t> design_bandpass_filter(float center_freq, float bandwidth, float fs) {

        float lowcut = (center_freq - bandwidth / 2) / (fs / 2);
        float highcut = (center_freq + bandwidth / 2) / (fs / 2);



        auto taps = dsp::taps::bandPass<dsp::complex_t>(center_freq-bandwidth/2, center_freq+bandwidth/2, bandwidth*1.5, fs, true);
        std::vector<dsp::complex_t> filter(taps.size);

        // Apply a window function (e.g., Hamming window)
        for (int i = 0; i < taps.size; ++i) {
            taps.taps[i] *= 0.54 - 0.46 * std::cos(2 * M_PI * i / (taps.size - 1));
        }
        for (int i = 0; i < taps.size; ++i) {
            filter[i] = taps.taps[i];
        }

        dsp::taps::free(taps);

        return filter;
    }

    Equalizer() : dsp::Processor<dsp::complex_t, dsp::complex_t>() {
        std::vector<float> center_freqs = {800, 1200, 1800, 2400};
        std::vector<float> bandwidths = {800, 800, 800, 800};
        std::vector<float> gains = {0.3, 1.0, 2.0, 2.0};

        std::vector<dsp::complex_t> equalizer;

        for (size_t i = 0; i < center_freqs.size(); ++i) {
            auto band_filter = design_bandpass_filter(center_freqs[i], bandwidths[i], 48000);
            for(int q=0; q<band_filter.size(); ++q) {
                band_filter[q] *= gains[i];
            }
            if (i == 0) {
                equalizer = band_filter;
            } else {
                for (int j = 0; j < band_filter.size(); ++j) {
                    equalizer[j] += band_filter[j];
                }
            }
        }


        auto taps = dsp::taps::alloc<dsp::complex_t>(equalizer.size());
        for (int i = 0; i < equalizer.size(); ++i) {
            taps.taps[i] = equalizer[i];
        }
        taps.size = equalizer.size();
        flt.init(nullptr, taps);

    }

    void process(size_t size, dsp::complex_t* in, dsp::complex_t* out) {
        flt.process(size, in, out);
    }



    int run() override {
        return 0;
    }
};

struct ConfigPanel {
    float agcAttack = 120.0f;
    float agcDecay = 0.1f;
//    bool doNR = true;
    bool doEqualize = true;

    // audio passband
    int highPass = 120;
    int lowPass = 2700;

    dsp::convert::StereoToMono s2m;
    dsp::convert::MonoToStereo m2s;
    dsp::loop::AGC<float> agc;
//    dsp::loop::AGC<dsp::complex_t> agc2;
    std::shared_ptr<dsp::AFNR_OMLSA_MCRA> afnr;
    Equalizer equalizer;
    dsp::convert::RealToComplex r2c;
    dsp::convert::ComplexToReal c2r;

    Decibelometer rawInDecibels;
    Decibelometer outDecibels;

    SimpleRecorder recorder;

    explicit ConfigPanel(dsp::routing::Splitter<dsp::stereo_t>* inputAudio) : recorder(inputAudio) {
//        afnr.setProcessingBandwidth(11000);
        agc.init(NULL, 1.0, agcAttack / 48000.0, agcDecay / 48000.0, 10e6, 10.0, INFINITY);
//        agc2.init(NULL, 1.0, agcAttack / 48000.0, agcDecay / 48000.0, 10e6, 10.0, INFINITY);
    }

    void draw();
};

struct QSOPanel {
    QSOPanel() : audioInProcessedIn(), audioInProcessed(&audioInProcessedIn) {
        audioIn.origin = "qsopanel.audioin";
        audioTowardsTransmitter.origin = "qsopanel.audioInToTransmitter";
        audioInProcessedIn.origin = "qsopanel.audioInProcessedIn";
        audioInProcessed.origin = "QSOPanel.audioInProcessed";
    }
    virtual ~QSOPanel();
    void startSoundPipeline();
    void stopSoundPipeline();
    void draw(float currentFreq, ImGui::WaterfallVFO* pVfo);
    dsp::stream<dsp::stereo_t> audioIn;
    dsp::stream<dsp::stereo_t> audioInProcessedIn;
    dsp::routing::Splitter<dsp::stereo_t> audioInProcessed;
    dsp::stream<dsp::stereo_t> audioTowardsTransmitter;
    std::shared_ptr<ConfigPanel> configPanel;


    //    int readSamples;
    //    const int fftFPS = 60;
    std::shared_ptr<AudioFFTForDisplay> audioInToFFT;
    std::shared_ptr<AudioInToTransmitter> audioInToTransmitter;
    std::shared_ptr<std::thread> receiveBuffers;
    dsp::arrays::FloatArray currentFFTBuffer;
    std::mutex currentFFTBufferMutex;
    void handleTxButton(bool tx, bool tune);
    float currentFreq;
    int txGain = 0;
    float micGain = 0; // in db
    bool enablePA = false;
    bool transmitting = false;
    void setModeSubmode(const std::string& mode, const std::string& submode);
    std::string submode;
    std::string mode;
    EventHandler<float> maxSignalHandler;
    std::atomic<float> maxSignal = 0.0;
    bool postprocess = true;
    int triggerTXOffEvent = 0;
    std::vector<float> lastSWR, lastForward, lastReflected;
    void drawHistogram();

    float maxTxSignalPeak = 0.0;
    int64_t maxTxSignalPeakTime = 0;
};


bool MobileButton::isLongPress() {
    bool retval = this->currentlyPressedTime != 0 && this->currentlyPressed && currentTimeMillis() > this->currentlyPressedTime + 1000;
    if (retval) {
        this->currentlyPressedTime = 0; // prevent sending click event
    }
    return retval;
}

bool MobileButton::draw() {

    bool retval = false;
    ImGui::PushFont(style::mediumFont);
    auto windowPos = ImGui::GetWindowPos();
    auto widgetPos = windowPos + ImGui::GetCursorPos();
    auto avail = ImGui::GetContentRegionAvail();
    auto window = ImGui::GetCurrentWindow();

    auto mouseCoord = ImVec2(ImGui::GetMousePos().x - widgetPos.x, ImGui::GetMousePos().y - widgetPos.y);

    //    auto diameter = std::min<float>(avail.x, avail.y);
    //    float radius = diameter / 2 * this->sizeFactor;


    ImU32 bg0 = ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    //    const ImVec2& buttonCenter = avail / 2;

    bool pressed = ImGui::IsAnyMouseDown() && ImGui::GetTopMostPopupModal() == NULL;
    bool startedInside = withinRect(avail, this->pressPoint);
    ImGuiContext& g = *GImGui;
    if (pressed) {
        const float t = g.IO.MouseDownDuration[0];
        if (t < 0.2) {
            if (isnan(this->pressPoint.x)) {
                this->pressPoint = mouseCoord;
                if (withinRect(avail, this->pressPoint)) {
                    this->currentlyPressed = true; // not cleared whe   n finger away of button
                    this->currentlyPressedTime = currentTimeMillis();
                }
            }
        }
    }
    else {
        this->currentlyPressed = false;
        if (startedInside && withinRect(avail, mouseCoord)) {
            // released inside
            if (currentlyPressedTime != 0) { // handle release, longpress not happened
                retval = true;
            }
        }
        currentlyPressedTime = 0;
        this->pressPoint = ImVec2((float)nan(""), (float)nan(""));
    }

    if (pressed && startedInside && withinRect(avail, mouseCoord)) {
        if (this->currentlyPressedTime != 0) {
            bg0 = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
        }
        else {
            // remove the highlight after long press
        }
    }

    window->DrawList->AddRectFilled(windowPos, windowPos + avail, bg0, (float)avail.y / 10.0);


    //    auto ts = ImGui::CalcTextSize(this->upperText.c_str());
    //    ImGui::SetCursorPos((ImVec2{avail.x, 0} - ImVec2{ts.x, 0}) / 2);
    //    ImGui::Text("%s", this->upperText.c_str());

    const char* buttonTextStr = this->buttonText.c_str();
    //    if (this->buttonText.size() == 1) {
    //        ImGui::PushFont(style::bigFont);
    //    }
    auto ts = ImGui::CalcTextSize(buttonTextStr);
    ImGui::SetCursorPos((avail - ImVec2{ ts.x, ts.y }) / 2);
    ImGui::Text("%s", buttonTextStr);
    //    if (this->buttonText.size() == 1) {
    //        ImGui::PopFont();
    //    }
    ImGui::PopFont();

    return retval;
}

double TheEncoder::draw(ImGui::WaterfallVFO* vfo) {
    auto avail = ImGui::GetContentRegionAvail();
    int MARKS_PER_ENCODER = 20;
    float retval = 0;


    float R = avail.y / 2;
    auto window = ImGui::GetCurrentWindow();
    auto widgetPos = ImGui::GetWindowPos();
    auto DRAW_HEIGHT_PERCENT = 0.95f;

    ImU32 bg0 = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    window->DrawList->AddRectFilled(widgetPos, widgetPos + avail, bg0);

    for (int q = 0; q < MARKS_PER_ENCODER; q++) {
        auto markY = (R * DRAW_HEIGHT_PERCENT) * (float)sin(M_PI * 2 / MARKS_PER_ENCODER * q + this->somePosition);
        auto markColor = (float)cos(M_PI * 2 / MARKS_PER_ENCODER * q + this->somePosition);
        if (markColor > 0) {
            ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, markColor));
            window->DrawList->AddRectFilled(widgetPos + ImVec2(0, R + markY - 4), widgetPos + ImVec2(avail.x, R + markY + 4), col);
        }
    }

    if (ImGui::IsAnyMouseDown()) {
        auto mouseX = ImGui::GetMousePos().x - widgetPos.x;
        bool mouseInside = mouseX >= 0 && mouseX < avail.x;
        if (!mouseInside && isnan(lastMouseAngle)) {
            // clicked outside
        }
        else {
            // can continue everywhere
            auto mouseAngle = (ImGui::GetMousePos().y - widgetPos.y - R) / (R * DRAW_HEIGHT_PERCENT);
            if (!isnan(lastMouseAngle)) {
                this->somePosition += mouseAngle - lastMouseAngle;
                retval = (float)(mouseAngle - lastMouseAngle);
            }
            else {
                // first click!
                auto currentFreq = vfo ? (vfo->generalOffset + gui::waterfall.getCenterFrequency()) : gui::waterfall.getCenterFrequency();
                this->currentFrequency = (float)currentFreq;
            }
            lastMouseAngle = mouseAngle;
            this->speed = 0;
            fingerMovement.emplace_back(lastMouseAngle);
        }
    }
    else {
        auto sz = this->fingerMovement.size();
        if (sz >= 2) {
            this->speed = (this->fingerMovement[sz - 1] - this->fingerMovement[sz - 2]) / 2;
        }
        this->fingerMovement.clear();
        lastMouseAngle = nan("");
    }
    if (fabs(speed) < 0.001) {
        if (speed != 0) {
            speed = 0;
        }
    }
    this->somePosition += speed;
    if (speed != 0) {
        retval = (float)speed;
    }
    this->speed *= this->delayFactor;
    return retval;
}

void MobileMainWindow::updateSubmodeAfterChange() {
    auto mode = getCurrentMode();
    auto submode = getCurrentModeAttr("submode");
    if (submode.empty()) {
        submode = subModes[mode].front();
        setCurrentModeAttr("submode", submode);
    }
    this->submodeToggle.upperText = submode;
    ImGui::WaterfallVFO*& pVfo = gui::waterfall.vfos[gui::waterfall.selectedVFO];
    if (pVfo) {
        auto selectedDemod = RadioModule::RADIO_DEMOD_USB;
        if (submode == "LSB") selectedDemod = RadioModule::RADIO_DEMOD_LSB;
        if (submode == "CWU") selectedDemod = RadioModule::RADIO_DEMOD_CW;
        if (submode == "CWL") selectedDemod = RadioModule::RADIO_DEMOD_CW;
        if (submode == "CW") selectedDemod = RadioModule::RADIO_DEMOD_CW;
        if (submode == "NFM") selectedDemod = RadioModule::RADIO_DEMOD_NFM;
        if (submode == "WFM") selectedDemod = RadioModule::RADIO_DEMOD_WFM;
        if (submode == "AM") selectedDemod = RadioModule::RADIO_DEMOD_AM;
        pVfo->onUserChangedDemodulator.emit((int)selectedDemod);
    }
    updateFrequencyAfterChange();
}

void MobileMainWindow::updateFrequencyAfterChange() {
    auto mode = getCurrentMode();
    auto submode = getCurrentModeAttr("submode");
    auto band = getCurrentBand();
    auto maybeFreq = getCurrentModeAttr("freq_" + submode + "_" + band);
    this->bandUp.upperText = band;
    if (!maybeFreq.empty()) {
        double currentFreq = strtod(maybeFreq.c_str(), nullptr);
        tuner::tune(tuner::TUNER_MODE_CENTER, gui::waterfall.selectedVFO, currentFreq);
        return;
    }
    auto nfreq = this->frequencyDefaults[band + "_" + submode];
    if (nfreq == 0)
        nfreq = this->frequencyDefaults[band + "_" + mode];
    if (nfreq == 0)
        nfreq = this->frequencyDefaults[band];
    if (nfreq != 0) {
        tuner::tune(tuner::TUNER_MODE_CENTER, gui::waterfall.selectedVFO, nfreq * 1000.0);
    }
}

void MobileMainWindow::autoDetectBand(int frequency) {
    static int lastFrequency = 0;
    static const std::string* lastFreqBand = nullptr;
    const std::string* newBand;

    if (frequency == lastFrequency) {
        newBand = lastFreqBand;
    }
    else {
        newBand = &this->getBand(frequency);
        lastFrequency = frequency;
        lastFreqBand = newBand;
    }
    if (*newBand != this->bandUp.upperText) {
        this->bandUp.upperText = *newBand;
    }
}

void MobileMainWindow::updateAudioWaterfallPipeline() {
    if (gui::waterfall.selectedVFO != currentAudioStreamName) {
        if (!currentAudioStreamName.empty()) {
            currentAudioStream->stopReader();
            sigpath::sinkManager.unbindStream(currentAudioStreamName, currentAudioStream);
        }
        currentAudioStreamName = gui::waterfall.selectedVFO;
        if (currentAudioStreamName.empty()) {
            currentAudioStream = nullptr;
        } else {
            currentAudioStreamSampleRate = (int)sigpath::sinkManager.getStreamSampleRate(currentAudioStreamName);
            currentAudioStream = sigpath::sinkManager.bindStream(currentAudioStreamName);
            std::thread x([&]() {
                while(true) {
                    int rd = currentAudioStream->read();
                    if (rd < 0) {
                        break;
                    }
                    audioWaterfall->addAudioSamples(currentAudioStream->readBuf, rd, currentAudioStreamSampleRate);
                    currentAudioStream->flush();
                }
            });
            x.detach();
        }
    }
    if (currentAudioStreamName != "") {
        currentAudioStreamSampleRate = (int)sigpath::sinkManager.getStreamSampleRate(currentAudioStreamName);
    }
}

void MobileMainWindow::draw() {

    updateAudioWaterfallPipeline();



    gui::waterfall.alwaysDrawLine = false;
    if (displaymenu::transcieverLayout == 0) {
        shouldInitialize = true;
        MainWindow::draw();
        return;
    }
    if (shouldInitialize) {
        shouldInitialize = false;
        if (getCurrentBand().empty()) {
            selectCurrentBand("20M", -1);
        }
        bandUp.upperText = this->getCurrentBand();
        auto currentMode = this->getCurrentMode();
        modeToggle.upperText = currentMode;
        updateSubmodeAfterChange();
        zoomToggle.upperText = "custom";
    }
    gui::waterfall.alwaysDrawLine = true;
    ImGui::WaterfallVFO* vfo = nullptr; // gets initialized below
    this->preDraw(&vfo);

    ImGui::Begin("Main", nullptr, WINDOW_FLAGS);

    ImVec4 textCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    ImVec2 btnSize(30 * style::uiScale, 30 * style::uiScale);
    ImGui::PushID((int)ImGui::GetID("sdrpp_menu_btn"));
    if (ImGui::ImageButton(icons::MENU, btnSize, ImVec2(0, 0), ImVec2(1, 1), 5, ImVec4(0, 0, 0, 0), textCol) || ImGui::IsKeyPressed(ImGuiKey_Menu, false)) {
        showMenu = !showMenu;
        core::configManager.acquire();
        core::configManager.conf["showMenu"] = showMenu;
        core::configManager.release(true);
    }
    ImGui::PopID();

    ImGui::SameLine();

    this->drawUpperLine(vfo);

    auto cornerPos = ImGui::GetCursorPos();

    float encoderWidth = 150;
    float defaultButtonsWidth = 300;
    float buttonsWidth = defaultButtonsWidth;
    float statusHeight = 100;
    switch (qsoMode) {
    case VIEW_QSO:
        buttonsWidth = 600;
        break;
    case VIEW_CONFIG:
        buttonsWidth = 1200;
        break;
    default:
        break;
    }

    const ImVec2 waterfallRegion = ImVec2(ImGui::GetContentRegionAvail().x - encoderWidth - buttonsWidth, ImGui::GetContentRegionAvail().y - statusHeight);

    lockWaterfallControls = showMenu || (qsoMode != VIEW_DEFAULT && modeToggle.upperText == "CW");
    ImGui::BeginChildEx("Waterfall", ImGui::GetID("sdrpp_waterfall"), waterfallRegion, false, 0);
    auto waterfallStart = ImGui::GetCursorPos();
    gui::waterfall.draw();
    ImGui::SetCursorPos(waterfallStart + ImVec2(gui::waterfall.fftAreaMin.x + 5 * style::uiScale, 0));
    ImGui::PushFont(style::mediumFont);
    ImGui::Text("BAND: %s", this->bandUp.upperText.c_str());
    ImGui::PopFont();

    onWaterfallDrawn.emit(GImGui);
    ImGui::EndChild();

    this->handleWaterfallInput(vfo);

    ImGui::SetCursorPos(cornerPos + ImVec2(gui::waterfall.fftAreaMin.x + 5 * style::uiScale, waterfallRegion.y));
    ImGui::PushFont(style::mediumFont);
    ImGui::Text("%s -> %s    zoom: %s",
                this->modeToggle.upperText.c_str(),
                this->submodeToggle.upperText.c_str(),
                this->zoomToggle.upperText.c_str());
    ImGui::PopFont();

    ImGui::SetCursorPos(cornerPos);


    if (showMenu) {
        menuWidth = core::configManager.conf["menuWidth"];
        const ImVec2 menuRegion = ImVec2((float)menuWidth, ImGui::GetContentRegionAvail().y);
        ImGui::BeginChildEx("Menu", ImGui::GetID("sdrpp_menu"), menuRegion, false, 0);
        ImU32 bg = ImGui::ColorConvertFloat4ToU32(gui::themeManager.waterfallBg);
        auto window = ImGui::GetCurrentWindow();
        auto widgetPos = ImGui::GetWindowContentRegionMin();
        auto rectRegion = menuRegion;
        rectRegion.y += 10000; // menu is scrolling, rect somehow scrolls with it, so must be big enough to be always visible
        window->DrawList->AddRectFilled(widgetPos, widgetPos + rectRegion, bg);

        if (gui::menu.draw(firstMenuRender)) {
            core::configManager.acquire();
            json arr = json::array();
            for (int i = 0; i < gui::menu.order.size(); i++) {
                arr[i]["name"] = gui::menu.order[i].name;
                arr[i]["open"] = gui::menu.order[i].open;
            }
            core::configManager.conf["menuElements"] = arr;

            // Update enabled and disabled modules
            for (auto [_name, inst] : core::moduleManager.instances) {
                if (!core::configManager.conf["moduleInstances"].contains(_name)) { continue; }
                core::configManager.conf["moduleInstances"][_name]["enabled"] = inst.instance->isEnabled();
            }

            core::configManager.release(true);
        }
        if (startedWithMenuClosed) {
            startedWithMenuClosed = false;
        }
        else {
            firstMenuRender = false;
        }
        ImGui::EndChild();
    }

    ImGui::SetCursorPos(cornerPos + ImVec2{ waterfallRegion.x + buttonsWidth, 0 });
    const ImVec2 encoderRegion = ImVec2(encoderWidth, ImGui::GetContentRegionAvail().y);
    ImGui::BeginChildEx("Encoder", ImGui::GetID("sdrpp_encoder"), encoderRegion, false, 0);
    double offsetDelta = encoder.draw(vfo);
    if (offsetDelta != 0) {
        auto externalFreq = vfo ? (vfo->generalOffset + gui::waterfall.getCenterFrequency()) : gui::waterfall.getCenterFrequency();
        if (fabs(externalFreq - encoder.currentFrequency) > 10000) {
            // something changed! e.g. changed band when encoder was rotating
            encoder.speed = 0;
        }
        else {
            double od = offsetDelta * 100;
            double sign = od < 0 ? -1 : 1;
            if (fabs(od) < 2) {
                od = 0.5 * sign;
            }
            else {
                od = fabs(od) - 1; // 4 will become 1
                od = pow(od, 1.4) * sign;
                //                od *= 5;
            }
            flog::info("od={} encoder.currentFrequency={}", od, encoder.currentFrequency);
            encoder.currentFrequency -= od;
            auto cf = ((int)(encoder.currentFrequency / 10)) * 10.0;
            tuner::tune(tuningMode, gui::waterfall.selectedVFO, cf);
        }
    }
    auto currentFreq = vfo ? (vfo->generalOffset + gui::waterfall.getCenterFrequency()) : gui::waterfall.getCenterFrequency();
    this->autoDetectBand((int)currentFreq);
    ImGui::EndChild(); // encoder


    ImGui::SetCursorPos(cornerPos + ImVec2{ waterfallRegion.x, 0 });
    auto vertPadding = ImGui::GetStyle().WindowPadding.y;


    MobileButton* buttonsDefault[] = {  &this->qsoButton, &this->zoomToggle, &this->modeToggle /*&this->bandUp, &this->bandDown, &this->submodeToggle,*/ };
    MobileButton* buttonsQso[] = { &this->qsoButton, &this->txButton, &this->softTune };
    MobileButton* buttonsConfig[] = { &this->exitConfig };
    auto nButtonsQso = (int)((sizeof(buttonsQso) / sizeof(buttonsQso[0])));
    auto nButtonsDefault = (int)((sizeof(buttonsDefault) / sizeof(buttonsDefault[0])));
    auto nButtonsConfig = (int)((sizeof(buttonsConfig) / sizeof(buttonsConfig[0])));
    auto buttonsSpaceY = ImGui::GetContentRegionAvail().y;
    switch (this->qsoMode) {
    case VIEW_QSO:
        qsoButton.buttonText = "End QSO";
    case VIEW_DEFAULT:
        qsoButton.buttonText = "QSO";
    default:
        break;
    }
    const ImVec2 buttonsRegion = ImVec2(buttonsWidth, buttonsSpaceY);
    ImGui::BeginChildEx("Buttons", ImGui::GetID("sdrpp_mobile_buttons"), buttonsRegion, false, ImGuiWindowFlags_NoScrollbar);

    auto beforePanel = ImGui::GetCursorPos();
    if (this->qsoMode == VIEW_CONFIG) {
        ImGui::BeginChildEx("Config", ImGui::GetID("sdrpp_mobconffig"), ImVec2(buttonsWidth, ImGui::GetContentRegionAvail().y), false, 0);
        configPanel->draw();
        ImGui::EndChild(); // buttons
    }
    if (this->qsoMode == VIEW_QSO) {
        ImGui::BeginChildEx("QSO", ImGui::GetID("sdrpp_qso"), ImVec2(buttonsWidth, ImGui::GetContentRegionAvail().y), false, 0);
        qsoPanel->draw((float)currentFreq, vfo);
        //        auto afterQSOPanel = ImGui::GetCursorPos();
        ImGui::EndChild(); // buttons

        //        ImGui::SetCursorPos(ImVec2{beforeQSOPanel.x, beforeQSOPanel.y + afterQSOPanel.y});
        //        buttonsSpaceY -= afterQSOPanel.y;
    }
    ImGui::SetCursorPos(beforePanel);


    MobileButton** buttons;
    int NBUTTONS;
    switch (this->qsoMode) {
    case VIEW_QSO:
        buttons = buttonsQso;
        NBUTTONS = nButtonsQso;
        break;
    case VIEW_DEFAULT:
        NBUTTONS = nButtonsDefault;
        buttons = buttonsDefault;
        break;
    case VIEW_CONFIG:
        NBUTTONS = nButtonsConfig;
        buttons = buttonsConfig;
        break;
    }
    MobileButton* pressedButton = nullptr;
    auto buttonsStart = ImGui::GetCursorPos();
    auto intButtonHeight = style::baseFont->FontSize * 3.3;
    for (auto b = 0; b < NBUTTONS; b++) {
        char chi[100];
        sprintf(chi, "mob_button_%d", b);
        const ImVec2 childRegion = ImVec2(defaultButtonsWidth, intButtonHeight - vertPadding);
        if (this->qsoMode == VIEW_DEFAULT && false) {
            // align from top
            ImGui::SetCursorPos(buttonsStart + ImVec2{ 0, buttonsSpaceY - float(nButtonsDefault - b) * (childRegion.y + vertPadding) });
        }
        else {
            // align from bottom
            ImGui::SetCursorPos(buttonsStart + ImVec2{ buttonsWidth - defaultButtonsWidth, buttonsSpaceY - float(b + 1) * (childRegion.y + vertPadding) });
        }
        ImGui::BeginChildEx(chi, ImGui::GetID(chi), childRegion, false, 0);
        auto pressed = buttons[b]->draw();
        ImGui::EndChild();
        if (pressed) {
            pressedButton = buttons[b];
        }
    }
    ImGui::EndChild(); // buttons

    if (qsoMode == VIEW_QSO && modeToggle.upperText == "CW") {
        ImGui::SetCursorPos(cornerPos + ImVec2(waterfallRegion.x / 4, waterfallRegion.y / 2));
        ImVec2 childRegion(waterfallRegion.x / 2, waterfallRegion.y / 2);
        ImGui::BeginChildEx("cwbuttons", ImGui::GetID("cwbuttons"), childRegion, true, 0);
        cwPanel->draw();
        ImGui::EndChild(); // buttons
    }

    ImGuiIO& io = ImGui::GetIO();
    if (true) {
        ImVec2 sz(io.DisplaySize.x / 6, io.DisplaySize.y / 4);
        audioWaterfall->draw(ImVec2(io.DisplaySize.x/2-sz.x/2, io.DisplaySize.y - sz.y-20), sz);
    }


    ImGui::End();

    auto makeZoom = [&](int selectedIndex) {
        zoomToggle.upperText = zooms[selectedIndex].first;
        updateWaterfallZoomBandwidth(zooms[selectedIndex].second);
        double wfBw = gui::waterfall.getBandwidth();
        auto nbw = wfBw;
        if (zooms[selectedIndex].second != 0) {
            nbw = zooms[selectedIndex].second;
        }
        if (nbw > wfBw) {
            nbw = wfBw;
        }
        gui::waterfall.setViewBandwidth(nbw);
        if (vfo) {
            gui::waterfall.setViewOffset(vfo->centerOffset); // center vfo on screen
        }
    };
    if (this->zoomToggle.isLongPress()) {
        makeZoom(0);
    }
    if (this->qsoButton.isLongPress()) {
        if (!this->qsoPanel->audioInToFFT) {
            this->qsoPanel->startSoundPipeline();
        }
        this->qsoMode = VIEW_CONFIG;
    }
    if (pressedButton == &this->exitConfig) {
        this->qsoMode = VIEW_DEFAULT;
        this->qsoPanel->stopSoundPipeline();
    }
    if (pressedButton == &this->zoomToggle) {
        int selectedIndex = -1;
        for (auto q = 0; q < zooms.size(); q++) {
            if (zooms[q].first == zoomToggle.upperText) {
                selectedIndex = q;
                break;
            }
        }
        selectedIndex = int((selectedIndex + 1) % zooms.size());
        makeZoom(selectedIndex);
    }

    if (pressedButton == &this->bandUp || pressedButton == &this->bandDown) {
        auto cb = getCurrentBand();
        auto cbIter = std::find(bands.begin(), bands.end(), cb);
        if (cbIter != bands.end()) {
            auto cbIndex = cbIter - bands.begin();
            if (pressedButton == &this->bandUp) {
                cbIndex = (int)(cbIndex + 1) % bands.size();
            }
            else {
                cbIndex = (int)(cbIndex + bands.size() - 1) % bands.size();
            }
            auto newBand = bands[cbIndex];
            selectCurrentBand(newBand, (int)currentFreq);
            if (getCurrentMode() == "SSB") {
                this->selectSSBModeForBand(newBand);
            }
            updateFrequencyAfterChange();
        }
    }
    const char * TxModePopup = "TX/RX Mode Select";
    if (pressedButton == &this->modeToggle) {
        ImGui::OpenPopup(TxModePopup);

//        auto curr = std::find(this->modes.begin(), this->modes.end(), getCurrentMode());
//        if (curr != this->modes.end()) {
//            auto currIndex = curr - this->modes.begin();
//            currIndex = int(currIndex + 1) % this->modes.size();
//            auto newMode = this->modes[currIndex];
//            this->leaveBandOrMode((int)currentFreq);
//            setCurrentMode(newMode);
//            pressedButton->upperText = newMode;
//            updateSubmodeAfterChange();
//        }
    }
    if (pressedButton == &this->qsoButton) {
        this->qsoMode = this->qsoMode == VIEW_DEFAULT ? VIEW_QSO : VIEW_DEFAULT;
        if (this->qsoMode == VIEW_QSO) {
            qsoPanel->startSoundPipeline();
        }
        else {
            qsoPanel->stopSoundPipeline();
        }
    }
    if (pressedButton == &this->submodeToggle) {
        std::vector<std::string>& submos = subModes[getCurrentMode()];
        auto submoIter = std::find(submos.begin(), submos.end(), getCurrentModeAttr("submode"));
        if (submoIter != submos.end()) {
            auto submoIndex = submoIter - submos.begin();
            submoIndex = int(submoIndex + 1) % submos.size();
            auto newSubmode = submos[submoIndex];
            setCurrentModeAttr("submode", newSubmode);
            updateSubmodeAfterChange();
        }
    }
    qsoPanel->setModeSubmode(modeToggle.upperText, submodeToggle.upperText);
    if (sigpath::transmitter && qsoPanel->triggerTXOffEvent < 0) { // transiver exists, and TX is not in handing off state
        if (pressedButton == &this->softTune) {
            if (qsoPanel->audioInToTransmitter) {
                // tuning, need to stop
                qsoPanel->handleTxButton(false, true);
#define SOFT_TUNE_LABEL "SoftTune"
                pressedButton->buttonText = SOFT_TUNE_LABEL;
            }
            else {
                // need to start
                qsoPanel->handleTxButton(true, true);
                pressedButton->buttonText = "Tuning..";
            }
        }
        if (txButton.currentlyPressed && !qsoPanel->audioInToTransmitter || !txButton.currentlyPressed && qsoPanel->audioInToTransmitter && qsoPanel->audioInToTransmitter->tuneFrequency == 0) {
            // button is pressed ok to send, or button is released and audioInToTransmitter running and it is not in tune mode
            qsoPanel->handleTxButton(this->txButton.currentlyPressed, false);
            //
        }
    }
    if (ImGui::BeginPopupModal(TxModePopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImVec2 screenSize = io.DisplaySize;
        screenSize.x *= 0.8;
        screenSize.y *= 0.8;
        ImGui::SetWindowSize(screenSize);
        ImGui::BeginChild("##TxModePopup", ImVec2(screenSize.x, screenSize.y - style::baseFont->FontSize * 4), true, 0);
        auto currentBand = getCurrentBand();
        ImGui::Text("Band:");
        ImGui::NewLine();
        for (auto &b : bands) {
            ImGui::SameLine();
            bool pressed = false;
            if (currentBand == b) {
                pressed = doFingerButton(">" + b + "<");
            } else {
                pressed = doFingerButton(b);
            }
            if (pressed && b != currentBand) {
                selectCurrentBand(b, (int)currentFreq);
                if (getCurrentMode() == "SSB") {
                    this->selectSSBModeForBand(b);
                }
                updateFrequencyAfterChange();
            }
        }
        ImGui::NewLine();
        ImGui::Text("Mode / Submode:");
        ImGui::NewLine();
        auto currentSubmode = getCurrentModeAttr("submode");
        std::string currentMode = "";
        for (auto &b : modes) {
            ImGui::SameLine();
            for(auto &bb : subModes[b]) {
                bool pressed = false;
                ImGui::SameLine();
                if (bb == currentSubmode) {
                    pressed = doFingerButton(">" + bb + "<");
                } else {
                    pressed = doFingerButton(bb);
                }
                if (pressed && bb != currentSubmode) {
                    setCurrentMode(b);
                    setCurrentModeAttr("submode", bb);
                    updateSubmodeAfterChange();
                    currentSubmode = bb;
                }
                if (bb == currentSubmode) {
                    currentMode = b;
                }
            }
        }
        ImGui::NewLine();
        ImGui::Text("Bandwidth:");
        ImGui::NewLine();
        auto *bw = &ssbBandwidths;
        if (currentMode == "CW") {
            bw = &cwBandwidths;
        }
        if (currentMode == "AM") {
            bw = &amBandwidths;
        }
        if (currentMode == "FM") {
            bw = &fmBandwidths;
        }
        auto bandwidthk = vfo->bandwidth/1000.0;
        char fmt[32];
        if (bandwidthk == floor(bandwidthk)) {
            sprintf(fmt, "%0f", bandwidthk);
        } else {
            sprintf(fmt, "%0.1f", bandwidthk);
        }
        for (auto &b : *bw) {
            ImGui::SameLine();
            bool pressed = false;
            if (b == fmt) {
                pressed = doFingerButton(">" + b + "<");
            } else {
                pressed = doFingerButton(b);
            }
            if (pressed) {
                if (bw == &cwBandwidths) {
                    vfo->setBandwidth(std::stof(b));    // in hz
                } else {
                    vfo->setBandwidth(std::stof(b) * 1000); // in khz
                }
            }
        }
        ImGui::EndChild();
        if (doFingerButton("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    else if (!sigpath::transmitter) {
        if (txButton.currentlyPressed) {
            sigpath::sinkManager.toneGenerator.store(1);
        }
        else {
            sigpath::sinkManager.toneGenerator.store(0);
        }
    }
}
std::string MobileMainWindow::getCurrentBand() {
    std::string retval;
    core::configManager.acquire();
    if (core::configManager.conf.find("mobileBand") == core::configManager.conf.end()) {
        core::configManager.release(false);
        return "20M";
    }
    retval = core::configManager.conf["mobileBand"];
    core::configManager.release(false);
    return retval;
}

std::string MobileMainWindow::getCurrentMode() {
    bool changed = false;
    std::string retval;
    core::configManager.acquire();
    if (core::configManager.conf.find("mobileRadioMode") == core::configManager.conf.end()) {
        core::configManager.conf["mobileRadioMode"] = this->modes.front();
        changed = true;
    }
    retval = core::configManager.conf["mobileRadioMode"];
    core::configManager.release(changed);
    return retval;
}

void MobileMainWindow::setCurrentMode(std::string mode) {
    core::configManager.acquire();
    core::configManager.conf["mobileRadioMode"] = mode;
    core::configManager.release(true);
}

void MobileMainWindow::leaveBandOrMode(int leavingFrequency) {
    auto submode = getCurrentModeAttr("submode");
    auto leavingBand = getCurrentBand();
    setCurrentModeAttr("freq_" + submode + "_" + leavingBand, std::to_string(leavingFrequency));
    if (submode == "USB" || submode == "LSB") {
        setCurrentModeAttr("band_" + leavingBand + "_ssb", submode);
    }
}

void MobileMainWindow::selectSSBModeForBand(const std::string& band) {
    auto maybeSubmode = getCurrentModeAttr("band_" + band + "_ssb");
    if (!maybeSubmode.empty()) {
        if (bandsLimits[band].first >= 10000000) {
            maybeSubmode = "USB";
        }
        else {
            maybeSubmode = "LSB";
        }
    }
    setCurrentModeAttr("submode", maybeSubmode);
    updateSubmodeAfterChange();
}

void MobileMainWindow::selectCurrentBand(const std::string& band, int leavingFrequency) {
    if (getCurrentBand() != "" && leavingFrequency > 0) {
        this->leaveBandOrMode(leavingFrequency);
    }
    core::configManager.acquire();
    core::configManager.conf["mobileBand"] = band;
    core::configManager.release(true);
}

std::string MobileMainWindow::getCurrentModeAttr(const std::string& key) {
    std::string retval;
    auto currentMode = getCurrentMode();
    core::configManager.acquire();
    json x = core::configManager.conf["mobileRadioMode_" + currentMode];
    if (!x.empty()) {
        if (x.find(key) != x.end()) {
            retval = x[key];
        }
    }
    core::configManager.release(false);
    return retval;
}

void MobileMainWindow::setCurrentModeAttr(const std::string& key, std::string val) {
    auto currentMode = getCurrentMode();
    core::configManager.acquire();
    json x = core::configManager.conf["mobileRadioMode_" + currentMode];
    if (!x.empty()) {
        x[key] = val;
        core::configManager.conf["mobileRadioMode_" + currentMode] = x;
    }
    else {
        x = json();
        x[key] = val;
        core::configManager.conf["mobileRadioMode_" + currentMode] = x;
    }
    core::configManager.release(true);
}

const std::string& MobileMainWindow::getBand(int frequency) {
    static std::string empty;
    for (auto& b : bandsLimits) {
        if (frequency >= b.second.first && frequency <= b.second.second) {
            return b.first;
        }
    }
    return empty;
}

MobileMainWindow::MobileMainWindow() : MainWindow(),
                                       bandUp("14 Mhz", "+"),
                                       bandDown("", "-"),
                                       zoomToggle("custom", "Zoom"),
                                       modeToggle("SSB", "Mode"),
                                       submodeToggle("LSB", "Submode"),
                                       qsoButton("", "QSO"),
                                       txButton("", "TX"),
                                       exitConfig("", "OK"),
                                       softTune("", SOFT_TUNE_LABEL) {
    qsoPanel = std::make_shared<QSOPanel>();
    configPanel = std::make_shared<ConfigPanel>(&qsoPanel->audioInProcessed);
    qsoPanel->configPanel = configPanel;
    cwPanel = std::make_shared<CWPanel>();
    audioWaterfall = std::make_shared<SubWaterfall>();
}

void MobileMainWindow::init() {
    MainWindow::init();
    audioWaterfall->init();
}

void MobileMainWindow::end() {
    MainWindow::end();
    qsoPanel.reset();
    cwPanel.reset();
    configPanel.reset();
}


void QSOPanel::startSoundPipeline() {
    if (!configPanel->afnr) {
        configPanel->afnr = std::make_shared<dsp::AFNR_OMLSA_MCRA>();
        configPanel->afnr->omlsa_mcra.setSampleRate(48000);
        configPanel->afnr->allowed = true;
    }
    audioIn.clearReadStop();
    sigpath::sinkManager.defaultInputAudio.bindStream(&audioIn);
    std::thread inputProcessor([this] {
        int64_t start = currentTimeMillis();
        int64_t count = 0;

        dsp::filter::FIR<float, float> hipass;
        dsp::filter::FIR<dsp::stereo_t, float> lopass;
        auto prevHighPass = configPanel->highPass;
        auto prevLowPass = configPanel->lowPass;
        auto hipassTaps = dsp::taps::highPass0<float>(prevHighPass, 30, 48000);
        hipass.init(nullptr, hipassTaps);
        auto lopassTaps = dsp::taps::lowPass0<float>(prevLowPass, 300, 48000);
        lopass.init(nullptr, lopassTaps);

        while (true) {
            int rd = audioIn.read();
            //            flog::info("audioIn.read() = {}", rd);
            if (rd < 0) {
                flog::info("Breaking the tx audio in loop");
                break;
            }
            configPanel->rawInDecibels.addSamples(audioIn.readBuf, rd);
            if (configPanel->lowPass != prevLowPass) {
                dsp::taps::free(lopassTaps);
                lopassTaps = dsp::taps::lowPass0<float>(configPanel->lowPass, 30, 48000);
                lopass.setTaps(lopassTaps);
                prevLowPass = configPanel->lowPass;
            }
            if (configPanel->highPass != prevHighPass) {
                dsp::taps::free(hipassTaps);
                hipassTaps = dsp::taps::highPass0<float>(configPanel->highPass, 3000, 48000);
                hipass.setTaps(hipassTaps);
                prevHighPass = configPanel->highPass;
            }
            configPanel->s2m.process(rd, audioIn.readBuf, configPanel->s2m.out.writeBuf);

            audioIn.flush();

            hipass.process(rd, configPanel->s2m.out.writeBuf, hipass.out.writeBuf);
            configPanel->r2c.process(rd, hipass.out.writeBuf, configPanel->r2c.out.writeBuf);
            if (configPanel->doEqualize) {
                configPanel->equalizer.process(rd, configPanel->r2c.out.writeBuf, configPanel->equalizer.out.writeBuf);
                auto micAmp = 5;       // this is the estimate power loss after the built-in equalizer
                for (int i = 0; i < rd; i++) {
                    configPanel->equalizer.out.writeBuf[i] *= (float)micAmp;
                }
            } else {
                memcpy(configPanel->equalizer.out.writeBuf, configPanel->r2c.out.writeBuf, rd * sizeof(dsp::complex_t));
            }
            configPanel->c2r.process(rd, configPanel->equalizer.out.writeBuf, configPanel->c2r.out.writeBuf);
            configPanel->agc.process(rd, configPanel->c2r.out.writeBuf, configPanel->agc.out.writeBuf);
            configPanel->m2s.process(rd, configPanel->agc.out.writeBuf, configPanel->m2s.out.writeBuf);
            configPanel->afnr->process(configPanel->m2s.out.writeBuf, rd, configPanel->afnr->out.writeBuf, rd);
            lopass.process(rd, configPanel->afnr->out.writeBuf, audioInProcessedIn.writeBuf);
            if (rd > 0) {
                configPanel->outDecibels.addSamples(audioInProcessedIn.writeBuf, rd);
            }
            if (rd != 0) {
                count += rd;
                int64_t since = currentTimeMillis() - start;
                if (since == 0) {
                    since = 1;
                }
                //                flog::info("audioInProcessedIn.swap({}), samples/sec {}, count {} since {}", rd, count * 1000/since, count, since);
                if (!audioInProcessedIn.swap(rd)) {
                    break;
                }
            }
        }
    });
    inputProcessor.detach();


    audioInToFFT = std::make_shared<AudioFFTForDisplay>(&audioInProcessed);
    audioInToFFT->start();
    sigpath::averageTxSignalLevel.bindHandler(&maxSignalHandler);
    maxSignalHandler.ctx = this;
    maxSignalHandler.handler = [](float _maxSignal, void* ctx) {
        auto _this = (QSOPanel*)ctx;
        _this->maxSignal.store(_maxSignal);
    };
    audioInProcessed.start();
    flog::info("audioInProcessed.start()");
    std::thread x([this] {
        while (true) {
            auto rd = audioInToFFT->out.read();
            if (rd < 0) {
                flog::info("audioInToFFT->out.read() causes loop break, rd={0}", rd);
                break;
            }
            //            flog::info("audioInToFFT->out.read() exited, data={0} {1} {2} {3} {4} {5} {6} {7}",
            //                         audioInToFFT->out.readBuf[0].re,
            //                         audioInToFFT->out.readBuf[1].re,
            //                         audioInToFFT->out.readBuf[2].re,
            //                         audioInToFFT->out.readBuf[3].re,
            //                         audioInToFFT->out.readBuf[0].im,
            //                         audioInToFFT->out.readBuf[1].im,
            //                         audioInToFFT->out.readBuf[2].im,
            //                         audioInToFFT->out.readBuf[3].im
            //                         );
            auto frame = std::make_shared<std::vector<dsp::complex_t>>(audioInToFFT->out.readBuf, audioInToFFT->out.readBuf + rd);
            auto floatArray = npabsolute(frame);
            currentFFTBufferMutex.lock();
            currentFFTBuffer = floatArray;
            currentFFTBufferMutex.unlock();
            audioInToFFT->out.flush();
        }
    });
    x.detach();
}


void QSOPanel::setModeSubmode(const std::string& _mode, const std::string& _submode) {
    if (submode != _submode) {
        submode = _submode;
    }
    if (mode != _mode) {
        mode = _mode;
    }
}

void QSOPanel::handleTxButton(bool tx, bool tune) {
    if (tx) {
        if (!this->transmitting) {
            sigpath::txState.emit(tx);
            this->transmitting = true;
            if (sigpath::transmitter) {
                audioInProcessed.bindStream(&audioTowardsTransmitter);
                audioInToTransmitter = std::make_shared<AudioInToTransmitter>(&audioTowardsTransmitter);
                audioInToTransmitter->setSubmode(submode);
                audioInToTransmitter->setMicGain(micGain);
                audioInToTransmitter->postprocess = postprocess;
                audioInToTransmitter->tuneFrequency = tune ? 20 : 0; // 20hz close to carrier
                audioInToTransmitter->start();
                sigpath::transmitter->setTransmitStream(&audioInToTransmitter->out);
                sigpath::transmitter->setTransmitFrequency((int)currentFreq);
                sigpath::transmitter->setTransmitStatus(true);
            }
            lastSWR.clear();
            lastForward.clear();
            lastReflected.clear();
        }
    }
    else {
        if (this->transmitting) {
            this->transmitting = false;
            triggerTXOffEvent = 14; // 60fps,
            if (sigpath::transmitter) {
                sigpath::transmitter->setTransmitStatus(false);
                audioInProcessed.unbindStream(&audioTowardsTransmitter);
                audioInToTransmitter->stop();
                audioInToTransmitter->out.stopReader();
            }
        }
    }
}

void QSOPanel::stopSoundPipeline() {
    if (audioInToFFT) {
        sigpath::sinkManager.defaultInputAudio.unbindStream(&audioIn);
        flog::info("QSOPanel::stop. Calling audioInToFFT->stop()");
        audioInToFFT->stop();
        audioInToFFT->out.stopReader();
        audioInProcessed.stop();
        flog::info("Calling audioInToFFT->reset()");
        audioInToFFT.reset();
        audioIn.stopReader();
        flog::info("Reset complete");
        sigpath::averageTxSignalLevel.unbindHandler(&maxSignalHandler);
    }
}

float rtmax(std::vector<float>& v) {
    if (v.empty()) {
        return 0;
    }
    while (v.size() > 20) {
        v.erase(v.begin());
    }
    auto m = v[0];
    for (int q = 1; q < v.size(); q++) {
        if (v[q] > m) {
            m = v[q];
        }
    }
    return m;
}

void draw_db_gauge(float gauge_width, float level, float peak, float min_level = -140.0f, float max_level = 20.0f, float red_zone = 0.0f) {
    //
    // NB this has been produced by chatgpt 4.
    //
    float gauge_height = style::baseFont->FontSize * 1.5f;


    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 gauge_min = ImVec2(pos.x, pos.y);
    ImVec2 gauge_max = ImVec2(pos.x + gauge_width, pos.y + gauge_height);

    // Draw the background
    ImGui::GetWindowDrawList()->AddRectFilled(gauge_min, gauge_max, ImGui::GetColorU32(ImGuiCol_FrameBg));

    // Draw the ticks and text
    for (int i = static_cast<int>(min_level); i <= static_cast<int>(max_level); ++i) {
        if (i % 10 == 0) {
            float x = (i - min_level) / (max_level - min_level) * gauge_width + pos.x;
            ImVec2 tick_start = ImVec2(x, pos.y);
            ImVec2 tick_end = ImVec2(x, pos.y + gauge_height * 0.3f);
            ImGui::GetWindowDrawList()->AddLine(tick_start, tick_end, ImGui::GetColorU32(ImGuiCol_Text), 1.0f);

            char text_buffer[16];
            snprintf(text_buffer, sizeof(text_buffer), "%d", i);
            ImGui::GetWindowDrawList()->AddText(ImVec2(x - 5.0f, pos.y + gauge_height * 0.35f), ImGui::GetColorU32(ImGuiCol_Text), text_buffer);
        }
    }

    // Draw the red zone
    ImVec2 red_zone_min = ImVec2((red_zone - min_level) / (max_level - min_level) * gauge_width + pos.x, pos.y);
    ImVec2 red_zone_max = ImVec2(gauge_max.x, gauge_max.y);
    ImGui::GetWindowDrawList()->AddRectFilled(red_zone_min, red_zone_max, IM_COL32(255, 0, 0, 128));

    // Draw the solid gray bar
    ImVec2 level_max = ImVec2((level - min_level) / (max_level - min_level) * gauge_width + pos.x, gauge_max.y);
    ImGui::GetWindowDrawList()->AddRectFilled(gauge_min, level_max, IM_COL32(160, 160, 160, 128));

    // Draw the peak level
    ImVec2 peak_pos = ImVec2((peak - min_level) / (max_level - min_level) * gauge_width + pos.x, pos.y);
    ImGui::GetWindowDrawList()->AddLine(peak_pos, ImVec2(peak_pos.x, pos.y + gauge_height), IM_COL32(255, 255, 255, 255), 2.0f);

    // Increment the cursor position
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + gauge_height));
}


void QSOPanel::drawHistogram() {
    if (!currentFFTBuffer || audioInToFFT->receivedSamplesCount.load() == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0, 0, 1.0f), "%s", "No microphone! Please");
        ImGui::TextColored(ImVec4(1.0f, 0, 0, 1.0f), "%s", "allow mic access in");
        ImGui::TextColored(ImVec4(1.0f, 0, 0, 1.0f), "%s", "the app permissions!");
        ImGui::TextColored(ImVec4(1.0f, 0, 0, 1.0f), "%s", "(and then restart)");
    }
    else {
        if (submode == "LSB" || submode == "USB") {
            auto mx0 = npmax(currentFFTBuffer);
            auto data = npsqrt(currentFFTBuffer);
            auto mx = npmax(data);
            // only 1/4 of spectrum
            ImVec2 space = ImGui::GetContentRegionAvail();
            space.y = 60; // fft height
            ImGui::PlotHistogram("##fft", data->data(), currentFFTBuffer->size() / 4, 0, NULL, 0,
                                 mx,
                                 space);
//            if (ImGui::SliderFloat("##_radio_mic_gain_", &this->micGain, 0, +22, "%.1f dB")) {
//                //
//            }
//            ImGui::SameLine();
            float db = configPanel->rawInDecibels.getMax(10);
            draw_db_gauge(ImGui::GetContentRegionAvail().x, db, 0, -60, +20, 0);
            ImGui::Text("Mic:%.3f", configPanel->rawInDecibels.getAvg(1));
        }
        if (submode == "CWU" || submode == "CWL" || submode == "CW") {
            ImGui::Text("CW Mode - use paddle");
        }
    }
}

void QSOPanel::draw(float _currentFreq, ImGui::WaterfallVFO*) {
    this->currentFreq = _currentFreq;
    currentFFTBufferMutex.lock();
    this->drawHistogram();
    float mx = 10 * log10(this->maxSignal.load());
    if (mx > maxTxSignalPeak || maxTxSignalPeakTime < currentTimeMillis() - 500) {
        maxTxSignalPeak = mx;
        maxTxSignalPeakTime = currentTimeMillis();;
    }

    draw_db_gauge(ImGui::GetContentRegionAvail().x, mx, maxTxSignalPeak, -60, +20, 0);
//    ImGui::Text("Max TX: %f", maxTxSignalPeak);
    if (sigpath::transmitter) {
        if (ImGui::Checkbox("PostPro(SSB)", &this->postprocess)) {
        }
        ImGui::Text("TRX Qsz: %d", (int)sigpath::transmitter->getFillLevel());
        float swr = sigpath::transmitter->getTransmitSWR();
        if (swr >= 9.9) swr = 9.9; // just not to jump much
        lastSWR.emplace_back(swr);
        lastForward.emplace_back(sigpath::transmitter->getTransmitPower());
        lastReflected.emplace_back(sigpath::transmitter->getReflectedPower());
        ImGui::Text("SWR:%.1f", rtmax(lastSWR));
        ImGui::SameLine();
        ImGui::Text("FWD:%.1f", rtmax(lastForward)); // below 10w will - not jump.
        ImGui::SameLine();
        ImGui::Text("REF:%.1f", rtmax(lastReflected));
        if (ImGui::Checkbox("PA", &this->enablePA)) {
            sigpath::transmitter->setPAEnabled(this->enablePA);
        }
        ImGui::SameLine();
        ImGui::LeftLabel("TX:");
        ImGui::SameLine();
        if (ImGui::SliderInt("##_radio_tx_gain_", &this->txGain, 0, 255)) {
            sigpath::transmitter->setTransmitGain(this->txGain);
        }
    }
    else {
        ImGui::TextColored(ImVec4(1.0f, 0, 0, 1.0f), "%s", "Transmitter not found");
        ImGui::TextColored(ImVec4(1.0f, 0, 0, 1.0f), "%s", "Select a device");
        ImGui::TextColored(ImVec4(1.0f, 0, 0, 1.0f), "%s", "(e.g. HL2 Source)");
    }
    currentFFTBufferMutex.unlock();

    triggerTXOffEvent--;
    if (!triggerTXOffEvent) { // zero cross
        audioInToTransmitter.reset();
        sigpath::txState.emit(false);
    }
}
QSOPanel::~QSOPanel() {
    stopSoundPipeline();
}

void ConfigPanel::draw() {
    gui::mainWindow.qsoPanel->currentFFTBufferMutex.lock();
    gui::mainWindow.qsoPanel->drawHistogram();
    gui::mainWindow.qsoPanel->currentFFTBufferMutex.unlock();

    ImGui::LeftLabel("Audio lo cutoff frequency");
    if (ImGui::SliderInt("##lowcutoff", &highPass, 0, lowPass, "%d Hz")) {
    }
    ImGui::LeftLabel("Audio hi cutoff frequency");
    if (ImGui::SliderInt("##highcutoff", &lowPass, highPass, 3200, "%d Hz")) {
    }

    ImGui::LeftLabel("AGC Attack");
    if (ImGui::SliderFloat("rec##attach", &agcAttack, 1.0f, 200.0f)) {
        agc.setAttack(agcAttack);
//        agc2.setAttack(agcAttack);
    }
    ImGui::LeftLabel("AGC Decay");
    if (ImGui::SliderFloat("rec##decay", &agcDecay, 1.0f, 20.0f)) {
        agc.setDecay(agcDecay);
//        agc2.setDecay(agcDecay);
    }
    if (afnr) {
        ImGui::LeftLabel("Mic Noise Reduction");
        if (ImGui::Checkbox("##donr", &afnr->allowed)) {
        }
    }
//    ImGui::SameLine();
//    if (ImGui::Button("Reset NR")) {
//        afnr.reset();
//    }
    ImGui::LeftLabel("DX Equalizer / Compressor");
    if (ImGui::Checkbox("##equalizeit", &doEqualize)) {
    }

    ImVec2 space = ImGui::GetContentRegionAvail();
    draw_db_gauge(space.x, outDecibels.getMax(3), outDecibels.getPeak(), -80, +20);

    ImGui::SetCursorPos(ImGui::GetCursorPos()+ImVec2(0, style::baseFont->FontSize*0.4));

    switch (recorder.mode) {
    case SimpleRecorder::RECORDING_IDLE:
        if (doFingerButton("Record")) {
            recorder.startRecording();
        }
        if (!recorder.data.empty()) {
            ImGui::SameLine();
            if (doFingerButton("Play")) {
                recorder.startPlaying();
            }
        }
        break;
    case SimpleRecorder::RECORDING_STARTED:
        if (doFingerButton("Stop Rec")) {
            recorder.stop();
        }
        break;
    case SimpleRecorder::PLAYING_STARTED:
        ImGui::BeginDisabled(true);
        if (doFingerButton("Record")) {
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (doFingerButton("Stop Play")) {
            recorder.stop();
        }
        break;
    }
}
