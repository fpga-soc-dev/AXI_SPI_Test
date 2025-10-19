#pragma once

#include "SPIDevice.hpp"
#include "Gpio.hpp"
#include <cstdint>
#include <vector>
#include <string>

namespace Display
{
    struct OLEDConfig
    {
        uint8_t reset_pin{0}; // GPIO pin for reset
        uint8_t dc_pin{1};    // GPIO pin for DC (Data/Command)

        uint8_t width{128};
        uint8_t height{64};
        uint8_t start_column{2};
        uint8_t contrast{0x80};
    };

    class OLEDDriver
    {
    public:
        OLEDDriver(SPI::SPIDevice &spi_device,
                   Gpio::Gpio &gpio,
                   const OLEDConfig &config = OLEDConfig{});

        ~OLEDDriver();

        OLEDDriver(const OLEDDriver &) = delete;
        OLEDDriver &operator=(const OLEDDriver &) = delete;
        OLEDDriver(OLEDDriver &&other) noexcept;
        OLEDDriver &operator=(OLEDDriver &&other) noexcept;

        bool Initialize();
        void SetDisplayOn(bool on);
        void SetInvertMode(bool invert);
        void SetContrast(uint8_t contrast);
        bool IsInitialized() const;
        void Clear();
        void Display();

        void SetPixel(uint8_t x, uint8_t y, bool color);
        bool GetPixel(uint8_t x, uint8_t y);
        void DrawHLine(uint8_t x, uint8_t y, uint8_t length, bool color);
        void DrawVLine(uint8_t x, uint8_t y, uint8_t length, bool color);
        void DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color, bool filled = false);

        void DrawString(uint8_t x, uint8_t y, const std::string &text, bool color, uint8_t size = 1);
        void PrintLine(const std::string &text, bool color, uint8_t size);

    private:
        static constexpr uint8_t SH1106_DISPLAYOFF = 0xAE;
        static constexpr uint8_t SH1106_DISPLAYON = 0xAF;
        static constexpr uint8_t SH1106_SETDISPLAYCLOCKDIV = 0xD5;
        static constexpr uint8_t SH1106_SETMULTIPLEX = 0xA8;
        static constexpr uint8_t SH1106_SETDISPLAYOFFSET = 0xD3;
        static constexpr uint8_t SH1106_SETSTARTLINE = 0x40;
        static constexpr uint8_t SH1106_CHARGEPUMP = 0x8D;
        static constexpr uint8_t SH1106_MEMORYMODE = 0x20;
        static constexpr uint8_t SH1106_SEGREMAP = 0xA1;
        static constexpr uint8_t SH1106_COMSCANDEC = 0xC8;
        static constexpr uint8_t SH1106_SETCOMPINS = 0xDA;
        static constexpr uint8_t SH1106_SETCONTRAST = 0x81;
        static constexpr uint8_t SH1106_SETPRECHARGE = 0xD9;
        static constexpr uint8_t SH1106_SETVCOMDETECT = 0xDB;
        static constexpr uint8_t SH1106_DISPLAYALLON_RESUME = 0xA4;
        static constexpr uint8_t SH1106_NORMALDISPLAY = 0xA6;
        static constexpr uint8_t SH1106_INVERTDISPLAY = 0xA7;
        static constexpr uint8_t SH1106_SETLOWCOLUMN = 0x02;
        static constexpr uint8_t SH1106_SETHIGHCOLUMN = 0x10;
        static constexpr uint8_t SH1106_SETPAGEADDR = 0xB0;

        SPI::SPIDevice &m_SpiDevice;
        Gpio::Gpio &m_Gpio;

        OLEDConfig m_Config;
        std::vector<uint8_t> m_DisplayBuffer;
        bool m_Initialized{false};
        std::string m_LastError;
        uint8_t m_Cursor_x = 0;
        uint8_t m_Cursor_y = 0;

        void SendCommand(uint8_t command);
        void SendData(uint8_t data);
        void SendDataArray(const std::vector<uint8_t> &data);

        void HardwareReset();
        void SetDcPin(bool is_data);
        // void SetCsPin(bool active);
        void SetError(const std::string &error);

        bool IsValidCoordinate(uint8_t x, uint8_t y) const noexcept;
        size_t GetBufferIndex(uint8_t x, uint8_t y) const noexcept;

        void SetColumnAddress(uint8_t column);
        void SetPageAddress(uint8_t page);
        void DrawChar(uint8_t x, uint8_t y, char ch, bool color, uint8_t size);
        uint8_t DrawStringAutoY(uint8_t x, uint8_t y, const std::string &text, bool color = true, uint8_t size = 1);
        void ResetCursor();
    };
};
