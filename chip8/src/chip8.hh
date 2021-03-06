#pragma once

#include "chip8_instructions.hh"
#include <string>
#include <thread>
#include <atomic>


namespace chip8emu
{
	// Typedefs
	typedef unsigned char   u8;
	typedef unsigned short u16;
	typedef unsigned int   u32;


	// Constants
	const u8  stackSize     = 16;
	const u16 totalRam      = 0x1000;
	const u16 programStart  = 0x200;
	const u16 hexDigsStart  = 0x100;

	const u8  displayWidth  = 64;
	const u8  displayHeight = 32;

	const size_t cycleLength = 20; // Microseconds


	// Helper Functions
	template <typename T>
	bool GetBit( T src, u8 offset )
	{
		return static_cast<bool>( static_cast<u8>( src >> offset ) & 1 );
	}

	template <typename T>
	u8 GetNibble( T src, u8 offset )
	{
		return static_cast<u8>( src >> offset ) & 0xF;
	}


	// The Chip-8 Structure
	struct Chip8
	{
		// General Purpose Registers
		u8 V[16];

		// Special Registers
		u16 I;                       // Address Register
		u16 PC;                      // Program Counter
		u8  SP;                      // Stack Pointer

		// Atomic Special Registers
		std::atomic<u8>  Key;        // Key Register (for input)
		std::atomic<u8>  Time, Tone; // Time and Tone Registers

		// Memory
		u16 stack[stackSize];        // Stack
		u8  ram[totalRam];           // System RAM

		// Video RAM
		u32  vram[displayHeight * displayWidth];

		// Flag for timer thread
		std::atomic<bool> killTimer;


		// Constructor
		Chip8();

		// Methods
		bool LoadProgram( const std::string &path );
		void StepCycle();

		void StartTimerThread();
		void StopTimerThread();


	  private:
		u16  ReadInstruction( u16 addr ) const;
		void ExecInstruction( u16 instr );
		bool Match( u16 instr, Chip8Instruction pattern, u16 mask );
		void Draw( u8 x, u8 y, u16 addr, u8 len );

		std::thread timerThread;
	};
};

