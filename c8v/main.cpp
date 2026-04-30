//
//  main.cpp
//  c8v
//
//  Created by mills on 2023-06-30.
//

#include <iostream>
#include <ctime>
#include <iomanip>
#include <SDL2/SDL.h>
#include "Display.cpp"

#define DEBUG 1

const int STACK_SIZE = 16;
const int NUM_REGISTERS = 16;
const int MEM_SIZE = 4096;
const int GFX_ROWS = 32;
const int GFX_COLS = 64;
const int MEM_START = 0x200;
const int FONT_OFFSET = 0x50;
const int FONTSET_BYTES_PER_CHAR = 5;
const int CYCLES_PER_SECOND = 500;
const int TIMER_HZ = 60;


class Chip8 {
public:
    Chip8() {
        for(auto i =0; i < MEM_SIZE;i++) {
            memory[i] = 0;
        }
        this->display = new Display();
    };
    ~Chip8() {
        delete this->display;
    };
    
    void setKey(uint8_t k, bool pressed) {
        this->keys[k] = pressed;
    }
    
    long loadFile(std::string filename) {
        FILE *fp = fopen(filename.c_str(), "rb");
           
        if(fp == NULL) {
            std::cout << "Unable to open file\n";
            return -1;
        }
       
        //Get length of binary file
        fseek(fp, 0L, SEEK_END);
        auto filesize = ftell(fp);
        
#ifdef DEBUG
        std::cout << filename << " Size: " << filesize << "\n";
#endif
        
        fseek(fp, 0L, SEEK_SET);
        
        //create temporary array for storing, read in all data
        auto memory = new uint8_t[filesize];
           
        fread(memory, 1, filesize, fp);
        fclose(fp);
        
        //copy over file into c8 memory, starting at 0x200 (512)
        auto startByte = 0x200;
        for(auto i = 0; i < filesize; i++) {
            this->memory[startByte++] = memory[i];
        }
        
        //tidy up temporary memory
        delete[] memory;
        
        return filesize;
    }
    
    void dumpMemory(int start=0x200, int end=MEM_SIZE) {
        for (size_t i = start; i < end; i+=2) {
            if ((i) % 8 == 0)
                std::cout << std::endl <<"0x" << std::hex << std::setw(4) << std::setfill('0') << i << ": ";
            
            uint16_t opcode = memory[i] << 8 | memory[i + 1];
               std::cout << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(opcode) << " ";
               
               if ((i + 1) % 8 == 0)
                   std::cout << std::endl;
           }
           std::cout << std::dec << std::endl;
    }
    
    void start() {
        this->SP = 0;
        this->I =0;
        this->PC = 0x200;
        this->delayTimer = 0;
        this->soundTimer = 0;
        
        uint8_t chip8FontData[] = {
                0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                0x20, 0x60, 0x20, 0x20, 0x70, // 1
                0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                0xF0, 0x80, 0xF0, 0x80, 0x80  // F
            };

        size_t fontDataSize = sizeof(chip8FontData);
        for (size_t i = 0; i < fontDataSize; i++) {
               this->memory[i + FONT_OFFSET] = chip8FontData[i];
           }
        
        for(auto i =0; i < 16;i++) keys[i] = false;
    }
    
    void emulateCycle() {
        uint16_t opcode = memory[PC] << 8 | memory[PC + 1];
        //std::cout << "Opcode: " << std::hex << opcode << "\n";
        
        //decode pieces of opcode
        uint8_t x, y, n;
        uint8_t kk;
        uint16_t nnn;
        x   = (opcode >> 8) & 0x000F; // the lower 4 bits of the high byte
        y   = (opcode >> 4) & 0x000F; // the upper 4 bits of the low byte
        n   = opcode & 0x000F; // the lowest 4 bits
        kk  = opcode & 0x00FF; // the lowest 8 bits
        nnn = opcode & 0x0FFF; // the lowest 12 bits
        
        switch(opcode & 0xF000) {
            case 0x0000: {
                switch(kk) {
                    case 0xE0: {
                        this->display->clear();
                        break;
                    }
                    case 0xEE: {
                        this->PC = this->stack[this->SP];
                        this->SP--;
                        return;
                    }
                }
                break;
            }
            case 0x1000: {
                this->PC = nnn;
                return;
                //break;
            }
            case 0x2000: {
                this->SP++;
                this->stack[this->SP] = this->PC + 2;
                this->PC = nnn;
                return;
            }
            case 0x3000: {
                if(this->V[x] == kk) {
                    this->PC +=2;
                }
                break;
            }
            case 0x4000: {
                if(this->V[x] != kk) {
                    this->PC +=2;
                }
                break;
            }
            case 0x5000: {
                if(this->V[x] == this->V[y]) {
                    this->PC +=2;
                }
                break;
            }
            case 0x6000: {
                this->V[x] = kk;
                break;
            }
            case 0x7000: {
                this->V[x] = this->V[x] + kk;
                break;
            }
            case 0x8000: {
                switch(n) {
                    case 0: {
                        //set
                        this->V[x] = this->V[y];
                        break;
                    }
                    case 1: {
                        //OR
                        this->V[x] = this->V[x] | this->V[y];
                        break;
                    }
                    case 2: {
                        //AND
                        this->V[x] = this->V[x] & this->V[y];
                        break;
                    }
                    case 3: {
                        //XOR
                        this->V[x] = this->V[x] ^ this->V[y];
                        break;
                    }
                    case 4: {
                        //ADD w/ carry
                        uint16_t sum = this->V[x] + this->V[y];
                        this->V[0xF] = sum > 255 ? 1 : 0;
                        this->V[x] = sum & 255;
                        break;
                    }
                    case 5: {
                        //SUB w/ borrow
                        this->V[0xF] = this->V[x] > this->V[y] ? 1 : 0;
                        this->V[x] = this->V[x] - this->V[y];
                        break;
                    }
                    case 6: {
                        // SHR
                       V[0xF] = V[x] & 0x1;
                       V[x] >>= 1;
                       break;
                   }
                   case 7: {
                       // SUBN
                       V[0xF] = V[y] > V[x] ? 1 : 0;
                       V[x] = V[y] - V[x];
                       break;
                   }
                   case 0xE: {
                       // SHL
                       V[0xF] = (V[x] >> 7) & 0x1;
                       V[x] <<= 1;
                       break;
                   }
                }
                break;
            }
            case 0x9000: {
                if(this->V[x] != this->V[y]) {
                    this->PC += 2;
                }
                break;
            }
            case 0xA000: {
                this->I = nnn;
                break;
            }
            case 0xB000: {
                this->PC = this->V[0] + nnn;
                return;
            }
            case 0xC000: {
                this->V[x] = (rand() % 256) & kk;
                break;
            }
            case 0xD000: {
                this->drawRoutine(x, y, n);
                break;
            }
            case 0xE000: {
                switch(kk) {
                    case 0x9E: {
                        if(this->keys[this->V[x]]) this->PC += 2;
                        break;
                    }
                    case 0xA1: {
                        if(!this->keys[this->V[x]]) this->PC += 2;
                        break;
                    }
                }
                break;
            }
            case 0xF000: {
                switch(kk) {
                    case 0x07: {
                        this->V[x] = this->delayTimer;
                        break;
                    }
                    case 0x0A: {
                        bool found = false;
                           for(int i = 0; i < 16; i++) {
                               if(this->keys[i]) {
                                   this->V[x] = i;
                                   found = true;
                                   break;
                               }
                           }
                           // no key === just excute instruction again on next loop
                           if(!found) this->PC -= 2;
                           break;
                    }
                    case 0x15: {
                        this->delayTimer = this->V[x];
                        break;
                    }
                    case 0x18: {
                        this->soundTimer = this->V[x];
                        break;
                    }
                    case 0x1E: {
                        this->I = this->I + this->V[x];
                        break;
                    }
                    case 0x29: {
                        I = FONT_OFFSET + FONTSET_BYTES_PER_CHAR * V[x];
                        break;
                    }
                    case 0x33: {
                        this->memory[this->I]     = this->V[x] / 100;        // hundreds
                        this->memory[this->I + 1] = (this->V[x] / 10) % 10;  // tens
                        this->memory[this->I + 2] = this->V[x] % 10;         // ones
                        break;
                    }
                    case 0x55: {
                        // store V[0] to V[x] in memory starting at I
                        for(int i = 0; i <= x; i++) {
                            this->memory[this->I + i] = this->V[i];
                        }
                        break;
                    }
                    case 0x65: {
                        // load V[0] to V[x] from memory starting at I
                        for(int i = 0; i <= x; i++) {
                            this->V[i] = this->memory[this->I + i];
                        }
                        break;
                    }
                }
            }
        }
        
        this->PC += 2;
    }
    
    void drawRoutine(int x, int y, int n) {
        unsigned col = this->V[x];
        unsigned row = this->V[y];
           unsigned byte_index;
           unsigned bit_index;

        // set the collision flag to 0
        this->V[0xF] = 0;
        for (byte_index = 0; byte_index < n; byte_index++) {
            uint8_t byte = this->memory[I + byte_index];
            for (bit_index = 0; bit_index < 8; bit_index++) {
                uint8_t bit = (byte >> (7 - bit_index)) & 0x1;
                unsigned px = (col + bit_index) % GFX_COLS;   // advance column
                unsigned py = (row + byte_index) % GFX_ROWS;  // advance row
                bool pixel = this->display->getPixel(px, py);
                if (bit == 1 && pixel) V[0xF] = 1;
                if (bit == 1)
                    this->display->setPixel(px, py, !pixel);  // only XOR if bit is set
            }
        }
    }
    
    Display* display;
    
    void checkTimers() {
        if(this->delayTimer > 0) this->delayTimer--;
        if(this->soundTimer > 0) this->soundTimer--;
    }
private:
    uint8_t memory[MEM_SIZE], V[NUM_REGISTERS], delayTimer, soundTimer;
    uint16_t I, PC, SP, stack[STACK_SIZE];
    bool keys[16];
  
};


int main(int argc, const char * argv[]) {
    srand(static_cast<unsigned int>(time(0)));

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Unable to init sdl\n";
        return -1;
    }
    

    Chip8* c8 = new Chip8();
    auto filesize = c8->loadFile("/Users/mills/projects/chipate/chip8/trip8.ch8");
    c8->start();
    c8->dumpMemory(MEM_START, MEM_START + filesize);

    uint32_t lastTime = SDL_GetTicks();
    uint32_t timerAccum = 0;
    uint32_t cycleAccum = 0;
    bool quit = false;
    SDL_Event event;
    while (!quit) {
         
        uint32_t now = SDL_GetTicks();
        uint32_t delta = now - lastTime;
        lastTime = now;
         
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { quit = true; }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool pressed = event.type == SDL_KEYDOWN;
                switch(event.key.keysym.sym) {
                    case SDLK_1: c8->setKey(0x1, pressed); break;
                    case SDLK_2: c8->setKey(0x2, pressed); break;
                    case SDLK_3: c8->setKey(0x3, pressed); break;
                    case SDLK_4: c8->setKey(0xC, pressed); break;
                    case SDLK_q: c8->setKey(0x4, pressed); break;
                    case SDLK_w: c8->setKey(0x5, pressed); break;
                    case SDLK_e: c8->setKey(0x6, pressed); break;
                    case SDLK_r: c8->setKey(0xD, pressed); break;
                    case SDLK_a: c8->setKey(0x7, pressed); break;
                    case SDLK_s: c8->setKey(0x8, pressed); break;
                    case SDLK_d: c8->setKey(0x9, pressed); break;
                    case SDLK_f: c8->setKey(0xE, pressed); break;
                    case SDLK_z: c8->setKey(0xA, pressed); break;
                    case SDLK_x: c8->setKey(0x0, pressed); break;
                    case SDLK_c: c8->setKey(0xB, pressed); break;
                    case SDLK_v: c8->setKey(0xF, pressed); break;
                }
            }
        }

        cycleAccum += delta;
        int cyclesToRun = (CYCLES_PER_SECOND * cycleAccum) / 1000;
        if(cyclesToRun > 0) {
           cycleAccum = 0;
           for(int i = 0; i < cyclesToRun; i++) {
               c8->emulateCycle();
           }
        }

        timerAccum += delta;
        if(timerAccum >= 1000 / TIMER_HZ) {
            c8->checkTimers();
            timerAccum = 0;
        }

        if(c8->display->dirty) {
            c8->display->draw();
            c8->display->dirty = false;
        }
    }
    
    delete c8;
    SDL_Quit();

    return 0;
}
