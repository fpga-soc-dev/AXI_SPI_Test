# AXI SPI IP Core Example — PYNQ-Z1

A minimal, end-to-end example of using the **Xilinx AXI Quad SPI IP core** on a
PYNQ-Z1 (Zynq-7020) board to drive a **Waveshare 1.3" OLED (SH1106)** over SPI
from Linux.

The project covers the full stack:

1. **Hardware (Vivado)** — block design with `axi_quad_spi` + `axi_gpio`
   connected to the PS.
2. **Linux (Yocto)** — a Scarthgap image whose device tree is generated from the
   exported `.xsa`, exposing the core as `/dev/spidev1.0` and the OLED control
   pins as GPIO.
3. **Application (C++20)** — a userspace driver that uses the standard Linux
   `spidev` + `libgpiod` interfaces to render a clock on the OLED.

---

## Overview

| Item         | Value                                               |
|--------------|-----------------------------------------------------|
| Board        | PYNQ-Z1 (Zynq-7020, `xc7z020clg400-1`)              |
| Display      | Waveshare 1.3" OLED, **SH1106** controller (128×64) |
| SPI IP core  | `xilinx.com:ip:axi_quad_spi` @ **0x41E0_0000** (master) |
| GPIO IP core | `xilinx.com:ip:axi_gpio` @ **0x4120_0000** (2-bit: RESET, DC) |
| SPI pins     | `MOSI` → **W14**, `SCK` → **Y14**, `CS` → **T11** (Pmod B) |
| GPIO pins    | `RESET` → **V16**, `DC` → **W16** (Pmod B)          |
| Linux devices| `/dev/spidev1.0`, `/dev/gpiochip0`                  |
| Toolchain    | Vivado **2024.2**, Yocto **Scarthgap** (kas)        |

## Setup environment

- Vivado 2024.2
- Yocto Scarthgap + [kas](https://kas.readthedocs.io/)
- [Waveshare 1.3" OLED (A)](https://www.waveshare.com/wiki/1.3inch_OLED_(A)) (SH1106)
- PYNQ-Z1 board
- Ubuntu 24.04 (or a similar Linux host)

## Hardware design

### Block design (`Vivado/scripts/pynq_z1_bd.tcl`)

The block design contains:

- `processing_system7` — the Zynq ARM Cortex-A9 PS.
- `axi_quad_spi` — the AXI SPI controller:
  - Master mode, TX/RX FIFO enabled (`FIFO_INCLUDED = 1`).
  - Base address `0x41E0_0000`.
  - Interrupt `ip2intc_irpt` wired back to the PS.
- `axi_gpio` — 2-bit output used as the OLED `RESET` and `DC` (Data/Command)
  control signals. Base address `0x4120_0000`.
- `proc_sys_reset`, `xlconcat`, `xlslice`, `smartconnect` — reset / interrupt /
  AXI glue.

### Top level & pins

`Vivado/src/pynq_z1_top.vhd` maps the block-design ports to board pins. This
design is **write-only** (the OLED has no MISO), so only `SCK`, `MOSI` and `CS`
are exported, together with the two GPIO lines for `RESET`/`DC`.

`Vivado/src/pynq_z1.xdc`:

```tcl
set_property -dict { PACKAGE_PIN W14   IOSTANDARD LVCMOS33 } [get_ports { SPI_MOSI}];  # PMODEB pin 1
set_property -dict { PACKAGE_PIN Y14   IOSTANDARD LVCMOS33 } [get_ports { SPI_CLK}];   # PMODEB pin 2
set_property -dict { PACKAGE_PIN T11   IOSTANDARD LVCMOS33 } [get_ports { SPI_CS}];    # PMODEB pin 3
set_property -dict { PACKAGE_PIN V16   IOSTANDARD LVCMOS33 } [get_ports { OLED_RESET}];# PMODEB pin 7
set_property -dict { PACKAGE_PIN W16   IOSTANDARD LVCMOS33 } [get_ports { OLED_DC}];   # PMODEB pin 8
```

### OLED wiring (Pmod B)

| OLED pin | PYNQ-Z1 (Pmod B) |
|----------|------------------|
| VCC      | 3V3              |
| GND      | GND              |
| DIN/MOSI | W14 (pin 1)      |
| CLK/SCK  | Y14 (pin 2)      |
| CS       | T11 (pin 3)      |
| RES      | V16 (pin 7)      |
| DC       | W16 (pin 8)      |

## Linux / device tree

The AXI SPI and GPIO nodes are **generated** from the exported `.xsa` by
`meta-xilinx-tools`. `system-user.dtsi` then adds a `spidev` child so the
controller is usable from userspace:

```dts
&axi_quad_spi_0 {
    is-decoded-cs = <0>;
    status = "okay";
    spidev@0 {
        compatible = "rohm,dh2228fv";
        status = "okay";
        spi-max-frequency = <5000000>;
        reg = <0>;
    };
};
```

This makes the core appear as `/dev/spidev1.0` (driver `xilinx_spi`), while the
AXI GPIO appears as `/dev/gpiochip0` (line 0 = RESET, line 1 = DC).

## Application (`app/oled-spi-test`)

A C++20 application that renders a live clock on the OLED:

- `Utilities/FileHandle` — RAII wrapper around `open()` / `close()`.
- `SPIInterface/SPIDevice` — wraps `ioctl(fd, SPI_IOC_MESSAGE, ...)` with
  `Write`, `Read`, `Transfer` helpers.
- `Gpio/Gpio` — `libgpiod` v2 wrapper for the RESET/DC lines.
- `OLEDDriver` — SH1106 register map, framebuffer and text rendering
  (`Font8x8.hpp`).
- `main.cpp` — initializes the display and prints day / date / time every second.

The device paths are set in `main.cpp`:

```cpp
spi_config.device_path = "/dev/spidev1.0";
Gpio::Gpio gpio{"/dev/gpiochip0", {0, 0}, {0, 1}};   // outputs: RESET=0, DC=1
```

## Build

### 1. Hardware — Vivado

```bash
source <Vivado_install>/settings64.sh
cd Vivado
make
```

`make` runs the Tcl scripts to create the project, build the bitstream, and
export `pynq_z1_top.xsa` (plus `pynq_z1_top.bit`) at the `Vivado/` root.

### 2. Linux image — Yocto (kas)

Prepare the host for Yocto:

- [Preparing the build host](https://docs.yoctoproject.org/singleindex.html#preparing-the-build-host)
- [System requirements (Scarthgap)](https://docs.yoctoproject.org/scarthgap/ref-manual/system-requirements.html#system-requirements)

Install `kas`:

```bash
pip install kas
```

Build:

```bash
cd yocto
make
```

The machine config (`yocto/meta-user/conf/machine/zynq-generic-7z020.conf`) reads
the hardware design directly via `HDF_PATH = ".../Vivado/pynq_z1_top.xsa"`, so
step 1 must run first. `kas build build.yaml` then produces the SD-card image —
the `.wic` file under `yocto/build/tmp-glibc/deploy/images/zynq-generic-7z020/`
(e.g. `image-pynq-z1-zynq-generic-7z020.rootfs.wic`).

### 3. Flash & run

1. Flash the `.wic` image to a microSD card (e.g. with Balena Etcher).
2. Insert the card, connect the USB UART (115200 8N1), and power the board.
3. Log in as `root` and run:

```bash
/home/root/oled-spi-test
```

The OLED should show the current day, date and time, updating every second.

## Repository layout

```
.
├── Vivado/                  # Hardware: block design, VHDL top, constraints
│   ├── scripts/             #   create_project.tcl, pynq_z1_bd.tcl, build_bitstream.tcl
│   └── src/                 #   pynq_z1_top.vhd, pynq_z1.xdc
├── yocto/                   # Linux image: build.yaml + meta-user layer
│   └── meta-user/
│       ├── recipes-bsp/     #   device-tree, u-boot
│       ├── recipes-kernel/  #   kernel config
│       └── recipes-apps/    #   oled-spi-test recipe
└── app/
    └── oled-spi-test/       # C++20 SH1106 OLED clock application
```

## References

- [AXI Quad SPI v3.2 — Product Guide (PG153)](https://docs.amd.com/r/en-US/pg153-axi-quad-spi)
- [AXI GPIO — Product Guide (PG144)](https://docs.amd.com/r/en-US/pg144-axi-gpio)
- [PYNQ-Z1 Reference Manual](https://digilent.com/reference/programmable-logic/pynq-z1/reference-manual)
- [Waveshare 1.3" OLED (A)](https://www.waveshare.com/wiki/1.3inch_OLED_(A))

