#pragma once
#include <imgui/imgui.h>
#include <fftw3.h>
#include <dsp/types.h>
#include <dsp/stream.h>
#include <signal_path/vfo_manager.h>
#include <string>
#include <utils/event.h>
#include <mutex>
#include <gui/tuner.h>

#define WINDOW_FLAGS ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse

class MainWindow {
public:
    virtual void init();
    virtual void end() {}
    virtual void preDraw(ImGui::WaterfallVFO* *vfo);
    virtual void draw();
    void drawUpperLine(ImGui::WaterfallVFO* vfo);
    void updateZoom() {
        this->updateWaterfallZoomBandwidth(bw);
    }
    bool sdrIsRunning();

    static float* acquireFFTBuffer(void* ctx);
    static void releaseFFTBuffer(void* ctx);

    // TODO: Replace with it's own class
    void setVFO(double freq);

    void setPlayState(bool _playing);
    bool isPlaying();

    bool lockWaterfallControls = false;
    bool playButtonLocked = false;

    Event<bool> onPlayStateChange;
    Event<ImGuiContext *> onWaterfallDrawn;
    Event<ImGuiContext *> onDebugDraw;

    void setFirstMenuRender() {
        firstMenuRender = true;
    }

    void addBottomWindow(std::string name, std::function<void()> drawFunc);
    void removeBottomWindow(std::string name);
    void updateBottomWindowLayout();
    void drawBottomWindows(int dy);
    bool hasBottomWindow(std::string name);

    bool logWindow = false;
    bool showMenu = true;

protected:
    static void vfoAddedHandler(VFOManager::VFO* vfo, void* ctx);

    // FFT Variables
    int fftSize = 8192 * 8;
    std::mutex fft_mtx;
    fftwf_complex *fft_in, *fft_out;
    fftwf_plan fftwPlan;

    // GUI Variables
    bool firstMenuRender = true;
    bool startedWithMenuClosed = false;
    float fftMin = -70.0;
    float fftMax = 0.0;
    float bw = 1.0;             // slider position 0.0 .. 1.0
    bool playing = false;
    bool showCredits = false;
    std::string audioStreamName = "";
    std::string sourceName = "";
    int menuWidth = 300;
    bool grabbingMenu = false;
    int newWidth = 300;
    int fftHeight = 300;
    int tuningMode = tuner::TUNER_MODE_NORMAL;

    dsp::stream<dsp::complex_t> dummyStream;
    bool demoWindow = false;
    int selectedWindow = 0;

    bool initComplete = false;
    bool autostart = false;

    EventHandler<VFOManager::VFO*> vfoCreatedHandler;

    void updateWaterfallZoomBandwidth(float bw);
    void handleWaterfallInput(ImGui::WaterfallVFO* vfo);

    struct ButtomWindow {
        std::string name;
        std::function<void()> drawFunc;
        ImVec2 loc;
        ImVec2 size;
    };

    std::vector<ButtomWindow> bottomWindows;

    void drawDebugMenu();

    void ShowLogWindow();
};