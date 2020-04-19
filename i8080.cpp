#include <bitset>
#include <iostream>
#include <iomanip>

#include "bus.h"
#include "i8080.h"

#define CPUDIAG

using namespace std;

// Constructor
i8080::i8080() {
    // Headers for the print output
    //cout << "CLK\tOPS\tPC\tOPCODE\tALIAS\tDATA\tREGISTERS\t\t\t\t\t\tFLAGS\tSP CONTENTS" << endl;

#ifdef CPUDIAG
    PC = 0x0100; // The CPU test program origin is 0100
#endif
};

// Desctructor
i8080::~i8080() {}

// Clock
bool i8080::clock()
{
    if(cycles==0)
    {
        // Automaticlly progresses the program counter by 1
        // Addressing modes add additional steps if required
        PC_previous = PC;
        opcode = read(PC++);

        // Finds the related instruction in the instructions map
        // If not found, runs the 'not implemented' instruction.
        try {
            instruction = instructions.at(opcode);
        }
        catch(...) {
            instruction = not_implemented;
            stopped = 1; // Uncomment if searching for not implemented functions
        }   

        // Performs the opcode instruction
        (this->*instruction.addrmode)();

        // Performs the opcode instruction
        (this->*instruction.operation)();

        op_count ++;
        print_CPU_detail();
    }

    clock_count++;
    cycles--;


 #ifdef CPUDIAG
    // The CPU diag program will only get to 0000 following
    // a program failure.
    if(PC==0x0000) 
    {
        stopped = 1;
        cout << "CPU diag error found" << endl;
    }
#endif

    return stopped;
}

// Connect to bus
void i8080::connect_bus(Bus *new_bus)
{
    bus = new_bus;
};

// Read from ram via bus
uint8_t i8080::read(uint16_t addr)
{
    return bus->read_from_ram(addr);
}

// Write to ram via bus
void i8080::write(uint16_t addr, uint8_t data)
{
    bus->write_to_ram(addr, data);
}

// Arithmetic flag checking function
void i8080::flagcheck(vector<string> flags, uint8_t result) {}

// Sets or unsets the content of flag f based on the value 'set'
void i8080::set_flag(FLAGS8080 f, bool set)
{
    if(set) status |= f;
    else    status &= ~f;
}

// Returns the status of a given flag
bool i8080::get_flag(FLAGS8080 f)
{
    return status & f;
}

bool i8080::parity(uint8_t val)
{
    uint8_t x = 1;
    for (int i=0; i!=8; ++i)
    {
        x = x^(val>>i);
    }

    return (x&0x01);  
}


// Internal print instructions================
void i8080::print_CPU_detail()
{
    cout << setfill('0') << setw(6) << dec << (int)clock_count;
    cout << "\t" << setfill('0') << setw(5) << dec << (int)(op_count);
    // PC-1 is printed because the PC has been incremented by the time
    // the print function is called, we want to know what it was when it
    // referred to the opcode.
    cout << "\t" << setfill('0') << setw(4) << hex << (int)(PC_previous);
    
    cout << "\t" << "0x" << setfill('0') << setw(2) << right << hex << (int)opcode;
    cout << "\t" << instruction.alias;

    // This horrible block prints the data that follows an instruction if relevant
    if(this->instruction.addrmode==&i8080::IM8) {
        cout << "\t" << setfill('0') << setw(2) << hex << (int)(byte2);       
    }
    else if(this->instruction.addrmode==&i8080::IM16) {
        cout << "\t" << setfill('0') << setw(4) << hex << (int)((byte3<<8) | byte2);             
    }
    else if(this->instruction.addrmode==(&i8080::RGI8M) | this->instruction.addrmode==(&i8080::RGI8r)) {
        cout << "\t" << setfill('0') << setw(4) << hex << (int)(rp_addr);             
    }
    else if(this->instruction.addrmode==&i8080::RGI16) {
        cout << "\t" << setfill('0') << setw(4) << hex << (int)(rp_val);             
    }
    else if(this->instruction.addrmode==&i8080::IMRI) {
        cout << "\t" << setfill('0') << setw(4) << hex << (int)rp_addr << "/" << setw(2) << (int)byte2;             
    }
    else if(this->instruction.addrmode==&i8080::DIR) {
        switch(opcode)
        {
            case 0xd3: cout << "\t" << setfill('0') << setw(2) << hex << (int)byte2;
            //case 0x32: cout << "\t" << setfill('0') << setw(4) << hex << setw(4) << (int)dir_addr;
            default: cout << "\t" << setfill('0') << setw(4) << hex << (int)addr_val;
        }          
    }
    else
    {
        cout << "\t";
    }    

    // Prints the content of every register on the same line as the opcode description
    cout << "\t";
    for(auto &reg: reg_map)
    {
        cout << reg.first << ":" << hex << setfill('0') << setw(2) << right << (int)*reg.second << " ";
    }
    cout << "PC:" << hex << setfill('0') << setw(4) << right << (int)PC << " ";
    cout << "SP:" << hex << setfill('0') << setw(4) << right << (int)SP;
    cout << "\t" << bitset<8>(status);

    for(int i=0; i!=10; ++i)
    {
        cout << " ";
        uint8_t temp_val = read(SP+i);
        cout << hex << setfill('0') << setw(2) << (int)temp_val;
    }

    cout << endl;
}


// ADDRESSING MODES ==========================

// Address mode: Direct
uint8_t i8080::DIR() 
{
    switch(opcode)
    {
        // Special functions like OUT use direct addressing differently
        // so clumsy exceptions are included above default behaviour.
        case 0xd3: // OUT
            byte2 = read(PC);
            PC++;

            // Output ports haven't been implemented yet, so for
            // the moment an intermediate value is assigned a number
            // and the pointer is 'updated' to null.
            port_placeholder = byte2;

            port = nullptr;
            break;

        default:
            IM16();
            dir_addr = (byte3<<8) | byte2;
            addr_val = read( dir_addr );
            break;
    }

    return 0;
};

// Address mode: Immediate
// Returns either a single byte (data) or 2 byte (address) value
uint8_t i8080::IM8()
{
    byte2 = read(PC);
    PC++;

    return 0;
}

uint8_t i8080::IM16()
{
    byte2 = read(PC);
    PC++;
    byte3 = read(PC);
    PC++;

    return 0;
}

// Immediate/register direct
// This isn't real, but many 'Immediate' functions
// also directly address a register. This made up
// address mode is meant to simplify this.
uint8_t i8080::IMRD()
{
    IM8();
    RGD();
    return 0;
}

uint8_t i8080::IMRI()
{
    IM8();
    RGI8M();
    return 0;
}

// Address mode: Implicit
uint8_t i8080::IMP()
{
    return 0;
}

// Address mode: Register direct
uint8_t i8080::RGD()
{
    switch(opcode)
    {
        case 0x03: rh=&B; rl=&C; break; // INX B
        case 0x04: r=&B; break;         // INR B   
        case 0x05: r=&B; break;         // DCR B 
        case 0x09: rh=&B; rl=&C; break; // DAD B
        case 0x14: r=&D; break;         // INR D           
        case 0x15: r=&D; break;         // DCR D 
        case 0x0b: rh=&B; rl=&C; break; // DCR BC       
        case 0x0c: r=&C; break;         // INR C  
        case 0x0d: r=&C; break;         // DCR C
        
        case 0x13: rh=&D; rl=&E; break;
        case 0x19: rh=&D; rl=&E; break;
        case 0x1b: rh=&D; rl=&E; break; // DCX D 
        case 0x1c: r=&E; break;         // INR E  
        case 0x1d: r=&E; break;         // DCR E
        case 0x23: rh=&H; rl=&L; break;
        case 0x24: r=&H; break;         // INR H  
        case 0x25: r=&H; break;         // DCR H       
        case 0x29: rh=&H; rl=&L; break;
        case 0x2b: rh=&H; rl=&L; break; // DCX H
        case 0x2c: r=&L; break;         // INR L  
        case 0x2d: r=&L; break;         // DCR L        

        case 0x33: break;               // INX SP         
        case 0x39: break;               // DAD SP       
        case 0x3b: break;               // DCX SP
        case 0x3c: r=&A; break;         // INR A
        case 0x3d: r=&A; break;         // DCR A

        case 0x40: r1=&B; r2=&B; break; // MOV B,B
        case 0x41: r1=&B; r2=&C; break; // MOV B,C
        case 0x42: r1=&B; r2=&D; break; // MOV B,D
        case 0x43: r1=&B; r2=&E; break; // MOV B,E
        case 0x44: r1=&B; r2=&H; break; // MOV B,H
        case 0x45: r1=&B; r2=&L; break; // MOV B,L
        case 0x47: r1=&B; r2=&A; break; // MOV B,A 

        case 0x48: r1=&C; r2=&B; break; // MOV C,B        
        case 0x49: r1=&C; r2=&C; break; // MOV C,C
        case 0x4a: r1=&C; r2=&D; break; // MOV C,D
        case 0x4b: r1=&C; r2=&E; break; // MOV C,E
        case 0x4c: r1=&C; r2=&H; break; // MOV C,H
        case 0x4d: r1=&C; r2=&L; break; // MOV C,L
        case 0x4f: r1=&C; r2=&A; break; // MOV C,A

        case 0x50: r1=&D; r2=&B; break; // MOV D,B
        case 0x51: r1=&D; r2=&C; break; // MOV D,C
        case 0x52: r1=&D; r2=&D; break; // MOV D,D
        case 0x53: r1=&D; r2=&E; break; // MOV D,E
        case 0x54: r1=&D; r2=&H; break; // MOV D,H
        case 0x55: r1=&D; r2=&L; break; // MOV D,L
        case 0x57: r1=&D; r2=&A; break; // MOV D,A 

        case 0x58: r1=&E; r2=&B; break; // MOV E,B        
        case 0x59: r1=&E; r2=&C; break; // MOV E,C
        case 0x5a: r1=&E; r2=&D; break; // MOV E,D
        case 0x5b: r1=&E; r2=&E; break; // MOV E,E
        case 0x5c: r1=&E; r2=&H; break; // MOV E,H
        case 0x5d: r1=&E; r2=&L; break; // MOV E,L
        case 0x5f: r1=&E; r2=&A; break; // MOV E,A

        case 0x60: r1=&H; r2=&B; break; // MOV H,B
        case 0x61: r1=&H; r2=&C; break; // MOV H,C
        case 0x62: r1=&H; r2=&D; break; // MOV H,D
        case 0x63: r1=&H; r2=&E; break; // MOV H,E
        case 0x64: r1=&H; r2=&H; break; // MOV H,H
        case 0x65: r1=&H; r2=&L; break; // MOV H,L
        case 0x67: r1=&H; r2=&A; break; // MOV H,A

        case 0x68: r1=&L; r2=&B; break; // MOV L,B        
        case 0x69: r1=&L; r2=&C; break; // MOV L,C
        case 0x6a: r1=&L; r2=&D; break; // MOV L,D
        case 0x6b: r1=&L; r2=&E; break; // MOV L,E
        case 0x6c: r1=&L; r2=&H; break; // MOV L,H
        case 0x6d: r1=&L; r2=&L; break; // MOV L,L
        case 0x6f: r1=&L; r2=&A; break; // MOV L,A

        case 0x78: r1=&A; r2=&B; break; // MOV A,B        
        case 0x79: r1=&A; r2=&C; break; // MOV A,C
        case 0x7a: r1=&A; r2=&D; break; // MOV A,D
        case 0x7b: r1=&A; r2=&E; break; // MOV A,E
        case 0x7c: r1=&A; r2=&H; break; // MOV A,H
        case 0x7d: r1=&A; r2=&L; break; // MOV A,L
        case 0x7f: r1=&A; r2=&A; break; // MOV A,A

        case 0x80: r=&B; break;         // ADD B
        case 0x81: r=&C; break;         // ADD C
        case 0x82: r=&D; break;         // ADD D
        case 0x83: r=&E; break;         // ADD E
        case 0x84: r=&H; break;         // ADD H
        case 0x85: r=&L; break;         // ADD L
        case 0x87: r=&A; break;         // ADD A

        case 0x88: r=&B; break;         // ADC B
        case 0x89: r=&C; break;         // ADC C
        case 0x8a: r=&D; break;         // ADC D
        case 0x8b: r=&E; break;         // ADC E
        case 0x8c: r=&H; break;         // ADC H
        case 0x8d: r=&L; break;         // ADC L
        case 0x8f: r=&A; break;         // ADC A     

        case 0x90: r=&B; break;         // SUB B
        case 0x91: r=&C; break;         // SUB C
        case 0x92: r=&D; break;         // SUB D
        case 0x93: r=&E; break;         // SUB E
        case 0x94: r=&H; break;         // SUB H
        case 0x95: r=&L; break;         // SUB L
        case 0x97: r=&A; break;         // SUB A

        case 0x98: r=&B; break;         // SBB B
        case 0x99: r=&C; break;         // SBB C
        case 0x9a: r=&D; break;         // SBB D
        case 0x9b: r=&E; break;         // SBB E
        case 0x9c: r=&H; break;         // SBB H
        case 0x9d: r=&L; break;         // SBB L
        case 0x9f: r=&A; break;         // SBB A 

        case 0xa0: r=&B; break;         // ANA B
        case 0xa1: r=&C; break;         // ANA C
        case 0xa2: r=&D; break;         // ANA D
        case 0xa3: r=&E; break;         // ANA E
        case 0xa4: r=&H; break;         // ANA H
        case 0xa5: r=&L; break;         // ANA L
        case 0xa7: r1=&A; break;        // ANA A

        case 0xa8: r=&B; break;         // XRA B
        case 0xa9: r=&C; break;         // XRA C
        case 0xaa: r=&D; break;         // XRA D
        case 0xab: r=&E; break;         // XRA E
        case 0xac: r=&H; break;         // XRA H
        case 0xad: r=&L; break;         // XRA L
        case 0xaf: r=&A; break;         // XRA A

        case 0xb0: r=&B; break;         // ANA B
        case 0xb1: r=&C; break;         // ANA C
        case 0xb2: r=&D; break;         // ANA D
        case 0xb3: r=&E; break;         // ANA E
        case 0xb4: r=&H; break;         // ANA H
        case 0xb5: r=&L; break;         // ANA L
        case 0xb7: r1=&A; break;        // ANA A

        case 0xb8: r=&B; break;         // CMP B
        case 0xb9: r=&C; break;         // CMP C
        case 0xba: r=&D; break;         // CMP D
        case 0xbb: r=&E; break;         // CMP E
        case 0xbc: r=&H; break;         // CMP H
        case 0xbd: r=&L; break;         // CMP L
        case 0xbf: r=&A; break;         // CMP A

        case 0xc1: rh=&B; rl=&C; break; // POP B
        case 0xc5: rh=&B; rl=&C; break; // PUSH B

        case 0xd1: rh=&D; rl=&E; break; // POP D
        case 0xd5: rh=&D; rl=&E; break; // PUSH D

        case 0xe1: rh=&H; rl=&L; break; // POP H
        case 0xe5: rh=&H; rl=&L; break; // PUSH H
        case 0xe9: rh=&H; rl=&L; break; // PCHL 
        case 0xeb: break;               // All action takes place in the opcode function

        case 0xf5: rh=&A; rl=&status; break; // PUSH PSW
        case 0xf9: rh=&H; rl=&L; break; // SPHL 
    }
};

// Address mode: Register indirect from memory (RP address)
// RGI reads the data from the memory location
// specified by a register pair. The two 8-bit results are
// combined into one 16 bit address.
uint8_t i8080::RGI8r() 
{
    // Temporary variables
    uint8_t rl_addr = 0x00;
    uint8_t rh_addr = 0x00;

    switch(opcode)
    {
        // Used when moving from memory
        case 0x02: rh_addr=B; rl_addr=C;  break;    // STAX BC
        case 0x0a: rh_addr=B; rl_addr=C;  break;    // LDAX BC
        case 0x12: rh_addr=D; rl_addr=E;  break;    // STAX DE 
        case 0x1a: rh_addr=D; rl_addr=E;  break;    // LDAX DE
    }
    rp_addr = (rh_addr<<8)|rl_addr;
    rp_val = read(rp_addr);

    return 0;
}; 

// Address mode: Register indirect to memory 
// RGI reads the data from the memory location
// specified by a register pair. The two 8-bit results are
// combined into one 16 bit address.
uint8_t i8080::RGI8M() 
{
    // Temporary variables
    uint8_t rl_addr = L;
    uint8_t rh_addr = H;

    rp_addr = (rh_addr<<8)|rl_addr;
    rp_val = read(rp_addr);

    return 0;
}; 

// Address mode: Register indirect
uint8_t i8080::RGI16() 
{
    // Temporary variables
    uint8_t rl_addr = 0x00;
    uint8_t rh_addr = 0x00;

    rh_addr=read(SP+1); rl_addr=read(SP);

    rp_val = (rh_addr<<8)|rl_addr;   

    return 0;
}; 

// INSTRUCTIONS ===============================

// Instruction: ADD immediate with carry
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ACI()
{
    // Use higher precision to capture carry bit
    uint8_t carry = 0;
    if(get_flag(CY)==1) carry=1;

    uint16_t tempA = A + byte2 + carry;
    A = tempA & 0x00FF;

    // Carry/Borrow - Bit 0
    set_flag(CY, tempA&0xFF00);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: ADD register with carry
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ADCr()
{
    uint8_t carry = 0;
    if (get_flag(CY)==1) carry=1;

    // Use higher precision to capture carry bit
    uint16_t tempA = A + *r + carry;
    A = tempA & 0x00FF;

    // Carry/Borrow - Bit 0
    set_flag(CY, tempA&0xFF00);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: ADD memory with carry
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ADCM()
{
    uint8_t carry = 0;
    if (get_flag(CY)==1) carry=1;

    // Use higher precision to capture carry bit
    uint16_t tempA = A + rp_val + carry;
    A = tempA & 0x00FF;

    // Carry/Borrow - Bit 0
    set_flag(CY, tempA&0xFF00);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: ADD register
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ADDr()
{
    // Use higher precision to capture carry bit
    uint16_t tempA = A + *r;
    A = tempA & 0x00FF;

    // Carry/Borrow - Bit 0
    set_flag(CY, tempA&0xFF00);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: ADD memory
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ADDM()
{
    // Use higher precision to capture carry bit
    uint16_t tempA = A + rp_val;
    A = tempA & 0x00FF;

    // Carry/Borrow - Bit 0
    set_flag(CY, tempA&0xFF00);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: ADD immediate
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ADI()
{
    // Use higher precision to capture carry bit
    uint16_t tempA = A + byte2;
    A = tempA & 0x00FF;

    // Carry/Borrow - Bit 0
    set_flag(CY, tempA&0xFF00);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: AND register
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ANAr()
{
    A = A&(*r);

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: AND memory
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ANAM()
{
    A = A&(rp_val);

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: AND immediate
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ANI()
{
    A = A&(byte2);

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: Call
uint8_t i8080::CALL()
{
#ifdef CPUDIAG
    if (((byte3<<8)|byte2) == 5)
    {
        stopped=1;
        bus->bdos_request(C, D, E);
    }
    else    

#endif   

    {  
        //cout << "Standard call function" << endl;
        write(SP-1, (PC>>8));
        write(SP-2, (PC&0x00FF));
        SP -= 2;      
     
        PC = (byte3<<8)|byte2;
    }   

    cycles = 17;
    return 0;
}

// Instruction: Conditional Call(s)
uint8_t i8080::Cc()
{
    bool condition = 0;
    switch(opcode)
    {
        case 0xc4: if(get_flag(Z)==0)   condition=1; break;  // CNZ
        case 0xcc: if(get_flag(Z)==1)   condition=1; break;  // CZ      
        case 0xd4: if(get_flag(CY)==0)  condition=1; break;  // CNC
        case 0xdc: if(get_flag(CY)==1)  condition=1; break;  // CC
        case 0xe4: if(get_flag(P)==0)   condition=1; break;  // CPO
        case 0xec: if(get_flag(P)==1)   condition=1; break;  // CPE
        case 0xf4: if(get_flag(S)==0)   condition=1; break;  // CP       
        case 0xfc: if(get_flag(S)==1)   condition=1; break;  // CM
    }

    if(condition==1)
    {
        write(SP-1, (PC>>8));
        write(SP-2, (PC&0x00FF));
        SP -= 2;      
        
        PC = (byte3<<8)|byte2;

        cycles = 17;
        return 0;
    }
    else
    {
        cycles = 11;
        return 0;
    }
}

// Instruction: Complement Accumulator
uint8_t i8080::CMA()
{
    A ^= 0xFF;

    cycles = 4;
    return 0;
}

// Instruction: Complement carry
uint8_t i8080::CMC()
{
    status ^= CY;

    cycles = 4;
    return 0;
}

// Instruction: Compare register
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::CMPr()
{
    // Carry/Borrow - Bit 0
    set_flag(CY, A < *r);

    uint8_t temp_val = A - *r;

    // Parity - Bit 2
    set_flag(P, parity(temp_val));
    // Zero - Bit 6
    set_flag(Z, temp_val==0);
    // Sign - Bit 7
    set_flag(S, ((temp_val&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: Compare memory
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::CMPM()
{
    // Carry/Borrow - Bit 0
    set_flag(CY, A < rp_val);

    uint8_t temp_val = A - rp_val;

    // Parity - Bit 2
    set_flag(P, parity(temp_val));
    // Zero - Bit 6
    set_flag(Z, temp_val==0);
    // Sign - Bit 7
    set_flag(S, ((temp_val&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: Compare immediate
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::CPI()
{
    uint8_t r=A;

    // Carry/Borrow - Bit 0
    set_flag(CY, r < byte2);

    r -= byte2;

    // Parity - Bit 2
    set_flag(P, parity(r));
    // Zero - Bit 6
    set_flag(Z, r==0);
    // Sign - Bit 7
    set_flag(S, ((r&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: Add register pair to H and L
// Affected flags: Z, S, P, CY ,AC
uint8_t i8080::DAA()
{
    // Does nothing for now

    cycles = 4;
    return 0;
}

// Instruction: Add register pair to H and L
// Affected flags: CY
uint8_t i8080::DAD()
{
    uint16_t rp_sum = 0x0000;
    if (opcode==0x39) // DAD SP
    {
        rp_sum = SP;       
    }
    else
    {
        rp_sum = (*rh<<8) | *rl;  
    }
    
    uint16_t HL_sum = (H<<8) | L;
    uint32_t temp_sum = rp_sum + HL_sum;
    
    H = temp_sum >> 8;
    L = temp_sum & 0x00FF;

    set_flag(CY, (temp_sum & 0xFFFF0000) > 0);

    cycles = 10;
    return 0;
}

// Instruction: Decrement register
// Affected flags: Z, S, P, AC, CY
uint8_t i8080::DCR()
{
    *r -= 1;

    // Carry/Borrow - Bit 0
    set_flag(CY, *r==0xFF);
    // Parity - Bit 2
    set_flag(P, *r%2==0);
    // Zero - Bit 6
    set_flag(Z, *r==0);
    // Sign - Bit 7
    set_flag(S, ((*r&0x80) > 0));

    cycles = 5;
    return 0;
}

// Instruction: Decrement memory
uint8_t i8080::DCRM()
{
    uint16_t temp_rp = (H<<8) | L;
    temp_rp--;

    H = (temp_rp & 0xFF00) >> 8;
    L = (temp_rp & 0x00FF);

    // Parity - Bit 2
    set_flag(P, parity(temp_rp));
    // AC 
    //set_flag(AC, 0); 
    // Zero - Bit 6
    set_flag(Z, temp_rp==0);
    // Sign - Bit 7
    set_flag(S, ((temp_rp&0x80) > 0));


    cycles = 10;
    return 0;
}

// Instruction: Decrement register pair
uint8_t i8080::DCX()
{
    uint16_t temp_rp = 0x0000;
    
    if (opcode==0x3b) // DCX SP
    {
        SP--;
        temp_rp = SP;
    }
    else
    {
        temp_rp = (*rh<<8) | *rl;

        temp_rp--;

        *rh = (temp_rp & 0xFF00) >> 8;
        *rl = (temp_rp & 0x00FF);
    }

    // Parity - Bit 2
    set_flag(P, parity(temp_rp));
    // AC 
    //set_flag(AC, 0); 
    // Zero - Bit 6
    set_flag(Z, temp_rp==0);
    // Sign - Bit 7
    set_flag(S, ((temp_rp&0x80) > 0));


    cycles = 10;
    return 0;
}

// Instruction: Enable interuppts
uint8_t i8080::EI()
{
    interupts_enabled = 1;

    cycles = 4;
    return 0;
}

// Instruction: Increment register
uint8_t i8080::INR()
{
    *r += 1;

    // Parity - Bit 2
    set_flag(P, parity(*r));
    // AC 
    //set_flag(AC, 0); 
    // Zero - Bit 6
    set_flag(Z, *r==0);
    // Sign - Bit 7
    set_flag(S, ((*r&0x80) > 0));


    cycles = 5;
    return 0;
}

// Instruction: Increment memory
uint8_t i8080::INRM()
{
    uint32_t temp_rp = (H<<8) | L;
    temp_rp++;

    H = (temp_rp & 0xFF00) >> 8;
    L = (temp_rp & 0x00FF);

    // Parity - Bit 2
    set_flag(P, parity(temp_rp));
    // AC 
    //set_flag(AC, 0); 
    // Zero - Bit 6
    set_flag(Z, temp_rp==0);
    // Sign - Bit 7
    set_flag(S, ((temp_rp&0x80) > 0));


    cycles = 10;
    return 0;
}

// Instruction: Increment register pair
uint8_t i8080::INX()
{
    uint16_t temp_pair = 0x0000;

    if (opcode==0x33) // INX SP
    {
        SP++;
        temp_pair = SP;
    }
    else
    {
        temp_pair = (*rh<<8) | *rl;

        temp_pair++;

        *rh = temp_pair >> 8;
        *rl = temp_pair & 0x00FF;
    }

    cycles = 5;
    return 0;
}

// Instruction: Jump to immediate dress
uint8_t i8080::JMP()
{  
    uint16_t addr = (byte3<<8) | byte2;

    switch(opcode)
    {
        // Conventional jump
        case 0xc3: PC = addr; break;

        // Conditional jumps
        case 0xc2: if(get_flag(Z)==0) PC = addr; break;   // JNZ  
        case 0xca: if(get_flag(Z))  PC = addr; break;   // JZ
        case 0xd2: if(get_flag(CY)==0) PC = addr; break;  // JNC
        case 0xda: if(get_flag(CY)) PC = addr; break;   // JC
        case 0xe2: if(get_flag(P)==0) PC = addr; break;   // JPO        
        case 0xea: if(get_flag(P))  PC = addr; break;   // JPE
        case 0xf2: if(get_flag(S)==0) PC = addr; break;   // JP
        case 0xfa: if(get_flag(S))  PC = addr; break;   // JM
     }

    cycles = 10;
    return 0;
}

// Instruction: Load accumulator direct
uint8_t i8080::LDA()
{
    // The register pair is derived in the addressing mode
    A = addr_val;

    cycles = 13;
    return 0;
}

// Instruction: Load accumulator indirect
uint8_t i8080::LDAX()
{
    // The register pair is derived in the addressing mode
    A = rp_val;

    cycles = 7;
    return 0;
}

// Instruction: Load H and L direct
uint8_t i8080::LHLD()
{
    uint16_t data_address = (byte3<<8) | byte2;
    L = read(data_address);
    H = read(data_address + 1);

    cycles = 16;
    return 0;
}

// Instruction: Load register pair immediate
uint8_t i8080::LXI()
{   
    // LXI uses a register pair or 16-bit register, depending on the opcode
    switch(opcode)
    {
        // RP cases
        case 0x01: C = byte2; B = byte3; break;
        case 0x11: E = byte2; D = byte3; break;
        case 0x21: L = byte2; H = byte3; break;

        // 16-bit register cases
        case 0x31: SP = (byte3<<8)|byte2; break;
    }

    cycles = 10;
    return 0;
}

// Instruction: Move register to register
uint8_t i8080::MOV()
{
    *r1=*r2;

    cycles = 7;
}

// Instruction: Move to/from memory
uint8_t i8080::MOVM()
{
    switch(opcode)
    {
        // When moving to memory
        case 0x70: write(rp_addr, B); break;
        case 0x71: write(rp_addr, C); break;
        case 0x72: write(rp_addr, D); break;
        case 0x73: write(rp_addr, E); break;
        case 0x74: write(rp_addr, H); break;
        case 0x75: write(rp_addr, L); break;        
        case 0x77: write(rp_addr, A); break;

        // When moving from memory
        case 0x46: B=rp_val; break;
        case 0x4e: C=rp_val; break;
        case 0x56: D=rp_val; break;
        case 0x5e: E=rp_val; break;
        case 0x66: H=rp_val; break;
        case 0x6e: L=rp_val; break;
        case 0x7e: A=rp_val; break;
    }

    cycles = 7;
}

// Instruction: Move to register immediate
uint8_t i8080::MVI()
{
    switch(opcode)
    {
        case 0x06: B=byte2; break;
        case 0x0e: C=byte2; break;
        case 0x16: D=byte2; break;
        case 0x1e: E=byte2; break;
        case 0x26: H=byte2; break;
        case 0x2e: L=byte2; break;
        case 0x3e: A=byte2; break;
    }

    cycles = 7;
    return 0;
}

// Instruction: Move to memory immediate
uint8_t i8080::MVIM()
{ 
    write(rp_addr, byte2);

    cycles = 10;
    return 0;
}

// Instruction: No operation
uint8_t i8080::NOP()
{
    cycles = 4;
    return 0;
}

// Instruction: OR register
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ORAr()
{
    A |= *r;

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // AC 
    set_flag(AC, 0);
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: OR memory
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ORAM()
{
    A |= rp_val;

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // AC 
    set_flag(AC, 0);
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: OR immediate
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::ORI()
{
    A |= byte2;

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // AC 
    set_flag(AC, 0);
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}


// Instruction: Output to port
uint8_t i8080::OUT()
{ 
    // This must be commented for now, as there is no port
    // implementation *port is just a nullptr.
    //*port = A;

    cycles = 10;
    return 0;
}

// Instruction: Jump H and L indirect
uint8_t i8080::PCHL()
{
    uint16_t data_address = (byte3<<8) | byte2;
    PC = data_address;

    cycles = 5;
    return 0;
}

// Instruction: Pop Register pair
uint8_t i8080::POP()
{  
    *rh = read(SP+1);
    *rl = read(SP);

    SP += 2;
    
    cycles = 10;
    return 0;  
}

// Instruction: Pop processor status word
uint8_t i8080::POPpsw()
{  
    uint8_t data = read(SP);
    status = data & 0b11010101;

    A = read((SP+1));
    SP += 2;
    
    cycles = 10;
    return 0;  
}

// Instruction: Push Register pair
uint8_t i8080::PUSHrp()
{  
    write(SP-1, *rh);
    write(SP-2, *rl);
    SP -= 2;
    
    cycles = 11;
    return 0;  
}

// Instruction: Subtract register with burrow
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::SBBr()
{
    uint8_t carry = 0;
    if(get_flag(CY)==1) carry=1;

    // Carry/Borrow - Bit 0
    set_flag(CY, ((*r+carry) > A));

    A = A - *r - carry;

    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: Subtract memory with burrow
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::SBBM()
{
    uint8_t carry = 0;
    if(get_flag(CY)==1) carry=1;

    // Carry/Borrow - Bit 0
    set_flag(CY, ((rp_val+carry) > A));

    A = A - rp_val - carry;

    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: Subtract immediate with burrow
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::SBI()
{
    uint8_t carry = 0;
    if(get_flag(CY)==1) carry=1;

    // Carry/Borrow - Bit 0
    set_flag(CY, ((byte2+carry) > A));

    A = A - byte2 - carry;

    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: Store H and L direct
uint8_t i8080::SHLD()
{
    uint16_t data_address = (byte3<<8) | byte2;
    write(data_address, L);
    write(data_address+1, H);

    cycles = 16;
    return 0;
}

// Instruction: Store H and L direct
uint8_t i8080::SPHL()
{
    uint16_t temp_addr = (*rh<<8) | *rl;
    SP = temp_addr;

    cycles = 5;
    return 0;
}

// Instruction: Store accumulator direct
uint8_t i8080::STA()
{
    // The register pair is derived in the addressing mode
    write(dir_addr, A);

    cycles = 13;
    return 0;
}

// Instruction: Store accumulator indirect
uint8_t i8080::STAX()
{
    // The register pair is derived in the addressing mode
    write(rp_addr, A);

    cycles = 7;
    return 0;
}

// Instruction: Set carry
uint8_t i8080::STC()
{
    set_flag(CY, 1);

    cycles = 4;
    return 0;
}

// Instruction: Subtract register
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::SUBr()
{
     // Carry/Borrow - Bit 0
    set_flag(CY, (*r > A));
 
    A = A - *r;

    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: Subtract memory
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::SUBM()
{
     // Carry/Borrow - Bit 0
    set_flag(CY, (rp_val > A));
 
    A = A - rp_val;

    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: Subtract immediate
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::SUI()
{
     // Carry/Borrow - Bit 0
    set_flag(CY, (byte2 > A));
 
    A = A - byte2;

    // Parity - Bit 2
    set_flag(P, parity(A));
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: Rotate left through carry
// Affected flags: CY
// A7 moved to CY, CY to A0
// All other digits left shifted
uint8_t i8080::RAL()
{ 
    // Copy A7 for use in the CY and A7 bits
    uint8_t temp_A7 = A&0x80;
    uint8_t temp_CY = get_flag(CY);

    A = (A<<1);

    set_flag(CY, (temp_A7 > 0));
    A |= temp_CY;

    cycles = 4;
    return 0;
}

// Instruction: Rotate right through carry
// Affected flags: CY
// A0 moved to CY, CY to A7
// All other digits right shifted
uint8_t i8080::RAR()
{ 
    // Copy A7 for use in the CY and A7 bits
    uint8_t temp_A0 = A&0x80;
    uint8_t temp_CY = get_flag(CY);

    A = (A>>1);

    set_flag(CY, (temp_A0 > 0));
    A |= (temp_CY<<7);

    cycles = 4;
    return 0;
}

// Instruction: Return conditional
uint8_t i8080::Rc()
{
    bool condition = 0;
    switch(opcode)
    {
        case 0xc0: if(get_flag(Z)==0)   condition=1; break;  // RNZ
        case 0xc8: if(get_flag(Z)==1)   condition=1; break;  // RZ              
        case 0xd0: if(get_flag(CY)==0)  condition=1; break;  // RNC
        case 0xd8: if(get_flag(CY)==1)  condition=1; break;  // RC
        case 0xe0: if(get_flag(P)==0)   condition=1; break;  // RPO
        case 0xe8: if(get_flag(P)==1)   condition=1; break;  // RPE
        case 0xf0: if(get_flag(S)==0)   condition=1; break;  // RP
        case 0xf8: if(get_flag(S)==1)   condition=1; break;  // RM 
    }

    if(condition==1)
    {
        PC = rp_val;
        SP += 2;
        
        cycles = 11;
        return 0;
    }    
    else
    {
        cycles = 5;
        return 0;
    }
}

// Instruction: Return
uint8_t i8080::RET()
{
    PC = rp_val;
    SP += 2;
    
    cycles = 10;
    return 0;
}

// Instruction: Rotate left with carry
// Affected flags: CY
// A7 moved to A0, A7 to CY
// All other digits left shifted
uint8_t i8080::RLC()
{ 
    // Copy A7 for use in the CY and A7 bits
    uint8_t temp_A7 = A&0x80;

    set_flag(CY, (temp_A7 > 1));

    A = (A<<1);

    // Move old A7 to A0
    A = A|(temp_A7>>7);

    cycles = 4;
    return 0;
}

// Instruction: Rotate right with carry
// Affected flags: CY
// A0 moved to A7, A0 to CY
// All other digits right shifted
uint8_t i8080::RRC()
{ 
    // Copy A0 for use in the CY and A7 bits
    uint8_t temp_A0 = A&0x01;

    set_flag(CY, (temp_A0 > 0));

    A = (A>>1);

    // Move old A0 to A7
    A = A|(temp_A0<<7);

    cycles = 4;
    return 0;
}

// Instruction: Exchange H and L with D and E
uint8_t i8080::XCHG()
{
    uint8_t temp_H = H;
    uint8_t temp_L = L;

    H=D;
    L=E;
    D=temp_H;
    E=temp_L;

    cycles = 4;
    return 0;
}

// Instruction: XOR register
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::XRAr()
{
    A = A^(*r);

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, A%2==0);
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 4;
    return 0;
}

// Instruction: XOR memory
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::XRAM()
{
    A = A^(rp_val);

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, A%2==0);
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));

    cycles = 7;
    return 0;
}

// Instruction: XOR immediate
// Affected flags: Z, S, P, CY, AC
uint8_t i8080::XRI()
{
    A ^= byte2;

    // Carry/Borrow - Bit 0
    set_flag(CY, 0);
    // Parity - Bit 2
    set_flag(P, parity(A));
    // AC 
    set_flag(AC, 0);
    // Zero - Bit 6
    set_flag(Z, A==0);
    // Sign - Bit 7
    set_flag(S, ((A&0x80) > 0));
    //

    cycles = 7;
    return 0;
}

// Instruction: Exchange stack top with H and L
uint8_t i8080::XTHL()
{
    uint8_t temp_H = H;
    uint8_t temp_L = L;

    H = read((SP+1));
    L = read(SP);

    write(SP+1, temp_H);
    write(SP, temp_L);

    cycles = 18;
    return 0;
}

// Instruction: Not implemented
uint8_t i8080::NotImplemented()
{
    cycles = 1;
    return 0;
}