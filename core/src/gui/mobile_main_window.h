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
#include "main_window.h"

struct TheEncoder {

    float somePosition = 0; // angle in radians
    float speed = 0;
    float delayFactor = 0.96;

    float lastMouseAngle = nan("");

    std::vector<float> fingerMovement;

    float draw();
};

struct MobileButton {
    std::string upperText;
    std::string buttonText;
    ImVec2 pressPoint;
    MobileButton(std::string upperText, std::string buttonText) {
        this->upperText = upperText;
        this->buttonText = buttonText;
        this->pressPoint = ImVec2(nan(""), nan(""));
    }
    float sizeFactor = 0.8;
    bool draw();
};


class MobileMainWindow : public MainWindow {
public:
    TheEncoder encoder;
    MobileButton bandUp;
    MobileButton bandDown;
    MobileButton zoomToggle;
    MobileButton modeToggle;
    MobileButton submodeToggle;
    MobileButton txButton;
    bool shouldInitialize = true;
    std::vector<std::string> modes = { "SSB", "CW", "DIGI", "FM", "AM" };
    std::map<std::string, std::vector<std::string>> subModes = { { "SSB", { "LSB", "USB" } }, { "FM", { "WFM", "NFM" } }, { "AM", { "AM" } }, { "CW", { "CWU", "CWL" } }, { "DIGI", { "FT8", "FT4", "OLIVIA", "PSK31", "SSTV" } } };
    std::vector<std::string> bands = { "MW", "LW", "160M", "80M", "60M", "40M", "30M", "20M", "18M", "15M", "12M", "10M", "2M" };
    std::map<std::string, float> frequencyDefaults = {
        { "MW", 630 },
        { "LW", 125 },
        { "160M", 1888 },
        { "160M_CW", 1800 },
        { "160M_FT8", 1840 },
        { "160M_PSK31", 1838 },
        { "160M_OLIVIA", 1808.75 },
        { "160M_SSTV", 1890 },
        { "80M", 3650 },
        { "80M_CW", 3600 },
        { "80M_FT8", 3573 },
        { "80M_FT4", 3568 },
        { "80M_PSK31", 3580 },
        { "80M_OLIVIA", 3577.75 },
        { "80M_SSTV", 3730 },
        { "60M", 5357 },
        { "60M_OLIVIA", 5366.5 },
        { "40M", 7100 },
        { "40M_CW", 7000 },
        { "40M_FT8", 7074 },
        { "40M_FT4", 7047 },
        { "40M_PSK31", 7040 },
        { "40M_OLIVIA", 7043.25 },
        { "40M_SSTV", 7033 },
        { "30M", 10100 },
        { "30M_CW", 10100 },
        { "30M_FT8", 10136 },
        { "30M_FT4", 10140 },
        { "30M_PSK31", 10142 },
        { "30M_SSTV", 10132 },
        { "30M_OLIVIA", 10142.25 },
        { "20M", 14200 },
        { "20M_CW", 14000 },
        { "20M_FT8", 14074 },
        { "20M_PSK31", 14070 },
        { "20M_FT4", 14080 },
        { "20M_OLIVIA", 14076.4 },
        { "20M_SSTV", 14230 },
        { "18M", 18120 },
        { "18M_FT8", 18100 },
        { "18M_FT4", 18104 },
        { "18M_CW", 18068 },
        { "18M_PSK31", 18097 },
        { "18M_OLIVIA", 18103.4 },
        { "15M", 21250 },
        { "15M_CW", 21000 },
        { "15M_FT8", 21074 },
        { "15M_PSK31", 21080 },
        { "15M_FT4", 21140 },
        { "15M_OLIVIA", 21087.25 },
        { "15M_SSTV", 21340 },
        { "12M", 24960 },
        { "12M_FT8", 24915 },
        { "12M_FT4", 24919 },
        { "12M_PSK31", 24920 },
        { "12M_CW", 24890 },
        { "12M_OLIVIA", 24922.25 },
        { "12M_SSTV", 24975 },
        { "11M", 27500 },
        { "11M_FM", 27100 },
        { "11M_SSB", 27185 },
        { "10M", 28530 },
        { "10M_CW", 28000 },
        { "10M_FT8", 28074 },
        { "10M_FT4", 28180 },
        { "10M_PSK31", 28120 },
        { "10M_OLIVIA", 28076.75 },
        { "10M_SSTV", 28680 },
        { "2M", 145000 },
        { "2M_FM", 145100 },
        { "2M_SSB", 144100 },
        { "2M_FT8", 144174 },
        { "2M_FT4", 144120 },
        { "2M_PSK31", 144144 },
        { "2M_OLIVIA", 144136.25 },
        { "2M_SSTV", 144550 }
    };

    std::vector<std::pair<std::string, float>> zooms = {
        {"Full", 0.0},
        {"100 KHz", 100000},
        {"50 KHz", 50000},
        {"25 KHz", 30000},
        {"3 KHz", 3000},
    };
    MobileMainWindow() : MainWindow(),
                         bandUp("14 Mhz", "+"),
                         bandDown("", "-"),
                         zoomToggle("custom", "Zoom"),
                         modeToggle("SSB", "Mode"),
                         submodeToggle("LSB", "Submode"),
                         txButton("tx: off", "TX")
    {
    }
    void draw() override;
    std::string getCurrentMode();
    void setCurrentMode(std::string);
    std::string getCurrentBand();
    void setCurrentBand(std::string);
    std::string getCurrentModeAttr(std::string key);
    void setCurrentModeAttr(std::string key, std::string val);
    void updateFrequencyAfterChange();
    void updateSubmodeAfterChange();
};