#include <SDL2/SDL.h>


class Display {
    friend class PPU;
    public:
        Display();
    private:
        void RENDER_PIXEL(uint16_t num, uint32_t val);

        SDL_Window* nesWindow;
        SDL_Renderer* nesRenderer;
        SDL_Texture* nesTexture;
        uint32_t* pixels;
};