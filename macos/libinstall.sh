#!/bin/sh

#####################################################
#
# prepeare your MacOS for compiling sdr++ sdr++brown
#
######################################################

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
SOAPYAISPY=N
SOAPYAISPYHF=N
SOAPYHACKRF=N
SOAPYLIMESUITE=N
SOAPYREDPITAYA=N
SOAPYPLUTOSDR=N
SOAPYRTLSDR=N 

################################################################
#
# a) MacOS does not have "realpath" so we need to fiddle around
#
################################################################

THISDIR="$(cd "$(dirname "$0")" && pwd -P)"

################################################################
#
# b) Initialize HomeBrew and required packages
#    (this does no harm if HomeBrew is already installed)
#
################################################################
  
#
# This installs the "command line tools", these are necessary to install the
# homebrew universe
#
xcode-select --install

#
# This installes the core of the homebrew universe
#
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"

#
# At this point, there is a "brew" command either in /usr/local/bin (Intel Mac) or in
# /opt/homebrew/bin (Silicon Mac). Look what applies
#
BREW=junk

if [ -x /usr/local/bin/brew ]; then
  BREW=/usr/local/bin/brew
fi

if [ -x /opt/homebrew/bin/brew ]; then
  BREW=/opt/homebrew/bin/brew
fi

if [ $BREW == "junk" ]; then
  echo HomeBrew installation obviously failed, exiting
  exit
fi

################################################################
#
# This adjusts the PATH. This is not bullet-proof, so if some-
# thing goes wrong here, the user will later not find the
# 'brew' command.
#
################################################################

if [ $SHELL == "/bin/sh" ]; then
$BREW shellenv sh >> $HOME/.profile
fi
if [ $SHELL == "/bin/csh" ]; then
$BREW shellenv csh >> $HOME/.cshrc
fi
if [ $SHELL == "/bin/zsh" ]; then
$BREW shellenv zsh >> $HOME/.zprofile
fi

################################################################
#
# create links in /usr/local if necessary (only if
# HomeBrew is installed in /opt/local)
#
# Should be done HERE if some of the following packages
# have to be compiled from the sources
#
# Note existing DIRECTORIES in /usr/local will not be deleted,
# the "rm" commands only remove symbolic links should they
# already exist.
################################################################

if [ ! -d /usr/local/lib ]; then
  echo "/usr/local/lib does not exist, creating symbolic link ..."
  sudo rm -f /usr/local/lib
  sudo ln -s /opt/local/lib /usr/local/lib
fi
if [ ! -d /usr/local/bin ]; then
  echo "/usr/local/bin does not exist, creating symbolic link ..."
  sudo rm -f /usr/local/bin
  sudo ln -s /opt/local/bin /usr/local/bin
fi
if [ ! -d /usr/local/include ]; then
  echo "/usr/local/include does not exist, creating symbolic link ..."
  sudo rm -f /usr/local/include
  sudo ln -s /opt/local/include /usr/local/include
fi

################################################################
#
# All homebrew packages needed for sdr++ sdr++brown
#
################################################################
$BREW install gtk+3
$BREW install librsvg
$BREW install pkg-config
$BREW install portaudio
$BREW install fftw
$BREW install libusb
$BREW install cmake
$BREW install glfw
$BREW install codec2
$BREW install libiio
$BREW install libad9361
$BREW install python-mako
$BREW install volk
$BREW install zstd


#########################################################
# Lib for bladerf
#########################################################
$BREW install libbladerf

#########################################################
# Lib for RTL_SDR
#########################################################
if [ SOAPYRTLSDR == N  ]; then
	$BREW install rtl-sdr
fi
#########################################################
# Lib for airspy / airspyhf
# Disable if you want to us soapysdr drivers
#########################################################
if [ SOAPYAISPY == N ]; then
	$BREW install airspy
fi

if [ SOAPYAISPYHF == N ]; then
	$BREW install airspyhf
fi

#########################################################
# Lib for hackers
# Disable if you want to us soapysdr drivers
#########################################################
if [ SOAPYHACKRF == N ]; then
	$BREW install hackrf
fi

#########################################################
# Limes suite
#########################################################
if [ SOAPYLIMESUITE == N ]; then
	$BREW install limesuite
fi

#############################################################
#
# This is for the SoapySDR universe
# There are even more radios supported for which you need
# additional modules, for a list, goto the web page
# https://formulae.brew.sh
# and insert the search string "pothosware". In the long
# list produced, search for the same string using the
# "search" facility of your internet browser
#
################################################################
if [ SOAPYSDR == Y ]; then

	$BREW install soapysdr

	if [ SOAPYAIRSPY == Y]; then
		$BREW install pothosware/pothos/soapyairspy
	fi

	if [ SOAPYAISPYHF == Y ]; then
		$BREW install pothosware/pothos/soapyairspyhf
	fi

	if [ SOAPYHACKRF == Y ]; then
		$BREW install pothosware/pothos/soapyhackrf
	fi

	if [ SOAPYLIMESUITE == Y ]; then
		$BREW install pothosware/pothos/limesuite
	fi

	if [ SOAPYREDPITAYA == Y  ]; then	
		$BREW install pothosware/pothos/soapyredpitaya
	fi

	if [ SOAPYRTLSDR == Y  ]; then
		$BREW install pothosware/pothos/soapyrtlsdr
	fi

	if [ SOAPYPLUTOSDR == Y  ]; then
		$BREW install pothosware/pothos/soapyplutosdr
	fi
fi

################################################################
#
# This is for PrivacyProtection
#
################################################################
$BREW analytics off

