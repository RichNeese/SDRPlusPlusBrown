#!/bin/sh

#####################################################
#
# prepeare your Linux system for compiling SDR++
#
######################################################
echo "First we need to make sure dependancies are installed."

echo "There may be several packages to install.  This could"

echo "take several minutes to complete."
#####################################################
#
# ENABLE SOAPYSDR DRIVERS
# if you enable all the soapy drivers it will disable the
# default libs/drivers that allow use with out SoapySDR
#
# Defualt for all is N
#
# Must be enable SOAPYSDR=Y to install SoapySDR and modules
#
######################################################

SOAPYSDR=N 

################################################################
#
# All packages needed for SDR++
#
################################################################
echo "Installing Deps for building SDR++ / SDR++Brown"
apt -y install build-essential
apt -y install cmake
apt -y install pkg-config
apt -y install libcodec2-dev
apt -y install libgtk-3-dev
apt -y install portaudio19-dev
apt -y install libfftw3-dev
apt -y install libpulse-dev
apt -y install libsoundio-dev
apt -y install libasound2-dev
apt -y install libusb-dev
apt -y install libglfw3-dev
apt -y install libiio-dev
apt -y install libiio-utils
apt -y install libad9361-dev
apt -y install libvolk2-dev
apt -y install libvulkan-volk-dev
apt -y install libzstd-dev
apt -y install zstd

#########################################################
# Python make
#########################################################
echo "INSTALLING MAKO FOR PYTHON3"
apt -y install python3-mako

#########################################################
# Lib for bladerf
#########################################################
echo "INSTALLING BLADERF"
apt -y install libbladerf-dev
apt -y install bladerf-firmware-fx3
apt -y install bladerf-fpga
apt -y install bladerf-fpga-hostedx115
apt -y install bladerf-fpga-hostedx40
apt -y install bladerf-fpga-hostedxa4
apt -y install bladerf-fpga-hostedxa5
apt -y install bladerf-fpga-hostedxa9

#########################################################
# Lib for RTL_SDR
#########################################################
echo "Installing RTL-SDR"
apt -y install librtlsdr-dev
apt -y install rtl-sdr

#########################################################
# Lib for airspy / airspyhf
# Disable if you want to us soapysdr drivers
#########################################################
if [ SOAPYSDR == N ]; then
	echo "INSTALLING AIRSPY"
	apt -y install airspy
	apt -y install libairspy-dev
fi

if [ SOAPYSDR == N ]; then
	echo "Installing AIRSPYHF"
	apt -y install airspyhf
	apt -y install libairspyhf-dev
fi

#########################################################
# Lib for hackers
# Disable if you want to us soapysdr drivers
#########################################################
echo "Installing HackRF"
apt -y install hackrf
apt -y install hackrf-dev

#########################################################
# Limes suite
#########################################################
echo "Installing LimeSuite"
apt -y install limesuite
apt -y install limesuite-dev

################################################################
#
# This is for the SoapySDR universe
# This installs all the modules and firmware
# 
#
################################################################

if [ SOAPYSDR == Y ]; then
	echo "Installing SoapySDR and Deps"
	apt -y install libsoapysdr-dev
	apt -y install soapysdr-module-all
	apt -y install soapysdr-module-xtrx 
	apt -y install soapysdr-tools
	apt -y install libsoapysdr-doc
	apt -y install uhd-host
	apt -y install uhd-soapysdr

	################################################################
	#
	# INSTALL SDRPLAY API for Linux
	#
	################################################################
	echo "Installing SDRPLAY API FOR Linux And SOAPYSDR"
	################################################################
	# download the API from the SDRplay website
	################################################################
	wget https://www.sdrplay.com/software/SDRplay_RSP_API-Linux-3.14.0.run

	################################################################
	# change permission so the run file is executable
	################################################################
	chmod 755 ./SDRplay_RSP_API-Linux-3.14.0.run

	################################################################
	# execute the API installer (follow the prompts)
	################################################################
	./SDRplay_RSP_API-Linux-3.14.0.run

	################################################################
	#Get the sdrplay soapy module and build it.
	################################################################
	git clone https://github.com/pothosware/SoapySDRPlay3.git
	cd SoapySDRPlay3
	mkdir build
	cd build
	cmake ..
	make
	sudo make install
fi

echo ""
echo "IF any errors please report them so that they can be fixed"
echo ""
echo "If there were no errors reported, please reboot your machine."
echo "Everything should be ready to go."
echo "You can Now build SDR++ / SDR++Brown"
echo ""
