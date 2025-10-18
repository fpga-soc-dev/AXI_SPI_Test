#include <thread>
#include <chrono>
#include "OLEDDriver.hpp"
#include "SPIDevice.hpp"
#include "Gpio.hpp"

int main()
{
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%^%l%$] %v");

    SPI::SPIConfig spi_config;
    spi_config.device_path = "/dev/spidev1.0";
    SPI::SPIDevice spi_device{spi_config};
    Gpio::Gpio gpio{"/dev/gpiochip0", {0, 0}, {0, 1}};

    Display::OLEDConfig config;
    Display::OLEDDriver oled_driver(spi_device, gpio, config);

    if (!oled_driver.Initialize())
    {
        return -1;
    }

    oled_driver.SetInvertMode(true);
    oled_driver.SetContrast(0xFF);

    while (true)
    {
        oled_driver.SetDisplayOn(true);
        oled_driver.DrawRect(10, 10, 50, 30, true, false);
        oled_driver.Display();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}