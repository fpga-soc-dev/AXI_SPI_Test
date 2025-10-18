#pragma once

#include "FileHandle.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <linux/spi/spidev.h>
#include <spdlog/spdlog.h>

namespace SPI
{
    struct SPIConfig
    {
        std::string device_path{"/dev/spidev0.0"};
        uint32_t speed_hz{1000000};
        uint8_t bits_per_word{8};
        uint8_t mode{SPI_MODE_0};
        uint16_t delay_usecs{0};
        bool cs_change{false};
    };

    class SPIDevice
    {
    public:
        explicit SPIDevice(const SPIConfig &config);

        ~SPIDevice();
        SPIDevice(const SPIDevice &) = delete;
        SPIDevice &operator=(const SPIDevice &) = delete;
        SPIDevice(SPIDevice &&other) noexcept;
        SPIDevice &operator=(SPIDevice &&other) noexcept;

        bool Write(const std::vector<uint8_t> &data);
        std::vector<uint8_t> Read(size_t length);
        std::vector<uint8_t> Transfer(const std::vector<uint8_t> &tx_data);
        bool WriteByte(uint8_t byte);
        uint8_t ReadByte();

        bool IsOpen() const;

    private:
        SPIConfig m_Config;
        std::string m_LastError;
        std::unique_ptr<Utils::FileHandle> m_FileHandle;

        bool PerformTransfer(struct spi_ioc_transfer &transfer);
        void SetError(const std::string &error);
        bool ApplyConfig();
    };

} // namespace SPI
