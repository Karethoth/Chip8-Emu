#pragma once

namespace chip8emu
{
	enum Chip8Instruction : unsigned char
	{
		SYS = 0x0000,
		CLS = 0x00E0,
		RET = 0x00EE,

		JP    = 0x1000,
		CALL  = 0x2000,
		SE    = 0x3000,
		SNE   = 0x4000,
		SE2   = 0x5000,
		STRG  = 0x6000,
		ADD   = 0x7000,

		STRG2 = 0x8000,
		OR    = 0x8001,
		AND   = 0x8002,
		XOR   = 0x8003,
		ADD2  = 0x8004,
		SUB   = 0x8005,
		SHR   = 0x8006,
		SUBN  = 0x8007,
		SHL   = 0x800E,

		SNE2 = 0x9000,
		LDI  = 0xA000,
		JP2  = 0xB000,
		RND  = 0xC000,
		DRW  = 0xD000,

		SKP    = 0xE09E,
		SKNP   = 0xE0A1,
		LDDT   = 0xF007,
		LDKEY  = 0xF00A,
		STDT   = 0xF015,
		STST   = 0xF018,
		ADD3   = 0xF01E,
		LDSP   = 0xF029,
		STBCD  = 0xF033,
		PSHRGS = 0xF055,
		POPRGS = 0xF065
	};
};

