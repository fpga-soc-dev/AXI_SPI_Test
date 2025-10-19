FILESEXTRAPATHS:prepend := "${THISDIR}/files:"


SRC_URI:append = " \
    file://wpa_supplicant.conf \
    file://wifi-usb-reset.service \
    file://20-wireless.network \
    file://wifi-reset.sh \
"

inherit systemd

do_install:append() {
    install -m 600 ${WORKDIR}/wpa_supplicant.conf ${D}${sysconfdir}/wpa_supplicant.conf
	
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/20-wireless.network ${D}${sysconfdir}/systemd/network
    
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/wifi-reset.sh ${D}${bindir}/
    
   if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install -d ${D}${systemd_system_unitdir}
        install -m 0644 ${WORKDIR}/wifi-usb-reset.service ${D}${systemd_system_unitdir}       
   fi

}

SYSTEMD_SERVICE:${PN} = "wifi-usb-reset.service"
SYSTEMD_AUTO_ENABLE = "enable"

FILES:${PN} += " \
    ${bindir}/wifi-reset.sh \
"




