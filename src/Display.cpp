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

    SDL_Init(SDL_INIT_VIDEO);
    pixels = new uint32_t[256 * 240]; //Holds RGBA values for each pixel on screen
    nesWindow = SDL_CreateWindow("JPlay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, SDL_WINDOW_SHOWN);
    nesRenderer = SDL_CreateRenderer(nesWindow, -1, SDL_RENDERER_ACCELERATED);
    nesTexture = SDL_CreateTexture(nesRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
}


Display::~Display() {
    SDL_DestroyRenderer(nesRenderer);
    SDL_DestroyWindow(nesWindow);
    SDL_Quit();
}

/*
void Display::RENDER_PIXEL(uint16_t num, uint8_t ind) {
    pixels[num] = ((SYSTEM_PAL[ind].R << 24) | (SYSTEM_PAL[ind].G << 16) | (SYSTEM_PAL[ind].B << 8) | (SYSTEM_PAL[ind].A));
    SDL_UpdateTexture(nesTexture, NULL, pixels, 256 * sizeof(Uint32));
    SDL_RenderClear(nesRenderer); //All of this at the end of the update loop
    SDL_RenderCopy(nesRenderer, nesTexture, NULL, NULL);
    SDL_RenderPresent(nesRenderer);
}*/

/*
void Display::RENDER_PIXEL(uint16_t num, uint8_t ind) {
    SDL_SetRenderDrawColor(nesRenderer, SYSTEM_PAL[ind].R, SYSTEM_PAL[ind].G, SYSTEM_PAL[ind].B, SYSTEM_PAL[ind].A);
    SDL_RenderDrawPoint(nesRenderer, num%256, num/256);
    SDL_RenderPresent(nesRenderer);
}*/


void Display::RENDER_FRAME() {
    if (SDL_LockTexture(nesTexture, NULL, &frame, &pitch) != 0)
        std::cout << "Lock failed: " << SDL_GetError();

    framePixels = (uint32_t*)frame;
    memcpy(framePixels, pixels, 256*960);
    SDL_UnlockTexture(nesTexture);
    SDL_RenderCopy(nesRenderer, nesTexture, NULL, NULL);
    SDL_RenderPresent(nesRenderer);
    frame = NULL;

}