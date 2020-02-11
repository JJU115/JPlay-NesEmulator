#include <unordered_map>
#include <string>


std::unordered_map<uint8_t, std::string> OPCODES (
    {
    //Implicit
    {0x00, "BRK"}, {0x18, "CLC"}, {0xD8, "CLD"}, {0x58, "CLI"}, {0xB8, "CLV"},
    {0xCA, "DEX"}, {0x88, "DEY"}, 

    //Accumulator
    {0x0A, "ASL"}

    //Immediate
    {0x69, "ADC"}, {0x29, "AND"}, {0xC9, "CMP"}, {0xE0, "CPX"}, {0xC0, "CPY"},
    {0x49, "EOR"}, 

    //Zero Page
    {0x65, "ADC"}, {0x25, "AND"}, {0x06, "ASL"}, {0x24, "BIT"}, {0xC5, "CMP"},
    {0xE4, "CPX"}, {0xC4, "CPY"}, {0xC6, "DEC"}, {0x45, "EOR"}, {0xE6, "INC"},

    //Zero Page X
    {0x75, "ADC"}, {0x35, "AND"}, {0x16, "ASL"}, {0xD5, "CMP"}, {0xD6, "DEC"},
    {0x55, "EOR"}, {0xF6, "INC"},

    //Zero Page Y

    //Relative
    {0x90, "BCC"}, {0xB0, "BCS"}, {0xF0, "BEQ"}, {0x30, "BMI"}, {0xD0, "BNE"},
    {0x10, "BPL"}, {0x50, "BVC"}, {0x70, "BVS"},

    //Absolute
    {0x6D, "ADC"}, {0x2D, "AND"}, {0x0E, "ASL"}, {0x2C, "BIT"}, {0xCD, "CMP"},
    {0xEC, "CPX"}, {0xCC, "CPY"}, {0xCE, "DEC"}, {0x4D, "EOR"}, {0xEE, "INC"},

    //Absolute X
    {0x7D, "ADC"}, {0x3D, "AND"}, {0x1E, "ASL"}, {0xDD, "CMP"}, {0xDE, "DEC"},
    {0x5D, "EOR"}, {0xFE, "INC"},

    //Absolute Y
    {0x79, "ADC"}, {0x39, "AND"}, {0xD9, "CMP"}, {0x59, "EOR"}, 

    //Indirect

    //Indexed Indirect
    {0x61, "ADC"}, {0x21, "AND"}, {0xC1, "CMP"}, {0x41, "EOR"}

    //Indirect Indexed
    {0x71, "ADC"}, {0x31, "AND"}, {0xD1, "CMP"}, {0x51, "EOR"}

    }

);