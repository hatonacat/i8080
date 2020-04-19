#include <iostream>
#include <fstream>
#include <string>

#include "bus.h"

#define CPUDIAG

using namespace std;


int main(int argc, char *argv[])
{
    // If a file is provided use it, otherwise use a default ROM
    const char* filename;
    if(argv[1]){
        filename=argv[1];
        //cout << "Using custom ROM: " << filename << endl;
    } 
    else {
        filename = "roms/default";
        //cout << "Using default ROM location: " << filename << endl;
    }   

    // Create bus and write ROM into RAM
    Bus bus;
    uint16_t org = 0; // File address origin

#ifdef CPUDIAG
    filename = "test/cpudiag.bin";
    cout << "CPUDIAG requested, loading test: " << filename << endl;
    org = 0x0100;
#endif

    bus.load_rom(filename, org);

#ifdef CPUDIAG
    // Add instruction to jump to starting location 0x0100
    // bus.write_to_ram(0, 0xc3);
    // bus.write_to_ram(1, 0x00);
    // bus.write_to_ram(2, 0x01);

    // Fix stack pointer bug
    bus.write_to_ram(368, 0x07);

    // Skip DAA test
    bus.write_to_ram(0x59c, 0xc3);//JMP 
    bus.write_to_ram(0x59d, 0xc2);
    bus.write_to_ram(0x59e, 0x05); 

#endif

    // Run a defined number of steps
    // for(int i=0; i!=800; ++i){
    //     bus.cpu.clock();
    // }

    // This is used to detect an exit condition when the program 
    // goes through long looping periods - it will be the normal 
    // operating mode in future, but is just a temporary debug tool for now.    
    bool stop_found = 0;
    while(!stop_found) 
    {
        stop_found = bus.cpu.clock();
    };

    return 0;
}