#include <SDL.h>
#include <emscripten.h>

SDL_Window *window;
SDL_Renderer *renderer;

int position = 0;
int direction = 1;

typedef struct {
	struct UiNode *prev;
	struct UiNode *next;
	SDL_Rect rect;
} UiNode;

void game_loop(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            emscripten_cancel_main_loop();
            return;
        }
    }

    position += direction * 3;

    if (position > 550 || position < 0) {
        direction *= -1;
    }

    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    SDL_Rect rectangle = { position, 200, 80, 80 };

    SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    SDL_RenderFillRect(renderer, &rectangle);

    SDL_RenderPresent(renderer);
}

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }

    window = SDL_CreateWindow(
        "My SDL2 Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640,
        480,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        return 1;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!renderer) {
        return 1;
    }

    emscripten_set_main_loop(game_loop, 0, 1);

    return 0;
}
