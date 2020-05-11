#include "Display.hpp"
#include "Palette.hpp"
#include <iostream>

//Of course, should do error checks for the init, createWindow, and WindowSurface functions
//Generated picture is 256*240 but overscan actually makes it 256*224 
Display::Display() {

    SDL_Init(SDL_INIT_VIDEO);
    pixels = new uint32_t[256 * 240]; //Holds RGBA values for each pixel on screen
    nesWindow = SDL_CreateWindow("JPlay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, SDL_WINDOW_SHOWN);
    nesRenderer = SDL_CreateRenderer(nesWindow, -1, 0);
    nesTexture = SDL_CreateTexture(nesRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
}


Display::~Display() {
    SDL_DestroyTexture(nesTexture);
    SDL_DestroyRenderer(nesRenderer);
    SDL_DestroyWindow(nesWindow);
    SDL_Quit();
}


void Display::RENDER_PIXEL(uint16_t num, uint8_t ind) {
    //std::cout << "num: " << int(num) << " ind"
    pixels[num] = ((SYSTEM_PAL[ind].R << 24) | (SYSTEM_PAL[ind].G << 16) | (SYSTEM_PAL[ind].B << 8) | (SYSTEM_PAL[ind].A));
    SDL_UpdateTexture(nesTexture, NULL, pixels, 256 * sizeof(Uint32));
    SDL_RenderClear(nesRenderer); //All of this at the end of the update loop
    SDL_RenderCopy(nesRenderer, nesTexture, NULL, NULL);
    SDL_RenderPresent(nesRenderer);
}