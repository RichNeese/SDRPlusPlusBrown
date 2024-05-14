#pragma once
#include <dsp/block.h>
#include <dsp/stream.h>
#include <dsp/types.h>
#include <dsp/processor.h>
#include "utils/arrays.h"
#include "logmmse.h"

namespace dsp {

    using namespace ::dsp::arrays;
    using namespace ::dsp::logmmse;

    class IFNRLogMMSE : public Processor<complex_t, complex_t> {
        using base_type = Processor<complex_t, complex_t>;
    public:
        IFNRLogMMSE() {
            params.forceWideband = true;
        }

        void init(stream<complex_t>* in) override {
            base_type::init(in);
        }

        void setInput(stream<complex_t>* in) override {
            if (!_block_init) {
                init(in);
            } else {
                Processor::setInput(in);
            }
        }

    public:

        ComplexArray worker1c;
        std::mutex workerMutex;
        int freq = 192000;
        LogMMSE::SavedParamsC params;
        std::mutex freqMutex;

        void doStart() override {
            base_type::doStart();
            shouldReset = true;
        }

        void setHold(bool hold) {
            params.hold = hold;
        }

        double currentCenterFrequency = -1.0;

        bool shouldReset = true;
        void reset() {
            shouldReset = true;
        }

        void process(complex_t *in, int count, complex_t *out, int &outCount) {
            if (shouldReset) {
                flog::info("Resetting IF NR LogMMSE");
                shouldReset = false;
                worker1c.reset();
                if (params.forceSampleRate != 0) {
                    freq = params.forceSampleRate;
                } else {
                    freq = (int)sigpath::iqFrontEnd.getSampleRate();
                }
            }
            if (!worker1c) {
                worker1c = npzeros_c(0);
                params.reset();
            }
            for (int i = 0; i < count; i++) {
                worker1c->emplace_back(in[i]);
            }
            int noiseFrames = 12;
            int fram = freq / 100;
            int initialDemand = fram * 2;
            if (!params.Xk_prev) {
                initialDemand = fram * (noiseFrames + 2) * 2;
            }
            if (worker1c->size() < initialDemand) {
                outCount = 0;
                return;
            }
            int retCount = 0;
            freqMutex.lock();
            if (!params.Xk_prev) {
                std::cout << std::endl << "Sampling initially" << std::endl;
                LogMMSE::logmmse_sample(worker1c, freq, 0.15f, &params, noiseFrames);
            }
            auto rv = LogMMSE::logmmse_all(worker1c, freq, 0.15f, &params);
            freqMutex.unlock();

            int limit = rv->size();
            auto dta = rv->data();
            for (int i = 0; i < limit; i++) {
                auto lp = dta[i];
                out[i] = lp * 4.0;
            }
            memmove(worker1c->data(), ((complex_t *) worker1c->data()) + rv->size(), sizeof(complex_t) * (worker1c->size() - rv->size()));
            worker1c->resize(worker1c->size() - rv->size());
            retCount += rv->size();
            outCount = limit;
            return;
        }

        long long lastReport = currentTimeMillis();
        long long cpuUsed = 0;
        int percentUsage = 0;

        int runMMSE(stream <complex_t> *_in, stream <complex_t> &out) {
            int count = _in->read();
            if (count < 0) { return -1; }
            int outCount;
            long long ctm0 = currentTimeMillis();
            process(_in->readBuf, count, out.writeBuf, outCount);
            _in->flush();
            if (!out.swap(outCount)) {
                return -1;
            }
            long long ctm = currentTimeMillis();
            cpuUsed += ctm - ctm0;
            if (lastReport / 1000 != ctm / 1000) {
                auto timeSinceLastReport = ctm - lastReport;
                auto usedSinceLastReport = cpuUsed;
                cpuUsed = 0;
                lastReport = ctm;
                percentUsage = (usedSinceLastReport * 100) / timeSinceLastReport;
            }
            return 1;
        }

        int run() override {
            int count = _in->read();
            if (count < 0) {
                return -1;
            }

//            if (bypass) {
//                memcpy(out.writeBuf, _in->readBuf, count * sizeof(complex_t));
//                _in->flush();
//                if (!out.swap(count)) { return -1; }
//                return count;
//            }
//
            runMMSE(_in, out);
            return count;
        }

        void start() override {
            txHandler.ctx = this;
            txHandler.handler = [](bool txActive, void *ctx) {
                auto _this = (IFNRLogMMSE*)ctx;
                _this->params.hold = txActive;
            };
            sigpath::txState.bindHandler(&txHandler);
            block::start();
        }
        void stop() override {
            percentUsage = -1;
            block::stop();
            sigpath::txState.unbindHandler(&txHandler);
        }

        bool bypass = true;
        EventHandler<bool> txHandler;


    };



}