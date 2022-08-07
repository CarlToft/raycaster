# Raycaster

This is a simple "from-scratch" raycaster implementation in C++ using SDL2. This implementation uses some pretty sketchy memory accessing for reading and setting RGB values in the SDL_Surface pixel data, but it was the fastest way I could come up with. You have been warned; don't blame me if it crasches your computer! 

## Dependencies
You will need to install SDL2 and SDL2-Image. On Ubuntu, you can install them like this:

```
sudo apt install libsdl2-2.0 libsdl2-dev libsdl2-image libsdl2-image-dev
```

## Compiling
The program can be compiled using g++ like this:

```
g++ main.cpp -l SDL2 -l SDL2_image
```

## Demo

![Demo of raycaster](https://github.com/CarlToft/raycaster/blob/main/images/vis.gif?raw=true)


