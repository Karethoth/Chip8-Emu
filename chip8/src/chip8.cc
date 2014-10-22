#include "chip8.hh"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <random>
#include <SDL2/SDL.h>

using namespace std;
using namespace chip8emu;

//#define DEBUG_MODE


random_device rd;
static mt19937 gen( rd() );
static uniform_int_distribution<> dis( 0, 255 );


CPU::CPU() :
	I(0), Time(0), Tone(0),
	PC(0x200), SP(0), Key(0),
	killTimer(false)
{
	memset( ram, 0, 0x1000 );
	memset( vram, 0, sizeof( vram ) );
	memset( stack, 0, 32 );
}


bool CPU::LoadProgram( const string &path )
{
	ifstream inp( path, ios::in | ios::binary );
	if( !inp )
	{
		cout << "Failed to open '" << path << "'!" << endl;
		return false;
	}

	u16 memOffset = 0;

	while( inp.good() && memOffset <  (totalRam - programStart) )
	{
		auto c = static_cast<u8>( inp.get() );
		ram[programStart + memOffset] = c;
		memOffset++;
	}

	inp.close();

	if( memOffset >= (totalRam - programStart) )
	{
		cout << "Too big file!'" << endl;
		return false;
	}

	return true;
}


void CPU::StepCycle()
{
	auto instr = ReadInstruction( PC );
	ExecInstruction( instr );
}


u16 CPU::ReadInstruction( u16 addr ) const
{
	if( addr >= totalRam )
	{
		cout << "Trying to access memory location " << hex << addr << "! Ignoring.." << endl;
		throw string( "Bad read" );
		return 0;
	}

	return static_cast<u16>( ram[addr] << 8 ) | static_cast<u16>( ram[addr+1] );
}


static void DebugPrint( const string &msg )
{
	#ifdef DEBUG_MODE
		cout << msg << endl;
	#endif
}


void CPU::ExecInstruction( u16 instr )
{
	#ifdef DEBUG_MODE
		cout << "Executing " << setw( 4 ) << setfill( '0' ) << hex
			 << static_cast<int>( instr ) << " @ " << static_cast<int>( PC ) << " : ";
	#endif

	if( !instr )
	{
		DebugPrint( "0000 - Null" );
		// Null instruction is handled as a NOP
		throw string( "Null Instruction" );
	}

	// 00E0 - CLS
	else if( Match( instr, CLS, 0xFFFF ) )
	{
		DebugPrint( "00E0 - Clear screen" );
		memset( vram, 0, displayHeight * displayWidth * sizeof( u32 ) );
	}

	// 00EE - RET
	else if( Match( instr, RET, 0xFFFF ) )
	{
		DebugPrint( "00EE - RET" );
		if( SP > 0xF )
		{
			throw string( "Stack overflow!" );
		}

		PC = stack[SP];
		SP--;
	}

	// 0nnn - SYS addr
	else if( !(instr & 0xF000) && instr )
	{
		// Ignored, handled as NOP
		DebugPrint( "0nnn - SYS/NOP" );
	}

	// 1nnn - JP addr
	else if( Match( instr, JP, 0xF000 ) )
	{
		DebugPrint( "1nnn - JP nnn" );
		PC = static_cast<u16>( instr & 0x0FFF );
		return;
	}

	// 2nnn - CALL addr
	else if( Match( instr, CALL, 0xF000 ) )
	{
		DebugPrint( "2nnn - CALL nnn" );
		if( SP > 0xF )
		{
			throw string( "Stack overflow!" );
		}

		SP++;
		stack[SP] = PC;

		PC = static_cast<u16>( instr & 0x0FFF );
		//cout << "Calling " << hex << static_cast<int>( PC ) << endl;
		return;
	}

	// 3xkk - SE Vx, byte
	else if( Match( instr, SE, 0xF000 ) )
	{
		DebugPrint( "3xkk - SE Vx, kk" );
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
		DebugPrint( "4xkk - SNE Vx, kk" );
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
		DebugPrint( "5xy0 - SE Vx, Vy" );
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
		DebugPrint( "6xkk - LD Vx, kk" );
		auto x = GetNibble<>( instr, 8 );
		V[x] = static_cast<u8>( instr & 0x00FF );
	}

	// 7xkk - ADD Vx, byte
	else if( Match( instr, ADD, 0xF000) )
	{
		DebugPrint( "7xkk - ADD Vx, kk" );
		auto x = GetNibble<>( instr, 8 );
		V[x] += static_cast<u8>( instr & 0x00FF );
	}

	// 8xy0 - LD Vx, Vy
	else if( Match( instr, STRG2, 0xF00F) )
	{
		DebugPrint( "8xy0 - LD Vx, Vy" );
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );
		V[x] = V[y];
	}

	// 8xy1 - OR Vx, Vy
	else if( Match( instr, OR, 0xF00F) )
	{
		DebugPrint( "8xy1 - OR Vx, Vy" );
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );
		V[x] |= V[y];
	}

	// 8xy2 - AND Vx, Vy
	else if( Match( instr, AND, 0xF00F) )
	{
		DebugPrint( "8xy2 - AND Vx, Vy" );
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );
		V[x] &= V[y];
	}

	// 8xy4 - ADD Vx, Vy
	else if( Match( instr, ADD2, 0xF00F ) )
	{
		DebugPrint( "8xy4 - ADD Vx, Vy" );
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		u16 sum = V[x] + V[y];
		V[0xF] = sum > 0xFF;

		V[x] = static_cast<u8>( sum & 0xFF );
	}

	// 8xy5 - SUB Vx, Vy
	else if( Match( instr, SUB, 0xF00F ) )
	{
		DebugPrint( "8xy5 - SUB Vx, Vy" );
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );

		if( V[x] > V[y] )
			V[0xF] = 1;
		else
			V[0xF] = 0;

		V[x] -= V[y];
	}

	// 8xy6 - SHR Vx {, Vy}
	else if( Match( instr, SHR, 0xF00F ) )
	{
		DebugPrint( "8xy6 - SHR Vx" );
		auto x = GetNibble<>( instr, 8 );
		V[0xF] = V[x] & 0x1;
		V[x] /= 2;
	}

	// 8xyE - SHL Vx {, Vy}
	else if( Match( instr, SHL, 0xF00F ) )
	{
		DebugPrint( "8xyE - SHL Vx" );
		auto x = GetNibble<>( instr, 8 );
		V[0xF] = V[x] >> 7;
		V[x] *= 2;
	}

	// 9xy0 - SNE Vx, Vy
	else if( Match( instr, SNE2, 0xF00F ) )
	{
		DebugPrint( "9xy0 - SNE Vx, Vy" );
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
		DebugPrint( "Annn - LD I, nnn" );
		I = instr & 0x0FFF;
	}

	// Cxkk - RND Vx, byte
	else if( Match( instr, RND, 0xF000 ) )
	{
		DebugPrint( "Cxkk - RND Vx, kk" );
		auto x  = GetNibble<>( instr, 8 );
		auto kk = static_cast<u8>( dis( gen ) & instr & 0x00FF );
		V[x] = kk;
	}

	// Dxyn - DRW Vx, Vy, nibble
	else if( Match( instr, DRW, 0xF000 ) )
	{
		DebugPrint( "Dxyn - DRW Vx, Vy, n" );
		auto x = GetNibble<>( instr, 8 );
		auto y = GetNibble<>( instr, 4 );
		auto n = GetNibble<>( instr, 0 );

		Draw( x, y, n );
	}

	// Fx07 - LD Vx, DT
	else if( Match( instr, LDDT, 0xF0FF ) )
	{
		DebugPrint( "Fx07 - LD Vx, DT" );
		auto x = GetNibble<>( instr, 8 );
		V[x] = Time;
	}

	// Fx0A - LD Vx, K
	else if( Match( instr, LDKEY, 0xF0FF ) )
	{
		DebugPrint( "Fx0A - LD Vx, K" );
		auto x = GetNibble<>( instr, 8 );

		// Wait for input, frequently check that we
		// are not quiting/killing the other thread
		u8 inpKey;
		while( !(inpKey = Key) && !killTimer )
		{
			SDL_PumpEvents();
			this_thread::sleep_for( chrono::milliseconds( 1 ) );
		}
		V[x] = inpKey;
	}

	// Fx15 - LD DT, Vx
	else if( Match( instr, STDT, 0xF0FF ) )
	{
		DebugPrint( "Fx15 - LD DT, Vx" );
		auto x = GetNibble<>( instr, 8 );
		Time = V[x];
	}

	// Fx18 - LD ST, Vx
	else if( Match( instr, STST, 0xF0FF ) )
	{
		DebugPrint( "Fx18 - LD DT, Vx" );
		auto x = GetNibble<>( instr, 8 );
		Tone = V[x];
	}

	// Fx1E - ADD I, Vx
	else if( Match( instr, ADD3, 0xF0FF ) )
	{
		DebugPrint( "FX1E - ADD I, Vx" );
		auto x = GetNibble<>( instr, 8 );
		I += V[x];
	}

	// Fx55 - LD [I], Vx
	else if( Match( instr, PSHRGS, 0xF0FF ) )
	{
		DebugPrint( "FX55 - PUSH [I], Vx" );
		u16 offset = 0;
		auto x = GetNibble<>( instr, 8 );

		for( u8 i=0; i <= x; i++ )
		{
			ram[I+offset++] = V[i];
		}
	}

	// Fx65 - LD [I], Vx
	else if( Match( instr, POPRGS, 0xF0FF ) )
	{
		DebugPrint( "FX65 - POP [I], Vx" );
		u16 offset = 0;
		auto x = GetNibble<>( instr, 8 );


		for( u8 i=0; i <= x; i++ )
		{
			//cout << "MEMREAD FROM " << hex << static_cast<int>( I+offset ) << endl;
			V[i] = ram[I+offset];
			offset++;
		}
	}

	else
	{
		DebugPrint( "Unknown" );
		cout << "Trying to exec instruction " << hex << static_cast<int>( instr ) << " @ " << PC
			 << " failed, it's not implemented!" << endl;

		getc( stdin );
	}


	// Increment PC to point to the next instruction
	PC += 2;
}


void CPU::PrintInfo() const
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


void CPU::PrintStack() const
{
	cout << endl << "Stack:" << endl;

	for( int i=0; i <= SP; i++ )
	{
		cout << "\t" << hex << i << ":" << setfill('0') << setw(4) << static_cast<int>( stack[i] ) << endl;
	}
	cout << endl << endl;
}


void _TimerLoop( CPU *cpu );
void CPU::StartTimerThread()
{
	timerThread = thread( _TimerLoop, this );
}


void CPU::StopTimerThread()
{
	killTimer = true;
	timerThread.join();
}


bool CPU::Match( u16 instr, Chip8Instruction pattern, u16 mask )
{
	if( (instr & mask) == (pattern & mask) )
	{
		return true;
	}

	return false;
}


void CPU::Draw( u8 x, u8 y, u8 n )
{
	u8 posX = V[x];
	u8 posY = V[y];

	if( I >= totalRam )
	{
		throw string( "RAM overflow!" );
	}

	for( u8 i=0; i < n; i++ )
	{
		u32 spriteLine = ram[I+i];
		u32 pos = posY*displayWidth + i*displayWidth + posX;

		if( pos > displayWidth * displayHeight )
		{
			throw string( "VRAM overflow!" );
		}

		for( u8 _x=0; _x < 8; _x++ )
		{
			auto on = GetBit( spriteLine, 7-_x );
			vram[pos + _x] ^= on ? 0xffff : 0x0000;
		}
	}
}


void _TimerLoop( CPU *cpu )
{
	while( cpu && !cpu->killTimer )
	{
		if( cpu->Time > 0 )
		{
			cpu->Time--;
		}

		if( cpu->Tone > 0 )
		{
			cpu->Tone--;
		}

		this_thread::sleep_for( chrono::milliseconds( 16 ) );
	}
}

