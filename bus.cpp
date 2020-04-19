#include <fstream>

#include "bus.h"

Bus::Bus() 
{
    cpu.connect_bus(this);
    bdos.connect_bus(this);

    // Clear RAM
    for(auto& addr: ram)
    {
        addr=0;
    }
}

Bus::~Bus() {}

// Passes on the CPU's request to the BDOS unit
void Bus::bdos_request(uint8_t C, uint8_t D, uint8_t E) 
{
    bdos.bdos_request(C, D, E);
}

void Bus::load_rom(const char* filename, uint16_t start_addr=0)
{
    std::ifstream rom_file(filename, std::ios::binary);

    rom_file.seekg(0, std::ios::end);
    int fsize = rom_file.tellg();
    rom_file.seekg(0, std::ios::beg);

    rom_file.read((char*)&ram[start_addr], fsize);
}

uint8_t Bus::read_from_ram(uint16_t addr)
{
    if(addr >= 0x0000 && addr <= 0x4000)
    {
        return ram[addr];
    }
}

void Bus::write_to_ram(uint16_t addr, uint8_t data)
{
    if(addr >= 0x0000 && addr <= 0x4000)
    {
        ram[addr] = data;
    }
}


