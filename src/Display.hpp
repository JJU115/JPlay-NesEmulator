#include <SDL.h>

 
class Display {
    public:
        Display();
        ~Display();
        void Render_Frame(uint32_t* nextFrame);
    private:
        SDL_Window* NesWindow;
        SDL_Renderer* NesRenderer;
        SDL_Texture* NesTexture;
        uint32_t* FramePixels;
        void* Frame;
        int Pitch;
};