#include "Display.hpp"
#include <iostream>

//Of course, should do error checks for the init, createWindow, and WindowSurface functions 
Display::Display() {

    SDL_Init(SDL_INIT_VIDEO);
    pixels = new uint32_t[256 * 224]; //Holds RGBA values for each pixel on screen
    nesWindow = SDL_CreateWindow("JPlay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 224, SDL_WINDOW_SHOWN);
    nesRenderer = SDL_CreateRenderer(nesWindow, -1, 0);
    nesTexture = SDL_CreateTexture(nesRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 256, 224);
}


void Display::RENDER_PIXEL(uint16_t num, uint32_t val) {
    pixels[num] = val;
    SDL_UpdateTexture(nesTexture, NULL, pixels, 256 * sizeof(Uint32));
    SDL_RenderClear(nesRenderer); //All of this at the end of the update loop
    SDL_RenderCopy(nesRenderer, nesTexture, NULL, NULL);
    SDL_RenderPresent(nesRenderer);
}