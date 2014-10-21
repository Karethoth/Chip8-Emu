#include "chip8.hh"
#include <iostream>
#include <SDL2/SDL.h>

#pragma comment( lib, "sdl2.lib" )
#undef main

using namespace std;
using namespace chip8emu;


void HandleInput();


CPU chip8;
bool shouldStop = false;


int main( int argc, char **argv )
{
	// Setup the screen
    SDL_Init( SDL_INIT_VIDEO );
    auto window = SDL_CreateWindow(
		"Chip-8 Emulator",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		displayWidth * 8,
		displayHeight * 8,
		0
    );

	// Create the renderer and texture for the actual display
	SDL_Renderer *renderer = SDL_CreateRenderer( window, -1, 0 );
	SDL_Texture  *texture  = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STATIC,
		displayWidth,
		displayHeight
	);


	// Load the program
	if( !chip8.LoadProgram( "test.ch8" ) )
	{
		shouldStop = true;
	}


	// Start the timer thread, which
	// decrements timers when appropriate
	chip8.StartTimerThread();


	try
	{
		// The main loop
		while( !shouldStop )
		{
			// Calculate when this cycle is supposed to end
			auto cycleEnd = chrono::high_resolution_clock::now() + chrono::microseconds( cycleLength );

			// Handle all input as appropriate
			HandleInput();

			// Run the current instruction
			chip8.StepCycle();

			// Update the SDL screen
			SDL_UpdateTexture( texture, nullptr, chip8.vram, displayWidth * sizeof( int ) );
			SDL_RenderClear( renderer );
			SDL_RenderCopy( renderer, texture, nullptr, nullptr );
			SDL_RenderPresent( renderer );

			// Sleep for the excess cycle time
			this_thread::sleep_until( cycleEnd );
		}
	}

	catch( string &e )
	{
		cout << "Ran to error '" << e << "', exiting.." << endl;
	}

	chip8.StopTimerThread();

	SDL_DestroyTexture( texture );
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();

	cout << endl << "Hit enter to quit." << endl;
	getc( stdin );
	return 0;
}


void HandleInput()
{
	SDL_Event event;
	while( SDL_PollEvent( &event ) )
	{
	}
}

