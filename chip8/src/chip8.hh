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
	const u16 programStart  = 0x200;
	const u8  displayWidth  = 64;
	const u8  displayHeight = 32;


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

	// The CPU Structure
	struct CPU
	{
		// General Purpose Registers
		u8 V[16];

		// Special Registers
		u16 I;                       // Address Register
		u16 PC;                      // Program Counter
		u8  SP;                      // Stack Pointer

		// Atomic Special Registers
		std::atomic<u16> Key;        // Key Register (for input)
		std::atomic<u8>  Time, Tone; // Time and Tone Registers

		// Memory
		u16 stack[16];               // Stack
		u8  ram[0x1000];             // System RAM

		// Video RAM
		u32  vram[displayHeight * displayWidth];

		// Flag for timer thread
		std::atomic<bool> killTimer;

		// Constructor
		CPU();

		// Methods
		void LoadProgram( const std::string &path );
		void StepCycle();
		void PrintInfo();
		void PrintStack();

		void StartTimerThread();
		void StopTimerThread();


	  private:
		u16  ReadInstruction( u16 addr ) const;
		void ExecInstruction( u16 instr );
		bool Match( u16 instr, Chip8Instruction pattern, u16 mask );
		void Draw( u8 x, u8 y, u8 n );

		std::thread       timerThread;
	};
};

