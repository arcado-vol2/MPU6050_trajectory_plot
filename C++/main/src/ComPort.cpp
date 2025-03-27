#include "ComPort.h"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <sstream>

namespace COM {
    Port::Port(const std::string& port, int speed)
        : portName(port), portSpeed(speed), hSerial(INVALID_HANDLE_VALUE), isOpen(false) {}

    Port::~Port() {
        Close();
    }

    bool Port::IsOpen() {
        return isOpen;
    }

    bool Port::Open() {
        std::string fullPortName = "\\\\.\\" + portName;
        std::wstring widePortName(fullPortName.begin(), fullPortName.end());

       
        hSerial = CreateFileW(widePortName.c_str(), GENERIC_READ, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hSerial == INVALID_HANDLE_VALUE) {
            std::cerr << "Error opening port " << portName << ". Error code: " << GetLastError() << std::endl;
            return false;
        }

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hSerial, &dcbSerialParams)) {
            std::cerr << "Can not get port parameters" << std::endl;
            CloseHandle(hSerial);
            return false;
        }

        dcbSerialParams.BaudRate = portSpeed;
        dcbSerialParams.ByteSize = BYTE_SIZE;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            std::cerr << "Error configurating port parameters" << std::endl;
            CloseHandle(hSerial);
            return false;
        }

        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = READ_INTERVAL_TIMEOUT;
        timeouts.ReadTotalTimeoutConstant = READ_TOTAL_TIMEOUT_CONST;
        timeouts.ReadTotalTimeoutMultiplier = READ_TOTAL_TIMEOUT_MULT;
        timeouts.WriteTotalTimeoutConstant = WRITE_TOTAL_TIMEOUT_CONST;
        timeouts.WriteTotalTimeoutMultiplier = WRITE_TOTAL_TIMEOUT_MULT;

        if (!SetCommTimeouts(hSerial, &timeouts)) {
            std::cerr << "Error setup port timeout parameters" << std::endl;
            CloseHandle(hSerial);
            return false;
        }

        isOpen = true;
        std::cout << "COM port " << portName << " opens sucesessfuly!" << std::endl;
        return true;
    }

    std::string Port::Read() {
        if (!isOpen) return "";

        static std::vector<char> buffer(4096);
        static size_t buffer_pos = 0;

        COMSTAT comStat;
        DWORD errors;
        if (!ClearCommError(hSerial, &errors, &comStat) || comStat.cbInQue == 0) {
            return "";
        }

        // Читаем все доступные данные
        DWORD bytes_to_read = comStat.cbInQue;

        // Убедимся, что есть место для новых данных и нуль-терминатора
        if (bytes_to_read > buffer.size() - buffer_pos - 1) {  // -1 для нуль-терминатора
            buffer.resize(buffer_pos + bytes_to_read + 1);  // +1 для нуль-терминатора
        }

        DWORD bytes_read = 0;
        if (!ReadFile(hSerial, buffer.data() + buffer_pos, bytes_to_read, &bytes_read, nullptr) || bytes_read == 0) {
            return "";
        }

        buffer_pos += bytes_read;

        // Проверяем, что buffer_pos в пределах вектора перед добавлением терминатора
        if (buffer_pos >= buffer.size()) {
            buffer.push_back('\0');
        }
        else {
            buffer[buffer_pos] = '\0';
        }

        // Ищем символ новой строки ('\n') — признак конца строки
        char* line_end = strchr(buffer.data(), '\n');
        if (!line_end) {
            return "";  // Полная строка ещё не пришла
        }

        // Вырезаем строку (исключая '\n')
        std::string line(buffer.data(), line_end);

        // Сдвигаем оставшиеся данные в начало буфера
        size_t remaining_data = buffer_pos - (line_end - buffer.data() + 1);
        if (remaining_data > 0) {
            memmove(buffer.data(), line_end + 1, remaining_data);
        }
        else {
            // Если данных не осталось, просто сбрасываем позицию
            remaining_data = 0;
        }
        buffer_pos = remaining_data;

        // Добавляем нуль-терминатор для оставшихся данных
        if (buffer_pos < buffer.size()) {
            buffer[buffer_pos] = '\0';
        }
        else if (!buffer.empty()) {
            buffer.back() = '\0';
        }

        return line;
    }

    const std::string& Port::GetName() const {
        return portName;
    }



    std::vector<std::string> GetAvailableComPorts() {
        std::vector<std::string> comPorts;
        const int maxPortNumber = 256;

        for (int i = 1; i <= maxPortNumber; ++i) {
            std::string portName = "COM" + std::to_string(i);
            std::wstring widePortName = L"\\\\.\\" + std::wstring(portName.begin(), portName.end());

            HANDLE hPort = CreateFileW(
                widePortName.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            if (hPort != INVALID_HANDLE_VALUE) {
                comPorts.push_back(portName);
                CloseHandle(hPort);
            }
        }

        return comPorts;
    }

    void Port::Close() {
        if (isOpen) {
            PurgeComm(hSerial, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
            isOpen = false;
            std::cout << "Port " << portName << " closed successfully." << std::endl;
        }
    }

}