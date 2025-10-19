LICENSE = "CLOSED"

SRC_URI = "\
    file://CMakeLists.txt \
    file://main.cpp \
    file://OLEDDriver.cpp \
    file://OLEDDriver.hpp \
     file://Font8x8.hpp \
    file://Gpio/Gpio.cpp \
    file://Gpio/Gpio.hpp \
    file://SPIInterface/SPIDevice.cpp \
    file://SPIInterface/SPIDevice.hpp \
    file://Utilities/FileHandle.cpp \
    file://Utilities/FileHandle.hpp \
    "

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
do_rm_work[noexec] = "1"

DEPENDS += "fmt spdlog libgpiod"

inherit cmake pkgconfig

do_install() {

    install -d ${D}/home/root
    install -m 0755 ${B}/oled-spi-test ${D}/home/root/

}


FILES:${PN} += "/home/root/oled-spi-test"
