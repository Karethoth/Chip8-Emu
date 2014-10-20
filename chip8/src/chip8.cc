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
		throw string( "Bad read" );
		return 0;
	}

	return static_cast<u16>( ram[addr] << 8 ) | static_cast<u16>( ram[addr+1] );
}


void CPU::ExecInstruction( u16 instr )
{
	cout << "Executing " << hex << static_cast<int>( instr ) << endl;
	if( !instr )
	{
		throw string( "Null Instruction!" );
	}

	// 1nnn - JP addr
	else if( Match( instr, JP ) && GetNibble( instr, 12 ) == 1 )
	{
		PC = static_cast<u16>( instr & 0x0FFF );
		cout << "Jumping to " << hex << static_cast<int>( PC ) << endl;
		return;
	}

	// 3xkk - SE Vx, byte
	else if( Match( instr, SE ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto kk = static_cast<u8>( instr & 0x00FF );
		if( x == kk )
		{
			PC += 2;
		}
	}

	// 6xkk - LD Vx, byte
	else if( Match( instr, STRG ) )
	{
		auto x = GetNibble<>( instr, 8 );
		V[x] = static_cast<u8>( instr & 0x00FF );
	}

	// 8xy0 - LD Vx, Vy
	else if( Match( instr, STRG2 ) && ((instr & 0x000F) == 0) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );
		V[x] = V[y];
	}

	// 8xy5 - SUB Vx, Vy
	else if( Match( instr, SUB ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		if( V[x] > V[y] )
			V[0xf] = 1;
		else
			V[0xf] = 0;

		V[x] -= V[y];
	}

	// Annn - LD I, addr
	else if( Match( instr, LDI ) )
	{
		I = static_cast<u16>( instr & 0x0FFF );
	}

	// Cxkk - RND Vx, byte
	else if( Match( instr, RND ) )
	{
		auto x  = GetNibble<>( instr, 8 );
		auto kk = static_cast<u8>( dis( gen ) & instr & 0x00FF );
		V[x] = kk;
	}

	// 0nnn - SYS addr
	else if( !(instr & 0xF000) && instr )
	{
		PC = static_cast<u16>( instr & 0x0FFF );
		cout << "Calling to " << hex << static_cast<int>( PC ) << endl;
		return;
	}

	else
	{
		cout << "Trying to exec instruction " << hex << static_cast<int>( instr ) << " @ " << PC
			 << " failed, it's not implemented!" << endl;

		getc( stdin );
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

