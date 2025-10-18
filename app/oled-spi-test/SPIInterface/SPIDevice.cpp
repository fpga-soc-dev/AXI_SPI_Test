#include "SPIDevice.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <spdlog/spdlog.h>

namespace SPI
{
    SPIDevice::SPIDevice(const SPIConfig &config)
        : m_Config(config),
          m_FileHandle(std::make_unique<Utils::FileHandle>(m_Config.device_path, O_RDWR))
    {
        if (!m_FileHandle || !m_FileHandle->IsOpen())
        {
            SPDLOG_ERROR("Failed to open SPI device at path: {}", m_Config.device_path);
            return;
        }

        if (!ApplyConfig())
        {
            SPDLOG_ERROR("Failed to apply SPI configuration");
            return;
        }

        SPDLOG_DEBUG("SPI Device initialized at path: {}", m_Config.device_path);
    }

    SPIDevice::~SPIDevice()
    {
    }

    SPIDevice::SPIDevice(SPIDevice &&other) noexcept
        : m_Config(std::move(other.m_Config)), m_FileHandle(std::move(other.m_FileHandle)), m_LastError(std::move(other.m_LastError))
    {
    }

    SPIDevice &SPIDevice::operator=(SPIDevice &&other) noexcept
    {
        if (this != &other)
        {
            m_Config = std::move(other.m_Config);
            m_FileHandle = std::move(other.m_FileHandle);
            m_LastError = std::move(other.m_LastError);
        }
        return *this;
    }

    bool SPIDevice::IsOpen() const
    {
        return m_FileHandle && m_FileHandle->IsOpen();
    }

    bool SPIDevice::Write(const std::vector<uint8_t> &data)
    {
        if (!m_FileHandle || !m_FileHandle->IsOpen())
        {
            SPDLOG_ERROR("SPI Device not open for writing");
            return false;
        }

        if (data.empty())
        {
            SPDLOG_WARN("No data provided to write to SPI Device");
            return false;
        }

        struct spi_ioc_transfer transfer = {};
        transfer.tx_buf = reinterpret_cast<uintptr_t>(data.data());
        transfer.len = data.size();
        transfer.speed_hz = m_Config.speed_hz;
        transfer.bits_per_word = m_Config.bits_per_word;
        transfer.delay_usecs = m_Config.delay_usecs;
        transfer.cs_change = m_Config.cs_change;

        auto result = PerformTransfer(transfer);
        if (result)
        {
            SPDLOG_DEBUG("Wrote {} bytes to SPI Device", data.size());
        }
        return result;
    }

    std::vector<uint8_t> SPIDevice::Read(size_t length)
    {
        if (!m_FileHandle || !m_FileHandle->IsOpen())
        {
            SPDLOG_ERROR("SPI Device not open for reading");
            return {};
        }

        if (length == 0)
        {
            SPDLOG_WARN("Requested read length is zero");
            return {};
        }

        std::vector<uint8_t> buffer(length, 0);
        struct spi_ioc_transfer transfer = {};
        transfer.rx_buf = reinterpret_cast<uintptr_t>(buffer.data());
        transfer.len = length;
        transfer.speed_hz = m_Config.speed_hz;
        transfer.bits_per_word = m_Config.bits_per_word;
        transfer.delay_usecs = m_Config.delay_usecs;
        transfer.cs_change = m_Config.cs_change;

        if (!PerformTransfer(transfer))
        {
            return {};
        }

        SPDLOG_DEBUG("Read {} bytes from SPI Device", length);
        return buffer;
    }

    std::vector<uint8_t> SPIDevice::Transfer(const std::vector<uint8_t> &tx_data)
    {
        if (!m_FileHandle || !m_FileHandle->IsOpen())
        {
            SPDLOG_ERROR("SPI Device not open for transfer");
            return {};
        }

        if (tx_data.empty())
        {
            SPDLOG_WARN("No data provided for SPI transfer");
            return {};
        }

        std::vector<uint8_t> rx_data(tx_data.size());

        struct spi_ioc_transfer transfer = {};
        transfer.tx_buf = reinterpret_cast<uintptr_t>(tx_data.data());
        transfer.rx_buf = reinterpret_cast<uintptr_t>(rx_data.data());
        transfer.len = tx_data.size();
        transfer.speed_hz = m_Config.speed_hz;
        transfer.bits_per_word = m_Config.bits_per_word;
        transfer.delay_usecs = m_Config.delay_usecs;
        transfer.cs_change = m_Config.cs_change;

        if (!PerformTransfer(transfer))
        {
            return {};
        }

        SPDLOG_DEBUG("Transferred {} bytes over SPI Device", tx_data.size());
        return rx_data;
    }

    bool SPIDevice::WriteByte(uint8_t byte)
    {
        return Write(std::vector<uint8_t>{byte});
    }

    uint8_t SPIDevice::ReadByte()
    {
        auto result = Read(1);
        return result.empty() ? 0 : result[0];
    }

    bool SPIDevice::PerformTransfer(struct spi_ioc_transfer &transfer)
    {
        if (!m_FileHandle || !m_FileHandle->IsOpen())
        {
            SPDLOG_ERROR("SPI Device not open for transfer");
            return false;
        }

        if (ioctl(m_FileHandle->GetFd(), SPI_IOC_MESSAGE(1), &transfer) < 0)
        {
            SPDLOG_ERROR("Failed to perform SPI transfer");
            return false;
        }

        return true;
    }

    bool SPIDevice::ApplyConfig()
    {
        if (!m_FileHandle || !m_FileHandle->IsOpen())
        {
            SetError("SPI Device not open for configuration");
            return false;
        }

        int fd = m_FileHandle->GetFd();

        // Set SPI mode
        if (ioctl(fd, SPI_IOC_WR_MODE, &m_Config.mode) == -1)
        {
            SetError("Failed to set SPI mode: " + std::string(std::strerror(errno)));
            return false;
        }

        // Set bits per word
        if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &m_Config.bits_per_word) == -1)
        {
            SetError("Failed to set bits per word: " + std::string(std::strerror(errno)));
            return false;
        }

        // Set max speed
        if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &m_Config.speed_hz) == -1)
        {
            SetError("Failed to set max speed: " + std::string(std::strerror(errno)));
            return false;
        }

        SPDLOG_INFO("Applied SPI configuration - Mode: {}, Speed: {} Hz, Bits: {}",
                    m_Config.mode, m_Config.speed_hz, m_Config.bits_per_word);

        return true;
    }

    void SPIDevice::SetError(const std::string &error)
    {
        m_LastError = error;
        SPDLOG_ERROR("SPI Device Error: {}", m_LastError);
    }

} // namespace SPI