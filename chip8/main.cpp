//
//  main.cpp
//  chip8
//  Created by daniel mills on 2020-01-23.
//  Copyright © 2020 daniel mills. All rights reserved.
//
// CHIP-8 SPEC DOC: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

#include <iostream>
#include <string>

const int STACK_SIZE = 16;
const int NUM_REGISTERS = 16;
const int MEM_SIZE = 4096;
const int GFX_ROWS = 32;
const int GFX_COLS = 64;

class Chip8 {
public:
    Chip8() {
        for(auto i =0; i < MEM_SIZE;i++) {
            memory[i] = 0;
        }
    };
    ~Chip8() {};
    
    void loadFile(std::string filename) {
        FILE *fp = fopen(filename.c_str(), "rb");
           
        if(fp == NULL) {
            std::cout << "Unable to open file\n";
            return;
        }
       
        //Get length of binary file
        fseek(fp, 0L, SEEK_END);
        auto filesize = ftell(fp);
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
    }
    
    void start() {
        
        this->PC = 0x200;
        while(this->PC < MEM_SIZE) {
            this->emulateCycle();
        }
    }
    void emulateCycle() {
        uint16_t opcode = memory[PC] << 8 | memory[PC + 1];
        std::cout << "Opcode: " << std::hex << opcode << "\n";
        
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
            case 0x0000:
                switch(opcode) {
                    case 0x00E0:
                        //00E0 - CLS => Clear the display.
                        std::cout<<"Clear screen.\n";
                        break;
                    case 0x00EE:
                        //00EE - RET => Return from a subroutine.
                        this->PC = this->stack[this->SP];
                        this->SP--;
                        break;
                }
                break;
            case 0x1000:
                // 1nnn - JP addr => Jump to location nnn.
                this->PC = nnn;
                break;
            case 0x2000:
                // 2nnn - CALL addr => Call subroutine at nnn.
                this->SP++;
                this->stack[this->SP] = this->PC;
                this->PC = nnn;
                break;
            case 0x3000:
                //3xkk - SE Vx, byte => Skip next instruction if Vx = kk.
                if(this->V[x] == kk) {
                    this->PC += 2;
                }
                break;
            case 0x4000:
                // 4xkk - SNE Vx, byte => Skip next instruction if Vx != kk.
                if(this->V[x] != kk) {
                    this->PC += 2;
                 }
                break;
            case 0x5000:
                // 5xy0 - SE Vx, Vy => Skip next instruction if Vx = Vy.
                if(this->V[x] == this->V[y]) {
                    this->PC += 2;
                }
                break;
            case 0x6000:
                // 6xkk - LD Vx, byte = >Set Vx = kk.
                V[x] = kk;
                break;
            case 0x7000:
                //7xkk - ADD Vx, byte => Set Vx = Vx + kk.
                this->V[x] = this->V[x] + kk;
                break;     
            case 0x8000:
                switch(n) {
                    case 0:
                        //8xy0 - LD Vx, Vy => Set Vx = Vy.
                        this->V[x] = this->V[y];
                        break;
                    case 1:
                        // 8xy1 - OR Vx, Vy => Set Vx = Vx OR Vy.
                        this->V[x] = this->V[x] | this->V[y];
                        break;
                    case 2:
                        // 8xy2 - AND Vx, Vy => Set Vx = Vx AND Vy.
                        this->V[x] = this->V[x] & this->V[y];
                        break;
                    case 3:
                        // 8xy3 - XOR Vx, Vy => Set Vx = Vx XOR Vy.
                        this->V[x] = this->V[x] ^ this->V[y];
                        break;
                    case 4: {
                        // 8xy4 - ADD Vx, Vy => Set Vx = Vx + Vy, set VF = carry.
                        uint16_t foo = this->V[x] + this->V[y];
                        if (foo > 255) this->V[0xF] = 1;
                        else this->V[0xF] = 0;
                        this->V[x] = foo & 0xff;
                        break;
                    }
                    case 5: {
                        // 8xy5 - SUB Vx, Vy => Set Vx = Vx - Vy, set VF = NOT borrow.
                        if(this->V[x] > this->V[y]) {
                            this->V[0xF] = 1;
                        } else { this->V[0xF] = 0; }
                        
                        uint8_t sub = this->V[x] - this->V[y];
                        this->V[x] = sub;
                        break;
                    }
                    case 6:
                        //8xy6 - SHR Vx {, Vy} => Set Vx = Vx SHR 1.
                        std::cout<< "0x8xy6 \n";
                        // If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
                        
                        break;
                    case 7:
                        std::cout<< "0x8xy7 \n";
                        break;
                    case 0xE:
                        std::cout<< "0x8xyE \n";
                        break;
                }
                break;
            case 0x9000:
                if(this->V[x] != this->V[x]) {
                    this->PC += 2;
                }
                break;
            case 0xA000:
                this->I = nnn;
                std::cout<<"0xA000\n";
                break;
            case 0xB000:
                this->PC = (this->V[0] + nnn);
                break;
            case 0xC000:
                std::cout<<"0xC000\n";
                break;
            case 0xD000:
                std::cout << "0xD DRAW ROUTINE \n";
            case 0xE000:
                std::cout<<"0xE000 skip instruction on keyboard\n";
                switch(kk) {
                    case 0x9E:
                        break;
                }
                break;
            case 0xF000:
                std::cout<<"0xE000\n";
                break;
        }
       // std::cout << "pc: " << this->PC << "\n";
        this->PC += 2;
    }
    
    
private:
    uint8_t memory[MEM_SIZE], V[NUM_REGISTERS], gfx[GFX_ROWS][GFX_COLS], delayTimer, soundTimer;
    uint16_t I, PC, SP, stack[STACK_SIZE];
};


int main(int argc, const char * argv[]) {
    Chip8 emulator;
    emulator.loadFile("/Users/vehikl/Code/chipate/chip8/GUESS");
    emulator.start();
    return 0;
}
