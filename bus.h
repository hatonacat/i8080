#include <array>
#include <cstdint>
#include <vector>

#include "BDOS.h"
#include "i8080.h"

class Bus
{
    public:
        Bus();
        ~Bus();

    public:
        // Add CPU to bus
        i8080 cpu;
        BDOS bdos;

        // Initialise RAM
        std::array<uint8_t, 16*1024> ram;

    public:
        // Interfaces between the CPU and the BDOS output
        void bdos_request(uint8_t C, uint8_t D, uint8_t E);
        
        // Read data from designated address on bus (from RAM)
        // If specified, start loading from address start_addr
        void load_rom(const char* filename, uint16_t start_addr);

        // RAM access functions
        uint8_t read_from_ram(uint16_t addr);
        void write_to_ram(uint16_t addr, uint8_t data);
};