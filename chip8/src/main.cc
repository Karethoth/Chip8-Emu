#include "chip8.hh"
#include <iostream>
#include <SDL2/SDL.h>

#pragma comment( lib, "sdl2.lib" )
#undef main

using namespace std;
using namespace chip8emu;


void HandleInput();


CPU chip8;


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

	SDL_Renderer *renderer = SDL_CreateRenderer( window, -1, 0 );
	SDL_Texture  *texture  = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STATIC,
		displayWidth,
		displayHeight
	);

	try
	{
		chip8.LoadProgram( "test.ch8" );

		while( true )
		{
			HandleInput();
			//chip8.PrintInfo();
			//chip8.PrintStack();
			//getc( stdin );
			chip8.StepCycle();

			SDL_UpdateTexture( texture, nullptr, chip8.vram, displayWidth * sizeof( int ) );
			SDL_RenderClear( renderer );
			SDL_RenderCopy( renderer, texture, nullptr, nullptr );
			SDL_RenderPresent( renderer );
		}
	}

	catch( string &e )
	{
		cout << "Ran to error '" << e << "', exiting.." << endl;
	}

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

