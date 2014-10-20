#pragma once

namespace Chip8Emu
{
	typedef unsigned char   u8;
	typedef unsigned short u16;

	template <typename T>
	bool GetBit( T src, u8 offset )
	{
		return static_cast<bool>( static_cast<u8>( src >> offset ) & 1 );
	}


	struct CPU
	{
		// General purpose registers
		u8 V0, V1, V2, V3, V4, V5, V6, V7,
		   V8, V9, VA, VB, VC, VD, VE, VF;
	};
};
