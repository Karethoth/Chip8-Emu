#include "chip8.hh"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <random>
#include <SDL2/SDL.h>

using namespace std;
using namespace chip8emu;



// Random number generator
random_device rd;
static mt19937 gen( rd() );
static uniform_int_distribution<> dis( 0, 255 );


// The loop for the timer thread
void _TimerLoop( Chip8 *Chip8 );



Chip8::Chip8() :
	I(0), Time(0), Tone(0),
	PC(0x200), SP(0), Key(0),
	killTimer(false)
{
	// Clear ram, vram and stack
	memset( ram, 0, totalRam );
	memset( vram, 0, sizeof( vram ) );
	memset( stack, 0, stackSize*2 );

	// Binary sprites for hexadecimal digits
	static u8 hexDigits[] =
	{
		0xf0, 0x90, 0x90, 0x90, 0xf0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xf0, 0x10, 0xf0, 0x80, 0xf0, // 2
		0xf0, 0x10, 0xf0, 0x10, 0xf0, // 3
		0x90, 0x90, 0xf0, 0x10, 0x10, // 4
		0xf0, 0x80, 0xf0, 0x10, 0xf0, // 5
		0xf0, 0x80, 0xf0, 0x90, 0xf0, // 6
		0xf0, 0x10, 0x20, 0x40, 0x40, // 7
		0xf0, 0x90, 0xf0, 0x90, 0xf0, // 8
		0xf0, 0x90, 0xf0, 0x10, 0xf0, // 9
		0xf0, 0x90, 0xf0, 0x90, 0x90, // A
		0xe0, 0x90, 0xe0, 0x90, 0xe0, // B
		0xf0, 0x80, 0x80, 0x80, 0xf0, // C
		0xe0, 0x90, 0x90, 0x90, 0xe0, // D
		0xf0, 0x80, 0xf0, 0x80, 0xf0, // E
		0xf0, 0x80, 0xf0, 0x80, 0x80  // F
	};


	// Copy the digits to the interpreter section
	for( int i = 0; i < sizeof( hexDigits ); i++ )
	{
		ram[hexDigsStart + i ] = hexDigits[i];
	}
}


bool Chip8::LoadProgram( const string &path )
{
	// Open the program file
	ifstream inp( path, ios::in | ios::binary );
	if( !inp )
	{
		cout << "Failed to open '" << path << "'!" << endl;
		return false;
	}

	// Load the program in to the memory
	u16 memOffset = 0;
	bool tooBigFile = false;

	while( inp.good()  )
	{
		// Check that we're still within the memory
		if( (memOffset < (totalRam - programStart)) )
		{
			tooBigFile = true;
			break;
		}

		// Load the byte
		auto c = static_cast<u8>( inp.get() );
		ram[programStart + memOffset] = c;
		memOffset++;
	}

	inp.close();


	// Check that the file wasn't too big
	if( tooBigFile )
	{
		cout << "Too big file, couldn't fit it to ram!'" << endl;
		return false;
	}

	return true;
}


void Chip8::StepCycle()
{
	auto instr = ReadInstruction( PC );
	ExecInstruction( instr );
}


u16 Chip8::ReadInstruction( u16 addr ) const
{
	if( addr >= totalRam - 1 )
	{
		cout << "Trying to read instruction from "
		     << hex << addr << "! Aborting..." << endl;

		throw string( "Bad read" );
	}

	// Return
	return static_cast<u16>( ram[addr] << 8 ) |
	       static_cast<u16>( ram[addr+1] );
}


void Chip8::ExecInstruction( u16 instr )
{
	// Try to find the match for instruction and
	// execute it accordingly.

	if( !instr )
	{
		// Null instruction is handled as a NOP
	}

	// 00E0 - CLS
	else if( Match( instr, CLS, 0xFFFF ) )
	{
		memset( vram, 0, displayHeight * displayWidth * sizeof( u32 ) );
	}

	// 00EE - RET
	else if( Match( instr, RET, 0xFFFF ) )
	{
		if( SP <= 0x0 )
		{
			throw string( "Stack underflow!" );
		}

		PC = stack[SP];
		SP--;
	}

	// 0nnn - SYS addr
	else if( !(instr & 0xF000) && instr )
	{
		// Ignored, handled as NOP
	}

	// 1nnn - JP addr
	else if( Match( instr, JP, 0xF000 ) )
	{
		PC = instr & 0x0FFF;
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
		stack[SP] = PC;

		PC = instr & 0x0FFF;
		return;
	}

	// 3xkk - SE Vx, byte
	else if( Match( instr, SE, 0xF000 ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto kk = static_cast<u8>( instr & 0x00FF );

		if( V[x] == kk )
		{
			PC += 2;
		}
	}

	// 4xkk - SNE Vx, byte
	else if( Match( instr, SNE, 0xF000 ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto kk = static_cast<u8>( instr & 0x00FF );

		if( V[x] != kk )
		{
			PC += 2;
		}
	}

	// 5xy0 - SE Vx, Vy
	else if( Match( instr, SE2, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		if( V[x] == V[y] )
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

	// 7xkk - ADD Vx, byte
	else if( Match( instr, ADD, 0xF000) )
	{
		auto x = GetNibble<>( instr, 8 );
		V[x] += static_cast<u8>( instr & 0x00FF );
	}

	// 8xy0 - LD Vx, Vy
	else if( Match( instr, STRG2, 0xF00F) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		V[x] = V[y];
	}

	// 8xy1 - OR Vx, Vy
	else if( Match( instr, OR, 0xF00F) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		V[x] |= V[y];
	}

	// 8xy2 - AND Vx, Vy
	else if( Match( instr, AND, 0xF00F) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		V[x] &= V[y];
	}

	// 8xy3 - XOR Vx, Vy
	else if( Match( instr, XOR, 0xF00F) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		V[x] ^= V[y];
	}

	// 8xy4 - ADD Vx, Vy
	else if( Match( instr, ADD2, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		u16 sum = V[x] + V[y];
		V[0xF]  = sum > 0xFF;
		V[x]    = static_cast<u8>( sum & 0xFF );
	}

	// 8xy5 - SUB Vx, Vy
	else if( Match( instr, SUB, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		V[0xF] = V[x] > V[y];
		V[x]  -= V[y];
	}

	// 8xy6 - SHR Vx {, Vy}
	else if( Match( instr, SHR, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );

		V[0xF] = V[x] & 0x1;
		V[x]  /= 2;
	}

	// 8xy7 - SUBN Vx, Vy
	else if( Match( instr, SUBN, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		V[0xF] = static_cast<u8>( V[y] > V[x] );
		V[x]   = V[y] - V[x];
	}

	// 8xyE - SHL Vx {, Vy}
	else if( Match( instr, SHL, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );

		V[0xF] = V[x] >> 7;
		V[x]  *= 2;
	}

	// 9xy0 - SNE Vx, Vy
	else if( Match( instr, SNE2, 0xF00F ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		if( V[x] != V[y] )
		{
			PC += 2;
		}
	}

	// Annn - LD I, addr
	else if( Match( instr, LDI, 0xF000 ) )
	{
		I = instr & 0x0FFF;
	}

	// Bnnn - JP V0, addr
	else if( Match( instr, JP2, 0xF000 ) )
	{
		PC = (instr + V[0]) & 0x0FFF;
	}

	// Cxkk - RND Vx, byte
	else if( Match( instr, RND, 0xF000 ) )
	{
		auto x  = GetNibble<>( instr, 8 );
		auto kk = static_cast<u8>( dis( gen ) & instr & 0x00FF );

		V[x] = kk;
	}

	// Dxyn - DRW Vx, Vy, nibble
	else if( Match( instr, DRW, 0xF000 ) )
	{
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );
		auto n = GetNibble<>( instr, 0 );

		Draw( V[x], V[y], I, n );
	}

	// Ex9E - SKP Vx
	else if( Match( instr, SKP, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );

		if( Key == x )
		{
			PC += 2;
		}
	}

	// ExA1 - SKNP Vx
	else if( Match( instr, SKNP, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );

		if( Key != x )
		{
			PC += 2;
		}
	}

	// Fx07 - LD Vx, DT
	else if( Match( instr, LDDT, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );
		V[x]   = Time;
	}

	// Fx0A - LD Vx, K
	else if( Match( instr, LDKEY, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );

		// Wait for input, and frequently check that
		// we are not quiting/killing the other thread
		u8 inpKey;

		while( !(inpKey = Key) && !killTimer )
		{
			SDL_PumpEvents(); // Refresh SDL events
			this_thread::sleep_for( chrono::milliseconds( 1 ) );
		}

		V[x] = inpKey;
	}

	// Fx15 - LD DT, Vx
	else if( Match( instr, STDT, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );
		Time = V[x];
	}

	// Fx18 - LD ST, Vx
	else if( Match( instr, STST, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );
		Tone = V[x];
	}

	// Fx1E - ADD I, Vx
	else if( Match( instr, ADD3, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );
		I += V[x];
	}

	// Fx29 - LD F, Vx
	else if( Match( instr, LDSP, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );

		// Set I to point to the location of
		// hex sprite corresponding to x.
		// (The sprites are 5 bytes in size)
		I = hexDigsStart + x * 5;
	}

	// Fx33 - LD B, Vx
	else if( Match( instr, STBCD, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );

		// Check we have enough memory
		// to save 3 bytes to [I+2].
		if( I+2 > totalRam )
		{
			throw string( "RAM overflow!" );
		}

		// Split x to hundreds, tens and ones
		ram[I]   = static_cast<u8>( x / 100 );
		ram[I+1] = static_cast<u8>( (x % 100) / 10 );
		ram[I+2] = static_cast<u8>( (x % 10) );
	}

	// Fx55 - LD [I], Vx
	else if( Match( instr, PSHRGS, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );

		u16 offset = 0;
		for( u8 i = 0; i <= x; i++ )
		{
			ram[I+offset++] = V[i];
		}
	}

	// Fx65 - LD [I], Vx
	else if( Match( instr, POPRGS, 0xF0FF ) )
	{
		auto x = GetNibble<>( instr, 8 );

		u16 offset = 0;
		for( u8 i = 0; i <= x; i++ )
		{
			V[i] = ram[I+offset];
			offset++;
		}
	}

	// Unknown instruction
	else
	{
		cout << "Trying to exec instruction " << hex << static_cast<int>( instr )
		     << " @ " << PC << " failed, it's not implemented!" << endl;

		throw string( "Unknown instruction" );
	}

	// Increment PC to point to the next instruction
	PC += 2;
}


void _TimerLoop( Chip8 *Chip8 );
void Chip8::StartTimerThread()
{
	killTimer = false;
	timerThread = thread( _TimerLoop, this );
}


void Chip8::StopTimerThread()
{
	killTimer = true;
	timerThread.join();
}


bool Chip8::Match( u16 instr, Chip8Instruction pattern, u16 mask )
{
	if( (instr & mask) == (pattern & mask) )
	{
		return true;
	}

	return false;
}


void Chip8::Draw( u8 x, u8 y, u16 addr, u8 len )
{
	if( addr >= totalRam )
	{
		throw string( "RAM overflow!" );
	}

	for( u8 i = 0; i < len; i++ )
	{
		u32 spriteLine = ram[I+i];
		u32 rowStart   = displayWidth * ((y + i) % displayHeight);

		for( u8 _x = 0; _x < 8; _x++ )
		{
			auto on   = GetBit( spriteLine, 7 - _x );
			auto xPos = (x + _x) % displayWidth;

			vram[rowStart + xPos] ^= on ? 0xffff : 0x0000;
		}
	}
}


void _TimerLoop( Chip8 *chip8 )
{
	while( chip8 && !chip8->killTimer )
	{
		if( chip8->Time > 0 )
		{
			chip8->Time--;
		}

		if( chip8->Tone > 0 )
		{
			chip8->Tone--;
		}

		this_thread::sleep_for( chrono::milliseconds( 16 ) );
	}
}

