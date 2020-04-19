/*
http://www.shaels.net/index.php/cpm80-22-documents/cpm-bdos/31-bdos-overview
BDOS was an OS that provided functionality to the 8080 chip.
The CPU DIAG program uses BDOS to print output states, this implementation
is designed to mimic the behaviour of the BDOS unit.
*/

#pragma once

#include <iostream>
#include <string>
#include <sstream>

// Forward declaration
class Bus;

class BDOS
{
    public:
        // Constuctor/descructor
        BDOS();
        ~BDOS();

        // BDOS variables
        Bus * bus;
        char error_char;
        std::stringstream error_msg;

        // Called by Bus on creation to link to BDOS
        void connect_bus(Bus * new_bus);

        // Processes a request based on register values
        void bdos_request(uint8_t C, uint8_t D, uint8_t E);

    private:
        // BDOS write byte
        void write_byte(uint8_t val);

        // BDOS write $ terminated string
        void write_string(uint16_t error_addr);
};
