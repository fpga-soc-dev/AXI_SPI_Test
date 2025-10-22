SUMMARY = "Image for pynq z1 board"

IMAGE_LINGUAS = " "

LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL = "packagegroup-core-boot"
IMAGE_INSTALL:append = " ${CORE_IMAGE_EXTRA_INSTALL}"
IMAGE_INSTALL:append = " kernel-modules"
IMAGE_INSTALL:append = " openssh openssh-sshd openssh-sftp-server"

IMAGE_INSTALL:append = " libgpiod libgpiod-dev libgpiod-tools"
IMAGE_INSTALL:append = " linux-firmware-ralink usbutils networkmanager"

IMAGE_INSTALL:append = " oled-spi-test"

inherit extrausers
EXTRA_USERS_PARAMS = "usermod -d /home/root root;"
