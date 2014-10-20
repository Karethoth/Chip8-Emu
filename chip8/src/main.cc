#include "chip8.hh"
#include <iostream>

using namespace std;
using namespace chip8emu;


int main()
{
	CPU chip8;

	chip8.LoadProgram( "test.ch8" );

	while( chip8.ram[chip8.PC] )
	{
		chip8.StepCycle();
	}

	cout << endl << "Hit enter to quit." << endl;
	getc( stdin );
	return 0;
}
