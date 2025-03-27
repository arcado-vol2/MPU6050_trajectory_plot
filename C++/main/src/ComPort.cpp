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

        char buffer[256];
        DWORD bytesRead;
        DWORD errors;
        COMSTAT status;

        ClearCommError(hSerial, &errors, &status);
        if (status.cbInQue > 0) {
            DWORD toRead = min(sizeof(buffer) - 1, status.cbInQue);

            if (ReadFile(hSerial, buffer, toRead, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                return std::string(buffer);
            }
        }
        return "";
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