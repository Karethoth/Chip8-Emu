#include "chip8.hh"
#include <iostream>
#include <iomanip>
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

	// 0nnn - SYS addr
	else if( !(instr & 0xF000) && instr )
	{
		PC = static_cast<u16>( instr & 0x0FFF );
		cout << "Calling to " << hex << static_cast<int>( PC ) << endl;
		return;
	}

	// 1nnn - JP addr
	else if( Match( instr, JP, 0xF000 ) )
	{
		PC = static_cast<u16>( instr & 0x0FFF );
		cout << "Jumping to " << hex << static_cast<int>( PC ) << endl;
		return;
	}

	// 2nnn - CALL addr
	else if( Match( instr, CALL, 0xF000 ) )
	{
		if( SP > 0xF )
		{
			throw string( "Stack overflow!" );
		}

		SP++;
		stack[SP] = PC+2;

		PC = static_cast<u16>( instr & 0x0FFF );
		cout << "Calling " << hex << static_cast<int>( PC ) << endl;
		return;
	}

	// 3xkk - SE Vx, byte
	else if( Match( instr, SE, 0xF000 ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto kk = static_cast<u8>( instr & 0x00FF );
		if( x == kk )
		{
			PC += 2;
		}
	}

	// 5xy0 - SE Vx, Vy
	else if( Match( instr, SE2, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );
		if( x == y )
		{
			PC += 2;
		}
	}

	// 6xkk - LD Vx, byte
	else if( Match( instr, STRG, 0xF000 ) )
	{
		auto x = GetNibble<>( instr, 8 );
		V[x] = static_cast<u8>( instr & 0x00FF );
	}

	// 8xy0 - LD Vx, Vy
	else if( Match( instr, STRG2, 0xF00F) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );
		V[x] = V[y];
	}

	// 8xy4 - ADD Vx, Vy
	else if( Match( instr, ADD2, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		u16 sum = x + y;
		V[0xF] = sum > 0xFF;

		V[x] = sum;
	}

	// 8xy5 - SUB Vx, Vy
	else if( Match( instr, SUB, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		if( V[x] > V[y] )
			V[0xF] = 1;
		else
			V[0xF] = 0;

		V[x] -= V[y];
	}

	// Annn - LD I, addr
	else if( Match( instr, LDI, 0xF000 ) )
	{
		I = static_cast<u16>( instr & 0x0FFF );
	}

	// Cxkk - RND Vx, byte
	else if( Match( instr, RND, 0xF000 ) )
	{
		auto x  = GetNibble<>( instr, 8 );
		auto kk = static_cast<u8>( dis( gen ) & instr & 0x00FF );
		V[x] = kk;
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


void CPU::PrintInfo()
{
	cout << endl << "General purpose registers:" << endl;

	for( int i=0; i <= 0xF; i++ )
	{
		cout << "V" << hex << i << ":" << setfill('0') << setw(2) << static_cast<int>( V[i] ) << ", ";
		if( !((i+1)%8) )
		{
			cout << endl;
		}
	}
	cout << endl;

	cout << " I:" << setfill( '0' )  << setw( 4 ) << static_cast<int>( I ) << ", "
	     << "PC:" << setfill( '0' )  << setw( 4 ) << static_cast<int>( PC ) << ", "
	     << "SP:" << setfill( '0' )  << setw( 2 ) << static_cast<int>( SP ) << ", "
	     << "Ti:" << setfill( '0' )  << setw( 2 ) << static_cast<int>( Time ) << ", "
	     << "To:" << setfill( '0' )  << setw( 2 ) << static_cast<int>( Tone ) << ", "
	     << "Key:" << setfill( '0' ) << setw( 4 ) << static_cast<int>( Key ) << endl;

	cout << endl;
}


bool CPU::Match( u16 instr, Chip8Instruction pattern, u16 mask )
{
	if( (instr & mask) == (pattern & mask) )
	{
		return true;
	}

	return false;
}

