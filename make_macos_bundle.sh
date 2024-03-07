#!/bin/sh
set -e

# ========================= Boilerplate =========================
BUILD_DIR=$1
BUNDLE=$2

source macos/bundle_utils.sh

# ========================= Prepare dotapp structure =========================

# Clear .app
rm -rf $BUNDLE

# Create .app structure
bundle_create_struct $BUNDLE

# Add resources
cp -R root/res/* $BUNDLE/Contents/Resources/

# Create the icon file
bundle_create_icns root/res/icons/sdrpp.macos.png $BUNDLE/Contents/Resources/sdrpp

# Create the property list
bundle_create_plist sdrpp SDR++ org.sdrpp.sdrpp 1.1.0 sdrp sdrpp sdrpp $BUNDLE/Contents/Info.plist

# ========================= Install binaries =========================

# Core
bundle_install_binary $BUNDLE $BUNDLE/Contents/MacOS $BUILD_DIR/sdrpp 
bundle_install_binary $BUNDLE $BUNDLE/Contents/Frameworks $BUILD_DIR/core/libsdrpp_core.dylib

# I gave up.
install_name_tool -change @rpath/Accelerate /System/Library/Frameworks/Accelerate.framework/Versions/A/Accelerate $BUILD_DIR/sdrpp

# Source modules
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/airspy_source/airspy_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/airspyhf_source/airspyhf_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/bladerf_source/bladerf_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/file_source/file_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/hackrf_source/hackrf_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/hermes_source/hermes_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/limesdr_source/limesdr_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/perseus_source/perseus_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/plutosdr_source/plutosdr_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/rfspace_source/rfspace_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/rtl_sdr_source/rtl_sdr_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/rtl_tcp_source/rtl_tcp_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/sdrplay_source/sdrplay_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/sdrpp_server_source/sdrpp_server_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/soapy_source/soapy_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/spyserver_source/spyserver_source.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/hl2_source/hl2_source.dylib
# bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/source_modules/usrp_source/usrp_source.dylib

# Sink modules
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/sink_modules/portaudio_sink/audio_sink.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/sink_modules/network_sink/network_sink.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/sink_modules/new_portaudio_sink/new_portaudio_sink.dylib

# Decoder modules
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/decoder_modules/m17_decoder/m17_decoder.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/decoder_modules/meteor_demodulator/meteor_demodulator.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/decoder_modules/radio/radio.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/decoder_modules/ft8_decoder/ft8_decoder.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/MacOS $BUILD_DIR/decoder_modules/ft8_decoder/sdrpp_ft8_mshv
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/decoder_modules/ft8_decoder/sdrpp_ft8_mshv

# Misc modules
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/discord_integration/discord_integration.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/frequency_manager/frequency_manager.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/recorder/recorder.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/rigctl_client/rigctl_client.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/rigctl_server/rigctl_server.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/scanner/scanner.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/noise_reduction_logmmse/noise_reduction_logmmse.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/reports_monitor/reports_monitor.dylib
bundle_install_binary $BUNDLE $BUNDLE/Contents/Plugins $BUILD_DIR/misc_modules/websdr_view/websdr_view.dylib

# ========================= Finalize =========================

# Sign the app
bundle_sign $BUNDLE