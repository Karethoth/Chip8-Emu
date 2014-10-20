#pragma once

namespace Chip8Emu
{
	// Typedefs
	typedef unsigned char   u8;
	typedef unsigned short u16;


	// Constants
	const u16 programStart = 0x200;


	// Helper Functions
	template <typename T>
	bool GetBit( T src, u8 offset )
	{
		return static_cast<bool>( static_cast<u8>( src >> offset ) & 1 );
	}


	// The CPU Structure
	struct CPU
	{
		// General Purpose Registers
		u8 V0, V1, V2, V3, V4, V5, V6, V7,
		   V8, V9, VA, VB, VC, VD, VE, VF;

		// Special registers
		u16 I;               // Address Register
		u8  Time, Tone;      // Time and Tone Registers
		u16 PC;              // Program Counter
		u8  SP;              // Stack Pointer
		u16 Key;             // Key Register (for input)

		// Memory
		u16 stack[16];       // Stack
		u8  ram[0x1000];     // System RAM

	};
};
