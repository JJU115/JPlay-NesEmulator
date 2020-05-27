#include "Display.hpp"
#include <iostream>
/*
    There are multiple versions of the render pixel function. The goal is to find the most efficient rendering
    procedure, this probably will involve not rendering a single pixel on every cycle but rather filling in all
    the pixel data for a whole scanline or a whole frame then rendering it all at once. 

    - Create new surface for each frame and blit it
    - Use a texture and directly manipulate the pixels with lock and unlock functions for each frame
*/

//Idea: don't need an alpha component for SYSTEM_PAL since it's always 0xFF

//Of course, should do error checks for the init, createWindow, and WindowSurface functions
//Generated picture is 256*240 but overscan actually makes it 256*224 
Display::Display() {

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    nesWindow = SDL_CreateWindow("JPlay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, SDL_WINDOW_SHOWN);
    nesRenderer = SDL_CreateRenderer(nesWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    nesTexture = SDL_CreateTexture(nesRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
}


Display::~Display() {
    SDL_DestroyRenderer(nesRenderer);
    SDL_DestroyWindow(nesWindow);
    SDL_Quit();
}


void Display::RENDER_FRAME(uint32_t* nextFrame) {
    if (SDL_LockTexture(nesTexture, NULL, &frame, &pitch) != 0)
        std::cout << "Lock failed: " << SDL_GetError();

    framePixels = (uint32_t*)frame;
    memcpy(framePixels, nextFrame, 256*960); //Can put (uint32_t*)frame here instead of using framePixels at all?
    SDL_UnlockTexture(nesTexture);
    SDL_RenderCopy(nesRenderer, nesTexture, NULL, NULL);
    SDL_RenderPresent(nesRenderer);
    frame = NULL;

}