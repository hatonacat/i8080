#include <iomanip>
#include <iostream>
#include <sstream>

#include "BDOS.h"
#include "bus.h"

BDOS::BDOS() {};
BDOS::~BDOS() {};

void BDOS::connect_bus(Bus * new_bus) 
{
    bus = new_bus;
}


void BDOS::bdos_request(uint8_t C, uint8_t D, uint8_t E)
{
    if(C==9)
    {
        write_string((D<<8) | E);
    }
    else if (C==2)
    {
        write_byte(E);
    }
}

void BDOS::write_byte(uint8_t val)
{
    //error_msg << std::hex << std::setfill('0') << std::setw(2) << (int)val;
    error_msg << std::hex << (int)val;
    std::cout << error_msg.str() << std::endl;
}

// BDOS operation is C==9. Will print characters
// at the error_addr until it hits a '$'
void BDOS::write_string(uint16_t error_addr)
{
    error_addr += 4; // Skips unnecessary leading characters
    error_char = (char)bus->read_from_ram(error_addr);

    while(error_char != '$')
    {
        error_msg << error_char;
        //std::cout << error_char; 
        error_addr++;
        error_char = (char)bus->read_from_ram(error_addr);

    }
    std::cout << error_msg.str() << std::endl;
}

