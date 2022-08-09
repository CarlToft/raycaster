# Raycaster

This is a simple "from-scratch" raycaster implementation in C++ using SDL2. This implementation uses some pretty sketchy memory accessing for reading and setting RGB values in the SDL_Surface pixel data, but it was the fastest way I could come up with. You have been warned; don't blame me if it crasches your computer!

## Dependencies
You will need to install SDL2 and SDL2-Image. On Ubuntu, you can install them like this:

```
sudo apt install libsdl2-2.0 libsdl2-dev libsdl2-image libsdl2-image-dev libsdl2-ttf-dev
```

If using cmake, you need to initialize the submodule for findsdl2
```bash
git submodule update --init
```

## Compiling
The program can be compiled using g++ like this:

```
g++ main.cpp -O3 -l SDL2 -l SDL2_image -l SDL2_ttf -pthread
```

or using cmake:
```bash
mkdir build
cd build
cmake ..
make
```

On windows, you can use vcpkg to install the dependencies and use the vcpkg cmake toolchain:

```
vcpkg install sdl2 sdl2_image sdl2_ttf

cmake -DCMAKE_TOOLCHAIN_FILE={VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake ..
```

## Demo

![Demo of raycaster](https://github.com/CarlToft/raycaster/blob/main/images/vis.gif?raw=true)
