#include "game.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

global_variable SDL_Window *Window = NULL;
global_variable SDL_Renderer *Renderer = NULL;
global_variable u64 LastTime = 0;
global_variable rng RNG;

SDL_AppResult SDL_AppInit(void **AppState, int argc, char *argv[])
{
    
}

SDL_AppResult SDL_AppEvent(void *AppState, SDL_Event *Event)
{
    
}

SDL_AppResult SDL_AppIterate(void *AppState)
{
    
}

void SDL_AppQuit(void *AppState, SDL_AppResult result)
{
    
}




































