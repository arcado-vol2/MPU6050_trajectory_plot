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

    bool Port::IsAvailable() {
        std::string fullPortName = "\\\\.\\" + portName;
        HANDLE hTest = CreateFileA(fullPortName.c_str(),
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hTest == INVALID_HANDLE_VALUE) {
            return false;
        }

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(hTest, &dcbSerialParams)) {
            CloseHandle(hTest);
            return false;
        }

        CloseHandle(hTest);
        return true;
    }

    bool Port::Open() {

        //No need to call IsAvaible cos it create temp handler and here we creating main handler
        //So if I decide to call IsAvaible here code will create two handlers
        std::string fullPortName = "\\\\.\\" + portName;
        //Basicly I send only float and int values, so it's wiser to use ANSI
        hSerial = CreateFileA(fullPortName.c_str(),
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
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
        std::cout << "COM port " << portName << " opens successfully!" << std::endl;
        return true;
    }

    std::string Port::Read() {
        if (!isOpen || hSerial == INVALID_HANDLE_VALUE) {
            return "";
        }

        DWORD bytesRead;
        char byte;
        std::string line;

        while (true) {
            if (!ReadFile(hSerial, &byte, 1, &bytesRead, NULL)) {
                return "";
            }

            if (bytesRead > 0) {
                if (byte == '\n') {
                    return line;
                }
                line += byte;
            }
            else {
                return "";
            }
        }
    }
    
    int Port::BytesAvailable() {
        ClearCommError(hSerial, &errors, &status);
        return status.cbInQue;
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