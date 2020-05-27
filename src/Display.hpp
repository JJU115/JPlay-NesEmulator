#include <SDL.h>

 
class Display {
    public:
        Display();
        ~Display();
        void RENDER_FRAME(uint32_t* nextFrame);
    private:
        SDL_Window* nesWindow;
        SDL_Renderer* nesRenderer;
        SDL_Texture* nesTexture;
        uint32_t* framePixels;
        void* frame;
        int pitch;
};