#include "chip8.hh"
#include <iostream>
#include <fstream>
#include <random>

using namespace std;
using namespace chip8emu;


random_device rd;
static mt19937 gen( rd() );
static uniform_int_distribution<> dis( 0, 255 );


CPU::CPU() :
	I(0), Time(0), Tone(0),
	PC(0x200), SP(0), Key(0)
{
	memset( ram, 0, 0x1000 );
	memset( stack, 0, 32 );
}


void CPU::LoadProgram( const string &path )
{
	ifstream inp( path );
	if( !inp )
	{
		cout << "Failed to open '" << path << "'!" << endl;
		return;
	}

	u16 offset = 0;

	while( inp.good() )
	{
		auto c = static_cast<u8>( inp.get() );
		ram[programStart + offset] = c;
		offset++;
	}

	inp.close();
}


void CPU::StepCycle()
{
	auto instr = ReadInstruction( PC );
	ExecInstruction( instr );
}


u16 CPU::ReadInstruction( u16 addr ) const
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
	if( !instr )
	{
	}

	// 6xkk - LD Vx, byte
	else if( Match( instr, STRG ) )
	{
		auto x = GetNibble<u16>( instr, 8 );
		V[x] = static_cast<u8>( instr & 0x00FF );
	}

	// Annn - LD I, addr
	else if( Match( instr, LDI ) )
	{
		I = static_cast<u16>( instr & 0x0FFF );
	}

	// Cxkk - RND Vx, byte
	else if( Match( instr, RND ) )
	{
		auto x = GetNibble<u16>( instr, 8 );
		auto r = static_cast<u8>( dis( gen ) & instr & 0x00FF );
		V[x] = r;
	}

	else
	{
		cout << "Trying to exec instruction " << hex << instr << " @ " << PC
			 << " failed, it's not implemented!" << endl;
	}


	// Increment PC to point to the next instruction
	PC += 2;
}


bool CPU::Match( u16 instr, Chip8Instruction pattern )
{
	u8 patternHigh = static_cast<u8>( (pattern & 0xFF00) >> 8 );
	u8 patternLow  = static_cast<u8>( pattern & 0x00FF );
	u8 instrHigh = static_cast<u8>( (instr & 0xFF00) >> 8 );
	u8 instrLow  = static_cast<u8>( instr & 0x00FF );

	if( ((instrHigh & patternHigh) == patternHigh) &&
	    ((instrLow & patternLow)   == patternLow) )
	{
		return true;
	}
	return false;
}

