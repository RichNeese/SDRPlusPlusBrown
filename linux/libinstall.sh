#!/bin/sh

#####################################################
#
# prepeare your Linux system for compiling SDR++
#
######################################################
echo "First we need to make sure dependancies are installed."

echo "There may be several packages to install.  This could"

echo "take several minutes to complete."

################################################################
#
# All packages needed for SDR++
#
################################################################
echo "Installing Deps for building SDR++ / SDR++Brown"
sudo apt -y install build-essential
sudo apt -y install cmake
sudo apt -y install pkg-config
sudo apt -y install libcodec2-dev
sudo apt -y install libgtk-3-dev
sudo apt -y install portaudio19-dev
sudo apt -y install libfftw3-dev
sudo apt -y install libpulse-dev
sudo apt -y install libsoundio-dev
sudo apt -y install libasound2-dev
sudo apt -y install libusb-dev
sudo apt -y install libglfw3-dev
sudo apt -y install libiio-dev
sudo apt -y install libiio-utils
sudo apt -y install libad9361-dev
sudo apt -y install libvolk2-dev
sudo apt -y install libvulkan-volk-dev
sudo apt -y install libzstd-dev
sudo apt -y install zstd
echo ""

#########################################################
# Python make
#########################################################
echo "INSTALLING MAKO FOR PYTHON3"
sudo apt -y install python3-mako
echo ""

#########################################################
# Lib for bladerf
#########################################################
echo "INSTALLING BLADERF"
sudo apt -y install bladerf
sudo apt -y install libbladerf-dev
sudo apt -y install bladerf-firmware-fx3
sudo apt -y install bladerf-fpga-hostedx115
sudo apt -y install bladerf-fpga-hostedx40
sudo apt -y install bladerf-fpga-hostedxa4
sudo apt -y install bladerf-fpga-hostedxa5
sudo apt -y install bladerf-fpga-hostedxa9
echo ""

#########################################################
# Lib for RTL_SDR
#########################################################
echo "Installing RTL-SDR"
sudo apt -y install librtlsdr-dev
sudo apt -y install rtl-sdr
echo ""

#########################################################
# Lib for airspy / airspyhf
# Disable if you want to us soapysdr drivers
#########################################################
echo "INSTALLING AIRSPY"
sudo apt -y install airspy
sudo apt -y install libairspy-dev

echo "Installing AIRSPYHF"
sudo apt -y install airspyhf
sudo apt -y install libairspyhf-dev
echo ""

#########################################################
# Lib for hackers
# Disable if you want to us soapysdr drivers
#########################################################
echo "Installing HackRF"
sudo apt -y install hackrf
sudo apt -y install libhackrf-dev
echo ""

#########################################################
# Limes suite
#########################################################
echo "Installing LimeSuite"
sudo apt -y install limesuite
sudo apt -y install liblimesuite-dev
echo ""

################################################################
#
# This is for the SoapySDR universe
# This installs all the modules and firmware
# 
#
################################################################
echo "Installing SoapySDR and Deps"
sudo apt -y install libsoapysdr-dev
sudo apt -y install soapysdr-module-all
sudo apt -y install soapysdr-module-xtrx 
sudo apt -y install soapysapt-cache search bladerf-*dr-tools
sudo apt -y install libsoapysdr-doc
sudo apt -y install uhd-host
sudo apt -y install uhd-soapysdr
echo ""

################################################################
#
# INSTALL SDRPLAY API for Linux
#
################################################################
echo "Installing SDRPLAY API FOR Linux And SOAPYSDR"
wget https://www.sdrplay.com/software/SDRplay_RSP_API-Linux-3.14.0.run
sudo chmod 755 ./SDRplay_RSP_API-Linux-3.14.0.run
sudo ./SDRplay_RSP_API-Linux-3.14.0.run
echo ""

################################################################
# GIt the sdrplay soapy module and build it.
################################################################
echo " building and installing SoapySDRPlay3"
git clone https://github.com/pothosware/SoapySDRPlay3.git
cd SoapySDRPlay3
mkdir build
cd build
cmake ..
make
sudo make install
echo ""

echo ""
echo "IF any errors please report them so that they can be fixed"
echo ""
echo "If there were no errors reported, please reboot your machine."
echo "Everything should be ready to go."
echo "You can Now build SDR++ / SDR++Brown"
echo ""
