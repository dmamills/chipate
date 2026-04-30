//
//  Display.cpp
//  c8v
//
//  Created by mills on 2023-07-01.
//

#include <stdio.h>
#include <SDL2/SDL.h>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 320;
const int PIXEL_SIZE = 10;
const int ARRAY_WIDTH = 64;
const int ARRAY_HEIGHT = 32;

class Display {
    
public:
    Display() {
        this->mWindow = SDL_CreateWindow(this->mWindowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
        this->mRenderer = SDL_CreateRenderer(this->mWindow, -1, SDL_RENDERER_ACCELERATED);
        this->clear();
        
    }
    ~Display() {
        SDL_DestroyRenderer(this->mRenderer);
        SDL_DestroyWindow(this->mWindow);
    }
    
    void setPixel(int x, int y, bool val) {
        this->mPixels[y * ARRAY_WIDTH + x] = val;
        this->dirty = true;
    }
    
    bool getPixel(int x, int y) {
        return this->mPixels[y * ARRAY_WIDTH + x];
    }
    
    bool getPixel(int n) { return this->mPixels[n]; }
    void setPixel(int n, bool val) {
        this->mPixels[n] = val;
        this->dirty = true;
    }
    
    void draw() {
        SDL_SetRenderDrawColor(this->mRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // Set background color to black
        SDL_RenderClear(this->mRenderer);

        SDL_SetRenderDrawColor(this->mRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // Set pixel color to white

        for (int x = 0; x < ARRAY_WIDTH; ++x) {
            for (int y = 0; y < ARRAY_HEIGHT; ++y) {
                if (this->mPixels[y * ARRAY_WIDTH + x]) {
                    SDL_Rect pixelRect = {x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE};
                    SDL_RenderFillRect(this->mRenderer, &pixelRect);
                }
            }
        }

        SDL_RenderPresent(this->mRenderer);
    }
    
    void clear() {
        for(auto i =0; i < ARRAY_WIDTH * ARRAY_HEIGHT;i++) this->mPixels[i] = false;
        dirty = true;
    }
    
    bool dirty = false;
    
private:
    const char* mWindowName = "Chip 8 Emulator";
    SDL_Window* mWindow;
    SDL_Renderer* mRenderer;
    bool mPixels[ARRAY_WIDTH * ARRAY_HEIGHT];
};
