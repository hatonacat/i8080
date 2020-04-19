#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#define CPUDIAG

// Forward declaration of the bus class
// only use as a pointer so minimises #include usage
class Bus;

class i8080
{
    public:
        i8080();
        ~i8080();

    public:
        bool clock();
        void connect_bus(Bus *new_bus);
        uint8_t get_cpu_reg(uint8_t r);

    public:
        // Bus
        Bus *bus = nullptr;
        
        // Registers
        uint8_t B = 0x00;
        uint8_t C = 0x00;
        uint8_t D = 0x00;
        uint8_t E = 0x00;
        uint8_t H = 0x00;
        uint8_t L = 0x00;
        uint8_t A = 0x00;
        uint8_t status = 0x00;  // The 'flag register' status (F)
        uint16_t PC = 0x0000;
        uint16_t SP = 0x0000;
        
        // Array of pointers to registers, currently only
        // used for fault finding print functions
        std::map<char, std::uint8_t *> reg_map = {
            {'B', &B}, {'C', &C}, {'D', &D}, {'E', &E}, 
            {'H', &H}, {'L', &L}, {'A', &A}, {'F', &status}
        };

        // Condition codes/flags. Pg22.
        enum FLAGS8080
        {
            CY  = (1 << 0),
            P   = (1 << 2),
            AC  = (1 << 4),
            Z   = (1 << 6),
            S   = (1 << 7)    
        };

    private:
        // Bus related instructions
        uint8_t read(uint16_t addr);
        void    write(uint16_t addr, uint8_t data);

        // Private CPU related functions
        void    flagcheck(std::vector<std::string> flags, uint8_t result); // Not currently used
        void    set_flag(FLAGS8080 f, bool set);
        bool    get_flag(FLAGS8080 f);
        bool    parity(uint8_t val);

        // Internal print instructions
        void print_CPU_detail();

        // Emulation variables
        uint32_t clock_count = 0;       // Total accumulated clock functions
        uint16_t op_count = 0;          // Total number of operations that have occured  
        uint8_t cycles = 0;             // The cycles required for a given instruction
        uint8_t opcode = 0x00;          // Hexadecimal opcode reference
        uint8_t port_placeholder = 0x00;// Placeholder for future port resolving
        uint16_t PC_previous = PC;      // A temporary work around for printing PC info
        bool stopped = 0;               // Return signal when a not implemented opcode is found
        bool interupts_enabled = 0;

#ifdef CPUDIAG
        uint16_t error_addr = 0x0000;   // Used in CPU DIAG
        char error_char; 
#endif

        // Addressing mode variables

        // Used for direct mode addressing
        uint8_t * port = 0x00;
        uint8_t addr_val = 0x00;
        uint16_t dir_addr = 0x0000;

        // Used for Immediate mode addressing
        uint8_t byte2 = 0x00;
        uint8_t byte3 = 0x00;

        // Used for register indirect addressing.
        uint16_t rp_addr = 0x0000;
        uint16_t rp_val = 0x0000;

        // Used for register direct address mode
        uint8_t * r;    // Register (8-bit immediate ops)
        uint8_t * r1;   // Register (8-bit register to register ops)
        uint8_t * r2;   // Register (8-bit register to register ops)
        uint8_t * rl;   // Register low (16-bit memory ops)
        uint8_t * rh;   // Register high (16-bit memory ops)

        // Data struction for opcode instructions
        struct Instruction
        {
            std::string alias;
            uint8_t (i8080::*operation)(void) = nullptr;
            uint8_t (i8080::*addrmode)(void) = nullptr;
        };

        // A forward declaration use for extraction of instructions
        // from the instruction table below
        Instruction instruction;

        // Map containing opcode details.
        using a = i8080;
        const std::unordered_map<uint8_t, Instruction> instructions
        {
            {0x00, {"NOP",      &a::NOP,    &a::IMP}},
            {0x01, {"LXI BC",   &a::LXI,    &a::IM16}},
            {0x02, {"STAX BC",  &a::STAX,   &a::RGI8r}},
            {0x03, {"INX BC",   &a::INX,    &a::RGD}},
            {0x04, {"INR B",    &a::INR,    &a::RGD}},              
            {0x05, {"DCR B",    &a::DCR,    &a::RGD}},
            {0x06, {"MVI B,d",  &a::MVI,    &a::IM8}},
            {0x07, {"RLC",      &a::RLC,    &a::IMP}},          
            {0x09, {"DAD B",    &a::DAD,    &a::RGD}},
            {0x0a, {"LDAX BE",  &a::LDAX,   &a::RGI8r}},             
            {0x0b, {"DCX BC",   &a::DCX,    &a::RGD}},           
            {0x0c, {"INR C",    &a::INR,    &a::RGD}},                         
            {0x0d, {"DCR C",    &a::DCR,    &a::RGD}},            
            {0x0e, {"MVI C,d",  &a::MVI,    &a::IM8}},
            {0x0f, {"RRC",      &a::RRC,    &a::IMP}},
            {0x11, {"LXI DE",   &a::LXI,    &a::IM16}}, 
            {0x12, {"STAX DE",  &a::STAX,   &a::RGI8r}},            
            {0x13, {"INX DE",   &a::INX,    &a::RGD}},
            {0x14, {"INR D",    &a::INR,    &a::RGD}},                         
            {0x15, {"DCR D",    &a::DCR,    &a::RGD}},             
            {0x16, {"MVI D,d",  &a::MVI,    &a::IM8}},           
            {0x17, {"RAL",      &a::RAL,    &a::IMP}},           
            {0x19, {"DAD D",    &a::DAD,    &a::RGD}},
            {0x1a, {"LDAX D",   &a::LDAX,   &a::RGI8r}}, 
            {0x1b, {"DCX DE",   &a::DCX,    &a::RGD}},               
            {0x1c, {"INR E",    &a::INR,    &a::RGD}},                         
            {0x1d, {"DCR E",    &a::DCR,    &a::RGD}},             
            {0x1e, {"MVI E,d",  &a::MVI,    &a::IM8}},             
            {0x1f, {"RAR",      &a::RAR,    &a::IMP}},                        
            {0x21, {"LXI HL",   &a::LXI,    &a::IM16}},
            {0x22, {"SHLD",     &a::SHLD,   &a::IM16}},            
            {0x23, {"INX HL",   &a::INX,    &a::RGD}},  
            {0x24, {"INR H",    &a::INR,    &a::RGD}},                         
            {0x25, {"DCR H",    &a::DCR,    &a::RGD}},                         
            {0x26, {"MVI H,d",  &a::MVI,    &a::IM8}},
            {0x27, {"DAA",      &a::DAA,    &a::IMP}},                  
            {0x29, {"DAD HL",   &a::DAD,    &a::RGD}},
            {0x2a, {"LHLD",     &a::LHLD,   &a::IM16}},
            {0x2b, {"DCX HL",   &a::DCX,    &a::RGD}}, 
            {0x2c, {"INR L",    &a::INR,    &a::RGD}},                         
            {0x2d, {"DCR L",    &a::DCR,    &a::RGD}},  
            {0x2e, {"MVI L,d",  &a::MVI,    &a::IM8}}, 
            {0x2f, {"CMA",      &a::CMA,    &a::IMP}}, 
            {0x31, {"LXI SP",   &a::LXI,    &a::IM16}},
            {0x32, {"STA adr",  &a::STA,    &a::DIR}},  
            {0x33, {"INX SP",   &a::INX,    &a::RGD}},             
            {0x34, {"INR M",    &a::INRM,   &a::IMP}}, // RGI, but implemented as IMP
            {0x35, {"DCR M",    &a::DCRM,   &a::IMP}}, // RGI, but implemented as IMP
            {0x36, {"MVI M,d",  &a::MVIM,   &a::IMRI}},
            {0x37, {"STC",      &a::STC,    &a::IMP}},                      
            {0x39, {"DAD SP",   &a::DAD,    &a::RGD}},            
            {0x3a, {"LDA adr",  &a::LDA,    &a::DIR}}, 
            {0x3b, {"DCX SP",   &a::DCX,    &a::RGD}},             
            {0x3c, {"INR A",    &a::INR,    &a::RGD}},                                   
            {0x3d, {"DCR A",    &a::DCR,    &a::RGD}},              
            {0x3e, {"MVI A,d",  &a::MVI,    &a::IM8}}, 
            {0x3f, {"CMC",      &a::CMC,    &a::IMP}},             
            {0x40, {"MOV B,B",  &a::MOV,    &a::RGD}},               
            {0x41, {"MOV B,C",  &a::MOV,    &a::RGD}},
            {0x42, {"MOV B,D",  &a::MOV,    &a::RGD}},
            {0x43, {"MOV B,E",  &a::MOV,    &a::RGD}},
            {0x44, {"MOV B,H",  &a::MOV,    &a::RGD}},
            {0x45, {"MOV B,L",  &a::MOV,    &a::RGD}},
            {0x46, {"MOV B,M",  &a::MOVM,   &a::RGI8M}}, 
            {0x47, {"MOV B,A",  &a::MOV,    &a::RGD}},               
            {0x48, {"MOV C,B",  &a::MOV,    &a::RGD}},             
            {0x49, {"MOV C,C",  &a::MOV,    &a::RGD}},                      
            {0x4a, {"MOV C,D",  &a::MOV,    &a::RGD}},             
            {0x4b, {"MOV C,E",  &a::MOV,    &a::RGD}},               
            {0x4c, {"MOV C,H",  &a::MOV,    &a::RGD}},             
            {0x4d, {"MOV C,L",  &a::MOV,    &a::RGD}},               
            {0x4e, {"MOV C,M",  &a::MOVM,   &a::RGI8M}},             
            {0x4f, {"MOV C,A",  &a::MOV,    &a::RGD}},    
            {0x50, {"MOV D,B",  &a::MOV,    &a::RGD}},
            {0x51, {"MOV D,C",  &a::MOV,    &a::RGD}},
            {0x52, {"MOV D,D",  &a::MOV,    &a::RGD}},
            {0x53, {"MOV D,E",  &a::MOV,    &a::RGD}},
            {0x54, {"MOV D,H",  &a::MOV,    &a::RGD}},
            {0x55, {"MOV D,L",  &a::MOV,    &a::RGD}},
            {0x56, {"MOV D,M",  &a::MOVM,   &a::RGI8M}},            
            {0x57, {"MOV D,A",  &a::MOV,    &a::RGD}},       
            {0x58, {"MOV E,B",  &a::MOV,    &a::RGD}},
            {0x59, {"MOV E,C",  &a::MOV,    &a::RGD}},
            {0x5a, {"MOV E,D",  &a::MOV,    &a::RGD}},
            {0x5b, {"MOV E,E",  &a::MOV,    &a::RGD}},
            {0x5c, {"MOV E,H",  &a::MOV,    &a::RGD}},
            {0x5d, {"MOV E,L",  &a::MOV,    &a::RGD}},
            {0x5e, {"MOV E,M",  &a::MOVM,   &a::RGI8M}},
            {0x5f, {"MOV E,A",  &a::MOV,    &a::RGD}},
            {0x60, {"MOV H,b",  &a::MOV,    &a::RGD}},           
            {0x61, {"MOV H,C",  &a::MOV,    &a::RGD}},
            {0x62, {"MOV H,D",  &a::MOV,    &a::RGD}},
            {0x63, {"MOV H,E",  &a::MOV,    &a::RGD}},
            {0x64, {"MOV H,H",  &a::MOV,    &a::RGD}},
            {0x65, {"MOV H,L",  &a::MOV,    &a::RGD}},
            {0x66, {"MOV H,M",  &a::MOVM,   &a::RGI8M}},
            {0x67, {"MOV H,A",  &a::MOV,    &a::RGD}},       
            {0x68, {"MOV L,B",  &a::MOV,    &a::RGD}},
            {0x69, {"MOV L,C",  &a::MOV,    &a::RGD}},
            {0x6a, {"MOV L,D",  &a::MOV,    &a::RGD}},
            {0x6b, {"MOV L,E",  &a::MOV,    &a::RGD}},
            {0x6c, {"MOV L,H",  &a::MOV,    &a::RGD}},
            {0x6d, {"MOV L,L",  &a::MOV,    &a::RGD}},
            {0x6e, {"MOV L,M",  &a::MOVM,   &a::RGI8M}},
            {0x6f, {"MOV L,A",  &a::MOV,    &a::RGD}},
            {0x70, {"MOV M,B",  &a::MOVM,   &a::RGI8M}},              
            {0x71, {"MOV M,C",  &a::MOVM,   &a::RGI8M}},  
            {0x72, {"MOV M,D",  &a::MOVM,   &a::RGI8M}}, 
            {0x73, {"MOV M,E",  &a::MOVM,   &a::RGI8M}}, 
            {0x74, {"MOV M,H",  &a::MOVM,   &a::RGI8M}}, 
            {0x75, {"MOV M,L",  &a::MOVM,   &a::RGI8M}}, 
            {0x77, {"MOV M,A",  &a::MOVM,   &a::RGI8M}},            
            {0x78, {"MOV A,B",  &a::MOV,    &a::RGD}},
            {0x79, {"MOV A,C",  &a::MOV,    &a::RGD}},
            {0x7a, {"MOV A,D",  &a::MOV,    &a::RGD}},
            {0x7b, {"MOV A,E",  &a::MOV,    &a::RGD}},            
            {0x7c, {"MOV A,H",  &a::MOV,    &a::RGD}},
            {0x7d, {"MOV A,L",  &a::MOV,    &a::RGD}},            
            {0x7e, {"MOV A,M",  &a::MOVM,   &a::RGI8M}},
            {0x7f, {"MOV A,A",  &a::MOV,    &a::RGD}},             
            {0x80, {"ADD B",    &a::ADDr,   &a::RGD}},            
            {0x81, {"ADD C",    &a::ADDr,   &a::RGD}},
            {0x82, {"ADD D",    &a::ADDr,   &a::RGD}},
            {0x83, {"ADD E",    &a::ADDr,   &a::RGD}},
            {0x84, {"ADD H",    &a::ADDr,   &a::RGD}},
            {0x85, {"ADD L",    &a::ADDr,   &a::RGD}},
            {0x86, {"ADD M",    &a::ADDM,   &a::RGI8M}},            
            {0x87, {"ADD A",    &a::ADDr,   &a::RGD}},
            {0x88, {"ADC B",    &a::ADCr,   &a::RGD}},            
            {0x89, {"ADC C",    &a::ADCr,   &a::RGD}},
            {0x8a, {"ADC D",    &a::ADCr,   &a::RGD}},
            {0x8b, {"ADC E",    &a::ADCr,   &a::RGD}},
            {0x8c, {"ADC H",    &a::ADCr,   &a::RGD}},
            {0x8d, {"ADC L",    &a::ADCr,   &a::RGD}},
            {0x8e, {"ADC M",    &a::ADCM,   &a::RGI8M}},   
            {0x8f, {"ADC A",    &a::ADCr,   &a::RGD}},       
            {0x90, {"SUB B",    &a::SUBr,   &a::RGD}},                      
            {0x91, {"SUB C",    &a::SUBr,   &a::RGD}},
            {0x92, {"SUB D",    &a::SUBr,   &a::RGD}},
            {0x93, {"SUB E",    &a::SUBr,   &a::RGD}},
            {0x94, {"SUB H",    &a::SUBr,   &a::RGD}},         
            {0x95, {"SUB L",    &a::SUBr,   &a::RGD}},
            {0x96, {"SUB M",    &a::SUBM,   &a::RGI8M}},           
            {0x97, {"SUB A",    &a::SUBr,   &a::RGD}},            
            {0x98, {"SBB B",    &a::SBBr,   &a::RGD}},            
            {0x99, {"SBB C",    &a::SBBr,   &a::RGD}},
            {0x9a, {"SBB D",    &a::SBBr,   &a::RGD}},
            {0x9b, {"SBB E",    &a::SBBr,   &a::RGD}},
            {0x9c, {"SBB H",    &a::SBBr,   &a::RGD}},
            {0x9d, {"SBB L",    &a::SBBr,   &a::RGD}},
            {0x9e, {"SBB M",    &a::SBBM,   &a::RGI8M}},   
            {0x9f, {"SBB A",    &a::SBBr,   &a::RGD}},                      
            {0xa0, {"ANA B",    &a::ANAr,   &a::RGD}},
            {0xa1, {"ANA C",    &a::ANAr,   &a::RGD}},
            {0xa2, {"ANA D",    &a::ANAr,   &a::RGD}},
            {0xa3, {"ANA E",    &a::ANAr,   &a::RGD}},
            {0xa4, {"ANA H",    &a::ANAr,   &a::RGD}},
            {0xa5, {"ANA L",    &a::ANAr,   &a::RGD}},
            {0xa6, {"ANA M",    &a::ANAM,   &a::RGI8M}},  
            {0xa7, {"ANA A",    &a::ANAr,   &a::RGD}},
            {0xa8, {"XRA B",    &a::XRAr,   &a::RGD}},            
            {0xa9, {"XRA C",    &a::XRAr,   &a::RGD}},
            {0xaa, {"XRA D",    &a::XRAr,   &a::RGD}},
            {0xab, {"XRA E",    &a::XRAr,   &a::RGD}},
            {0xac, {"XRA H",    &a::XRAr,   &a::RGD}},
            {0xad, {"XRA L",    &a::XRAr,   &a::RGD}},         
            {0xae, {"XRA M",    &a::XRAM,   &a::RGI8M}},  
            {0xaf, {"XRA A",    &a::XRAr,   &a::RGD}},
            {0xb0, {"ORA B",    &a::ORAr,   &a::RGD}},
            {0xb1, {"ORA C",    &a::ORAr,   &a::RGD}},
            {0xb2, {"ORA D",    &a::ORAr,   &a::RGD}},
            {0xb3, {"ORA E",    &a::ORAr,   &a::RGD}},
            {0xb4, {"ORA H",    &a::ORAr,   &a::RGD}},
            {0xb5, {"ORA L",    &a::ORAr,   &a::RGD}},
            {0xb6, {"ORA M",    &a::ORAM,   &a::RGI8M}},
            {0xb7, {"ORA A",    &a::ORAr,   &a::RGD}},
            {0xb8, {"CMP B",    &a::CMPr,   &a::RGD}},            
            {0xb9, {"CMP C",    &a::CMPr,   &a::RGD}},
            {0xba, {"CMP D",    &a::CMPr,   &a::RGD}},
            {0xbb, {"CMP E",    &a::CMPr,   &a::RGD}},
            {0xbc, {"CMP H",    &a::CMPr,   &a::RGD}},
            {0xbd, {"CMP L",    &a::CMPr,   &a::RGD}},         
            {0xbe, {"CMP M",    &a::CMPM,   &a::RGI8M}},              
            {0xbf, {"CMP A",    &a::CMPr,   &a::RGD}},     
            {0xc0, {"RNZ",      &a::Rc,     &a::RGI16}},             
            {0xc1, {"POP BC",   &a::POP,    &a::RGD}},  // This is listed as RGI, but is implemented as RGD
            {0xc2, {"JNZ",      &a::JMP,    &a::IM16}},            
            {0xc3, {"JMP",      &a::JMP,    &a::IM16}},
            {0xc4, {"CNZ",      &a::Cc,     &a::IM16}},                 
            {0xc5, {"PUSH B",   &a::PUSHrp, &a::RGD}},  // This is listed as RGI, but is implemented as RGD
            {0xc6, {"ADI d",    &a::ADI,    &a::IM8}},          
            {0xc8, {"RZ",       &a::Rc,     &a::RGI16}}, 
            {0xc9, {"RET",      &a::RET,    &a::RGI16}},
            {0xca, {"JZ",       &a::JMP,    &a::IM16}},        
            {0xcc, {"CZ",       &a::Cc,     &a::IM16}},               
            {0xcd, {"CALL",     &a::CALL,   &a::IM16}},
            {0xce, {"ACI d",    &a::ACI,    &a::IM8}},
            {0xd0, {"RNC",      &a::Rc,     &a::RGI16}},          
            {0xd1, {"POP DE",   &a::POP,    &a::RGD}},  // This is listed as RGI, but is implemented as RGD
            {0xd2, {"JNC",      &a::JMP,    &a::IM16}},
            {0xd3, {"OUT 6",    &a::OUT,    &a::DIR}}, 
            {0xd4, {"CNC",      &a::Cc,     &a::IM16}},             
            {0xd5, {"PUSH D",   &a::PUSHrp, &a::RGD}},  // This is listed as RGI, but is implemented as RGD
            {0xd6, {"SUI d",    &a::SUI,    &a::IM8}},           
            {0xd8, {"RC",       &a::Rc,     &a::RGI16}},             
            {0xda, {"JC",       &a::JMP,    &a::IM16}},          
            {0xdc, {"CC",       &a::Cc,     &a::IM16}},                        
            {0xde, {"SBI d",    &a::SBI,    &a::IM8}},            
            {0xe0, {"RPO",      &a::Rc,     &a::RGI16}},         
            {0xe1, {"POP HL",   &a::POP,    &a::RGD}},  // This is listed as RGI, but is implemented as RGD
            {0xe2, {"JPO",      &a::JMP,    &a::IM16}},               
            {0xe3, {"XTHL",     &a::XTHL,   &a::IMP}},  // Should be RGI, but implemented as IMP            
            {0xe4, {"CPO",      &a::Cc,     &a::IM16}},             
            {0xe5, {"PUSH HL",  &a::PUSHrp, &a::RGD}},  // This is listed as RGI, but is implemented as RGD
            {0xe6, {"ANI d",    &a::ANI,    &a::IM8}},            
            {0xe8, {"RPE",      &a::Rc,     &a::RGI16}},            
            {0xe9, {"PCHL",     &a::PCHL,   &a::RGD}},              
            {0xea, {"JPE",      &a::JMP,    &a::IM16}},         
            {0xeb, {"XCHG",     &a::XCHG,   &a::RGD}},
            {0xec, {"CPE",      &a::Cc,     &a::IM16}},             
            {0xee, {"XRI d",    &a::XRI,    &a::IM8}},                
            {0xf0, {"RP",       &a::Rc,     &a::RGI16}}, 
            {0xf1, {"POP PSW",  &a::POPpsw, &a::IMP}},  // Listed as RGI, but implemented as IMP
            {0xf2, {"JP",       &a::JMP,    &a::IM16}},
            {0xf4, {"CP",       &a::Cc,     &a::IM16}},             
            {0xf5, {"PSH PSW",  &a::PUSHrp, &a::RGD}},  // This is listed as RGI, but is implemented as RGD
            {0xf6, {"ORI d",    &a::ORI,    &a::IM8}},                
            {0xf8, {"RM",       &a::Rc,     &a::RGI16}}, 
            {0xf9, {"SPHL",     &a::SPHL,   &a::RGD}}, 
            {0xfa, {"JM adr",   &a::JMP,    &a::IM16}},
            {0xfb, {"EI",       &a::EI,     &a::IMP}},
            {0xfc, {"CM",       &a::Cc,     &a::IM16}},               
            {0xfe, {"CPI A,d",  &a::CPI,    &a::IM8}},

        };
        // The default instruction if the opcode isn't found
        Instruction not_implemented = {"...", &a::NotImplemented, &a::IMP};

    private:
        // Addressing modes
        uint8_t DIR();  // Direct
        uint8_t IM8();  // Immediate 8bit
        uint8_t IM16(); // Immediate 8bit
        uint8_t IMP();  // Implicit
        uint8_t IMRD(); // Immediate/register direct
        uint8_t IMRI(); // Immediate/register indirect
        uint8_t RGD();  // Register direct
        uint8_t RGI8r(); // Register indirect from memory
        uint8_t RGI8M(); // Register indirect to memory
        uint8_t RGI16();// Register indirect

        // Opcodes
        uint8_t ACI();
        uint8_t ADCr();
        uint8_t ADCM();
        uint8_t ADDr();
        uint8_t ADDM();
        uint8_t ADI();
        uint8_t ANAr();
        uint8_t ANAM();
        uint8_t ANI();
        uint8_t CALL();
        uint8_t Cc();
        uint8_t CMA();
        uint8_t CMC();
        uint8_t CMPr();
        uint8_t CMPM();
        uint8_t CPI();
        uint8_t DAA();
        uint8_t DAD();
        uint8_t DCR();
        uint8_t DCRM();
        uint8_t DCX();
        uint8_t EI();
        uint8_t INR();
        uint8_t INRM();
        uint8_t INX();
        uint8_t JMP();
        uint8_t JNZ();
        uint8_t LDA();
        uint8_t LDAX();
        uint8_t LHLD();
        uint8_t LXI();
        uint8_t MOV();
        uint8_t MOVM();
        uint8_t MVI();
        uint8_t MVIM();
        uint8_t NOP();
        uint8_t ORAr();
        uint8_t ORAM();
        uint8_t ORI();
        uint8_t OUT();
        uint8_t PCHL();
        uint8_t POP();
        uint8_t POPpsw();
        uint8_t PUSHrp();
        uint8_t Rc();
        uint8_t RAL();
        uint8_t RAR();
        uint8_t RET();
        uint8_t RLC();
        uint8_t RRC();
        uint8_t SBBr();
        uint8_t SBBM();
        uint8_t SBI();
        uint8_t SHLD();
        uint8_t SPHL();
        uint8_t STA();
        uint8_t STAX();
        uint8_t STC();
        uint8_t SUBr();
        uint8_t SUBM();
        uint8_t SUI();
        uint8_t XCHG();
        uint8_t XRAr();
        uint8_t XRAM();
        uint8_t XRI();
        uint8_t XTHL();
        uint8_t NotImplemented();
};