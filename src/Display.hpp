#include <SDL2/SDL.h>

 
class Display {
    friend class PPU;
    public:
        Display();
        ~Display();
    private:
        void RENDER_PIXEL(uint16_t num, uint8_t val);
        void RENDER_FRAME();

        SDL_Window* nesWindow;
        SDL_Renderer* nesRenderer;
        SDL_Texture* nesTexture;
        uint32_t* pixels;
        uint32_t* framePixels;
        void* frame;
        int pitch;
};