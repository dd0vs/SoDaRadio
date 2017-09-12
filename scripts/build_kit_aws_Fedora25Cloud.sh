#!/bin/bash

# This script is used to build the RPM on an
# AWS Fedora 25 Cloud instance. 

# needs to be run as root

 dnf group install --assumeyes "C Development Tools and Libraries"
 dnf install --assumeyes libusb-devel doxygen boost-devel
 dnf install --assumeyes fftw-devel alsa-lib-devel
 dnf install --assumeyes qt5-qtbase-devel
 dnf install --assumeyes qwt-qt5-devel
 dnf install --assumeyes python-mako
 dnf install --assumeyes qt5-qtmultimedia-devel
 dnf install --assumeyes hamlib-devel
 dnf install --assumeyes gpsd-devel gpsd-clients gpsd-libs gpsd
 dnf install --assumeyes rpm-build
 dnf install --assumeyes uhd-devel uhd-firmware
 dnf install --assumeyes git
 dnf install --assumeyes redhat-lsb

git clone https://git.code.sf.net/p/sodaradio/SoDaRadio/
cd SoDaRadio
git checkout QTgui
mkdir build
cd build
cmake -DBUILD_RPM=1 ../
make package
