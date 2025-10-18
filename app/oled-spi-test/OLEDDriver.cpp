#include "OLEDDriver.hpp"

namespace Display
{
    OLEDDriver::OLEDDriver(SPI::SPIDevice &spi_device,
                           Gpio::Gpio &gpio,
                           const OLEDConfig &config)
        : m_SpiDevice(spi_device), m_Gpio(gpio), m_Config(config)
    {

        // Initialize display buffer (128x64 = 8192 bits = 1024 bytes, organized as 8 pages of 128 bytes)
        size_t buffer_size = (m_Config.width * m_Config.height) / 8;
        m_DisplayBuffer.resize(buffer_size, 0);

        SPDLOG_DEBUG("OLEDDriver created with width: {}, height: {}, reset_pin: {}, dc_pin: {}",
                     m_Config.width, m_Config.height, static_cast<int>(m_Config.reset_pin), static_cast<int>(m_Config.dc_pin));
    }
    OLEDDriver::~OLEDDriver()
    {
        if (m_Initialized)
        {
            SetDisplayOn(false);
            SPDLOG_DEBUG("OLEDDriver destructor called, deinitializing display.");
        }
    }

    OLEDDriver::OLEDDriver(OLEDDriver &&other) noexcept
        : m_SpiDevice(other.m_SpiDevice),
          m_Gpio(other.m_Gpio),
          m_Config(std::move(other.m_Config)),
          m_DisplayBuffer(std::move(other.m_DisplayBuffer)),
          m_Initialized(other.m_Initialized),
          m_LastError(std::move(other.m_LastError))
    {

        other.m_Initialized = false;
    }

    OLEDDriver &OLEDDriver::operator=(OLEDDriver &&other) noexcept
    {
        if (this != &other)
        {
            m_Config = std::move(other.m_Config);
            m_DisplayBuffer = std::move(other.m_DisplayBuffer);
            m_Initialized = other.m_Initialized;
            m_LastError = std::move(other.m_LastError);

            other.m_Initialized = false;
        }
        return *this;
    }

    bool OLEDDriver::Initialize()
    {
        if (m_Initialized)
        {
            SPDLOG_WARN("OLEDDriver is already initialized.");
            return true;
        }

        if (!m_SpiDevice.IsOpen())
        {
            SPDLOG_ERROR("SPI Device is not open. Cannot initialize OLEDDriver.");
            return false;
        }

        HardwareReset();

        try
        {
            SendCommand(SH1106_DISPLAYOFF); //--turn off oled panel

            SendCommand(SH1106_SETDISPLAYCLOCKDIV);  //--set display clock divide ratio/oscillator frequency
            SendCommand(0x80);                       //--the suggested ratio 0x80
            SendCommand(SH1106_SETMULTIPLEX);        //--set multiplex ratio(1 to 64)
            SendCommand(0x3F);                       //--1/64 duty
            SendCommand(SH1106_SETDISPLAYOFFSET);    //--set display offset
            SendCommand(0x00);                       //--no offset
            SendCommand(SH1106_SETSTARTLINE);        //--set start line address
            SendCommand(SH1106_CHARGEPUMP);          //--set charge pump
            SendCommand(0x8A);                       //--enable charge pump
            SendCommand(SH1106_SEGREMAP);            //--set segment re-map 0 to 127
            SendCommand(SH1106_COMSCANDEC);          //--set com output scan direction
            SendCommand(SH1106_SETCOMPINS);          //--set com pins hardware configuration
            SendCommand(0x12);                       //
            SendCommand(SH1106_SETCONTRAST);         //--set contrast control register
            SendCommand(m_Config.contrast);          //
            SendCommand(SH1106_SETPRECHARGE);        //--set pre-charge period
            SendCommand(0x1F);                       //
            SendCommand(SH1106_SETVCOMDETECT);       //--set vcomh deseect level
            SendCommand(0x40);                       //
            SendCommand(SH1106_DISPLAYALLON_RESUME); //--disable entire display on
            SendCommand(SH1106_NORMALDISPLAY);       //--set normal display
            SendCommand(SH1106_DISPLAYON);           //--turn on oled panel

            m_Initialized = true;
            SPDLOG_DEBUG("OLEDDriver initialized successfully.");

            Clear();
            Display();

            return true;
        }
        catch (const std::exception &e)
        {
            SetError(std::string("Exception during OLED initialization: ") + e.what());
            return false;
        }
    }

    void OLEDDriver::Clear()
    {
        if (!m_Initialized)
        {
            SetError("Display not initialized");
            return;
        }

        std::fill(m_DisplayBuffer.begin(), m_DisplayBuffer.end(), 0x00);
        SPDLOG_DEBUG("OLED display buffer cleared.");
    }

    void OLEDDriver::Display()
    {
        if (!m_Initialized)
        {
            SetError("Display not initialized");
            return;
        }

        for (uint8_t page = 0; page < 8; page++)
        {
            SetPageAddress(page);
            SetColumnAddress(m_Config.start_column);

            size_t start_index = page * m_Config.width;
            size_t end_index = start_index + m_Config.width;
            std::vector<uint8_t> page_data(m_DisplayBuffer.begin() + start_index, m_DisplayBuffer.begin() + end_index);

            SendDataArray(page_data);
        }

        SPDLOG_DEBUG("OLED display buffer sent to display.");
    }

    void OLEDDriver::HardwareReset()
    {
        SPDLOG_DEBUG("Performing hardware reset on OLED display using reset pin {}", static_cast<int>(m_Config.reset_pin));

        m_Gpio.SetOutput(m_Config.reset_pin, Gpio::Value::HIGH);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        m_Gpio.SetOutput(m_Config.reset_pin, Gpio::Value::LOW);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        m_Gpio.SetOutput(m_Config.reset_pin, Gpio::Value::HIGH);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void OLEDDriver::SetDcPin(bool is_data)
    {
        m_Gpio.SetOutput(m_Config.dc_pin, is_data ? Gpio::Value::HIGH : Gpio::Value::LOW);
    }

    void OLEDDriver::SetError(const std::string &error)
    {
        m_LastError = error;
        SPDLOG_ERROR("OLEDDriver Error: {}", error);
    }

    bool OLEDDriver::IsValidCoordinate(uint8_t x, uint8_t y) const noexcept
    {
        return (x < m_Config.width) && (y < m_Config.height);
    }

    size_t OLEDDriver::GetBufferIndex(uint8_t x, uint8_t y) const noexcept
    {
        // SH1106 buffer organization: 8 pages, each page is 128 bytes wide
        // Each byte represents 8 vertical pixels
        return (y / 8) * m_Config.width + x;
    }

    void OLEDDriver::SendCommand(uint8_t command)
    {
        SetDcPin(false); // Command mode
        if (!m_SpiDevice.WriteByte(command))
        {
            SetError("Failed to send command: 0x" + std::to_string(command));
        }
    }

    void OLEDDriver::SendData(uint8_t data)
    {
        SetDcPin(true); // Data mode

        if (!m_SpiDevice.WriteByte(data))
        {
            SetError("Failed to send data: 0x" + std::to_string(data));
        }
    }

    void OLEDDriver::SendDataArray(const std::vector<uint8_t> &data)
    {
        if (data.empty())
            return;

        SetDcPin(true); // Data mode

        if (!m_SpiDevice.Write(data))
        {
            SetError("Failed to send data array of size " + std::to_string(data.size()));
        }
    }

    void OLEDDriver::SetDisplayOn(bool on)
    {
        if (!m_Initialized)
        {
            SetError("Display not initialized");
            return;
        }

        SendCommand(on ? SH1106_DISPLAYON : SH1106_DISPLAYOFF);
        SPDLOG_DEBUG("SH1106 display turned {}", on ? "on" : "off");
    }

    void OLEDDriver::SetPageAddress(uint8_t page)
    {
        SendCommand(SH1106_SETPAGEADDR | (page & 0x0F));
    }

    void OLEDDriver::SetColumnAddress(uint8_t column)
    {
        SendCommand(SH1106_SETLOWCOLUMN | (column & 0x0F));
        SendCommand(SH1106_SETHIGHCOLUMN | ((column >> 4) & 0x0F));
    }

    void OLEDDriver::SetInvertMode(bool invert)
    {
        if (!m_Initialized)
        {
            SetError("Display not initialized");
            return;
        }

        SendCommand(invert ? SH1106_INVERTDISPLAY : SH1106_NORMALDISPLAY);
        SPDLOG_DEBUG("SH1106 display invert mode set to {}", invert ? "INVERTED" : "NORMAL");
    }

    void OLEDDriver::SetContrast(uint8_t contrast)
    {
        if (!m_Initialized)
        {
            SetError("Display not initialized");
            return;
        }

        SendCommand(SH1106_SETCONTRAST);
        SendCommand(contrast);
        m_Config.contrast = contrast;
        SPDLOG_DEBUG("SH1106 display contrast set to {}", static_cast<int>(contrast));
    }

    bool OLEDDriver::IsInitialized() const
    {
        return m_Initialized;
    }

    void OLEDDriver::SetPixel(uint8_t x, uint8_t y, bool color)
    {
        if (!IsValidCoordinate(x, y))
        {
            SetError("SetPixel: Invalid coordinates (" + std::to_string(x) + ", " + std::to_string(y) + ")");
            return;
        }

        size_t index = GetBufferIndex(x, y);
        uint8_t bit_position = y % 8;

        if (color)
        {
            m_DisplayBuffer[index] |= (1 << bit_position);
        }
        else
        {
            m_DisplayBuffer[index] &= ~(1 << bit_position);
        }
    }

    bool OLEDDriver::GetPixel(uint8_t x, uint8_t y)
    {
        if (!IsValidCoordinate(x, y))
        {
            SetError("GetPixel: Invalid coordinates (" + std::to_string(x) + ", " + std::to_string(y) + ")");
            return false;
        }

        size_t index = GetBufferIndex(x, y);
        uint8_t bit_mask = 1 << (y % 8);

        return (m_DisplayBuffer[index] & bit_mask) != 0;
    }

    void OLEDDriver::DrawHLine(uint8_t x, uint8_t y, uint8_t length, bool color)
    {
        for (uint8_t i = 0; i < length && (x + i) < m_Config.width; i++)
        {
            {
                SetPixel(x + i, y, color);
            }
        }
    }

    void OLEDDriver::DrawVLine(uint8_t x, uint8_t y, uint8_t length, bool color)
    {
        for (uint8_t i = 0; i < length && (y + i) < m_Config.height; i++)
        {
            {
                SetPixel(x, y + i, color);
            }
        }
    }

    void OLEDDriver::DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color, bool filled)
    {
        if (filled)
        {
            for (uint8_t i = 0; i < height && (y + i) < m_Config.height; i++)
            {
                DrawHLine(x, y + i, width, color);
            }
        }
        else
        {
            DrawHLine(x, y, width, color);              // Top
            DrawHLine(x, y + height - 1, width, color); // Bottom
            DrawVLine(x, y, height, color);             // Left
            DrawVLine(x + width - 1, y, height, color); // Right
        }
    }

} // namespace Display