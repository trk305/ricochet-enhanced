/*
// platform_compat.cpp (Linux only)
#ifndef _WIN32
#include "platform_compat.h"
#include <SDL2/SDL.h>

void SDL_GetCursorPos(POINT* p) {
    SDL_GetMouseState(&p->x, &p->y);
}

void SDL_SetCursorPos(int x, int y) {
    SDL_WarpMouseInWindow(NULL, x, y);
}
#endif*/