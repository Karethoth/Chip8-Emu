#include "chip8.hh"
#include <iostream>

using namespace std;
using namespace chip8emu;


int main()
{
	try
	{
		CPU chip8;

		chip8.LoadProgram( "test.ch8" );

		while( true )
		{
			//chip8.PrintInfo();
			//chip8.PrintStack();
			//getc( stdin );
			chip8.StepCycle();
		}
	}

	catch( string &e )
	{
		cout << "Ran to error '" << e << "', exiting.." << endl;
	}

	cout << endl << "Hit enter to quit." << endl;
	getc( stdin );
	return 0;
}
