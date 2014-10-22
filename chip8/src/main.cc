#include "chip8.hh"
#include <iostream>
#include <SDL2/SDL.h>

#pragma comment( lib, "sdl2.lib" )
#undef main

using namespace std;
using namespace chip8emu;


void HandleSdlEventsTask();


Chip8 chip8;
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

	// Start the timer thread, which decrements
	// timers when appropriate (At 60Hz)
	chip8.StartTimerThread();

	// Launch a thread to handle sdl events
	thread eventThread( HandleSdlEventsTask );

	try
	{
		// The main loop
		while( !shouldStop )
		{
			// Calculate when this cycle is supposed to end
			auto cycleEnd = chrono::high_resolution_clock::now() + chrono::microseconds( cycleLength );

			// Pump SDL Events
			SDL_PumpEvents();

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

	// Wait for the timer thread to stop
	chip8.StopTimerThread();

	// Wait for the event handler thread to stop
	eventThread.join();

	// Cleanup SDL resources
	SDL_DestroyTexture( texture );
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();

	// Wait for user input to quit
	cout << endl << "Hit enter to quit." << endl;
	getc( stdin );
	return 0;
}


void HandleInput( const SDL_KeyboardEvent &inp  )
{
	// Check for escape key
	if( inp.keysym.scancode == SDL_SCANCODE_ESCAPE )
	{
		shouldStop = true;
		return;
	}

	// Just clear the chip input if any button is released
	if( inp.state == SDL_RELEASED )
	{
		chip8.Key = 0;
		return;
	}

	// Handle the possible emulator input
	switch( inp.keysym.sym )
	{

	 case SDLK_5:
	 case SDLK_UP:
	 case SDLK_KP_5:
		chip8.Key = 5;
		break;

	 case SDLK_8:
	 case SDLK_DOWN:
	 case SDLK_KP_2:
		chip8.Key = 8;
		break;

	 case SDLK_7:
	 case SDLK_LEFT:
	 case SDLK_KP_1:
		chip8.Key = 7;
		break;

	 case SDLK_9:
	 case SDLK_RIGHT:
	 case SDLK_KP_3:
		chip8.Key = 9;
		break;

	 case SDLK_0:
	 case SDLK_KP_0:
		chip8.Key = 0;
		break;

	 case SDLK_1:
	 case SDLK_KP_7:
		chip8.Key = 1;
		break;

	 case SDLK_2:
	 case SDLK_KP_8:
		chip8.Key = 2;
		break;

	 case SDLK_3:
	 case SDLK_KP_9:
		chip8.Key = 3;
		break;

	 case SDLK_4:
	 case SDLK_KP_4:
		chip8.Key = 4;
		break;

	 case SDLK_6:
	 case SDLK_KP_6:
		chip8.Key = 6;
		break;

	 case SDLK_z:
	 case 63:
		chip8.Key = 0xA;
		break;

	 case SDLK_x:
	 case SDLK_KP_ENTER:
		chip8.Key = 0xA;
		break;

	 case SDLK_c:
	 case SDLK_KP_MINUS:
		chip8.Key = 0xB;
		break;

	 case SDLK_v:
	 case SDLK_KP_PLUS:
		chip8.Key = 0xC;
		break;

	 case SDLK_a:
	 case SDLK_KP_MULTIPLY:
		chip8.Key = 0xD;
		break;

	 case SDLK_s:
	 case SDLK_KP_DIVIDE:
		chip8.Key = 0xE;
		break;

	 case SDLK_d:
	 case SDLK_KP_SPACE:
		chip8.Key = 0xF;
		break;

	 default:
		chip8.Key = 0;
	}
}


void HandleSdlEventsTask()
{
	SDL_Event event;
	while( !shouldStop )
	{
		while( SDL_PeepEvents( &event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT ) )
		{
			switch( event.type )
			{
			 case SDL_QUIT:
				chip8.killTimer = true;
				shouldStop = true;
				break;

			 case SDL_WINDOWEVENT:
				if( event.window.event == SDL_WINDOWEVENT_CLOSE )
				{
					chip8.killTimer = true;
					shouldStop = true;
				}
				break;

			 case SDL_KEYDOWN:
			 case SDL_KEYUP:
				HandleInput( event.key );
			}
		}

		this_thread::sleep_for( chrono::milliseconds( 1 ) );
	}
}

