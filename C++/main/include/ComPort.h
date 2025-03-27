#ifndef COMPORT_H
#define COMPORT_H

#include <string>
#include <vector>
#include <windows.h>

namespace COM {

    #define BYTE_SIZE 8
    #define READ_INTERVAL_TIMEOUT 50
    #define READ_TOTAL_TIMEOUT_CONST 50
    #define READ_TOTAL_TIMEOUT_MULT 10
    #define WRITE_TOTAL_TIMEOUT_CONST 50
    #define WRITE_TOTAL_TIMEOUT_MULT 10

    /**
    * @class Port
    * @brief Class for connect serial port and read values from it
    */
    class Port {

    private:
        HANDLE hSerial;
        std::string portName;
        int portSpeed;
        bool isOpen;

    public:
        /*
        * @brief Default constructor
        */
        Port() : hSerial(INVALID_HANDLE_VALUE), portName(""), portSpeed(0), isOpen(false) {}
        /**
        * @param port port name
        * @param speed serial speed
        */
        Port(const std::string& port, int speed);
        ~Port();

        /**
        * @brief opens port
        * @return success of opening 
        */
        bool Open();
        /**
        * @brief read value from port
        * @return read result
        */
        std::string Read();
        /**
        * @brief name of port
        * @return name of port
        */
        const std::string& GetName() const;

        /**
        * @brief release com device
        */
        void Close();

        bool IsOpen();
        
    };

    /**
    * @brief search for all available port
    * @return list of ready for work ports (occupied not counts)
    */
    std::vector<std::string> GetAvailableComPorts();

} 

#endif // COMPORT_H
