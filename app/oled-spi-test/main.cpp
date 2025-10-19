#include <thread>
#include <chrono>
#include "OLEDDriver.hpp"
#include "SPIDevice.hpp"
#include "Gpio.hpp"

std::string getCurrentTime24()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S");
    return ss.str();
}

std::string getCurrentDateShort()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%d/%m/%Y");
    return ss.str();
}

std::string getDayOfWeek()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%A");
    return ss.str();
}

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
    oled_driver.SetDisplayOn(false);
    // oled_driver.SetInvertMode(true);
    // oled_driver.SetContrast(0xFF);

    while (true)
    {
        oled_driver.Clear();
        oled_driver.SetDisplayOn(true);

        oled_driver.PrintLine(getDayOfWeek(), true, 1);
        oled_driver.PrintLine(getCurrentDateShort(), true, 1);
        oled_driver.PrintLine(getCurrentTime24(), true, 1);

        oled_driver.Display();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}