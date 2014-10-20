#include "chip8.hh"
#include <iostream>

using namespace std;
using namespace chip8emu;


CPU::CPU() :
	I(0), Time(0), Tone(0),
	PC(0x200), SP(0), Key(0)
{
}


void CPU::StepCycle()
{
	auto instr = ReadInstruction( PC );
	ExecInstruction( instr );

}


u16 CPU::ReadInstruction( u16 addr )
{
	if( addr > 0xFFF )
	{
		cout << "Trying to access memory location " << hex << addr << "! Ignoring.." << endl;
		return 0;
	}

	return static_cast<u16>( ram[addr] << 8 ) | static_cast<u16>( ram[addr+1] );
}


void CPU::ExecInstruction( u16 instr )
{
	cout << "Trying to exec instruction " << hex << instr << " failed, it's not implemented!" << endl;
}

