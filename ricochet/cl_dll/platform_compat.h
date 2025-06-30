// platform_compat.h
#ifdef _WIN32
#include <windows.h>
#else
#include <SDL2/SDL.h>
#endif

typedef struct tagPOINT {
    int x;
    int y;
} POINT;

#ifdef _WIN32
// Windows implementations
void SDL_GetCursorPos(POINT* p) {
    GetCursorPos((LPPOINT)p);
}

void SDL_SetCursorPos(int x, int y) {
    SetCursorPos(x, y);
}
#else
// Linux declarations
void SDL_GetCursorPos(POINT* p);
void SDL_SetCursorPos(int x, int y);
#endif