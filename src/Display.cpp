#include "Display.hpp"
#include <iostream>


//Of course, should do error checks for the init, createWindow, and WindowSurface functions
//Generated picture is 256*240 but overscan actually makes it 256*224 
Display::Display() {

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    NesWindow = SDL_CreateWindow("JPlay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, SDL_WINDOW_SHOWN);
    NesRenderer = SDL_CreateRenderer(NesWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    NesTexture = SDL_CreateTexture(NesRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
}


Display::~Display() {
    SDL_DestroyRenderer(NesRenderer);
    SDL_DestroyWindow(NesWindow);
    SDL_Quit();
}


void Display::Render_Frame(uint32_t* nextFrame) {
    if (SDL_LockTexture(NesTexture, NULL, &Frame, &Pitch) != 0)
        std::cout << "Lock failed: " << SDL_GetError();

    FramePixels = (uint32_t*)Frame;
    memcpy(FramePixels, nextFrame, 256*960);
    SDL_UnlockTexture(NesTexture);
    SDL_RenderCopy(NesRenderer, NesTexture, NULL, NULL);
    SDL_RenderPresent(NesRenderer);
    Frame = NULL;

}