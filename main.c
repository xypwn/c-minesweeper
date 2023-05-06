#include "main.h"
#include "data.gen.h"
#include "rng.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_video.h>

#include <stdlib.h>
#include <time.h>

typedef enum {
    TEXTURE_TILE_CLOSED,
    TEXTURE_TILE_OPEN,
    TEXTURE_MINE,
    TEXTURE_MINE_FLAGGED,
    TEXTURE_FLAG,
    TEXTURE_NUMBERS,
    TEXTURE_GAME_OVER,
    TEXTURE_VICTORY,
    TEXTURE_DIFFICULTIES,
    TEXTURES_LEN,
} Texture;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* textures[TEXTURES_LEN];
} Gfx;

static SDL_Texture* load_texture(SDL_Renderer* renderer, const u8* mem, usize mem_len)
{
    return IMG_LoadTexture_RW(renderer, SDL_RWFromConstMem(mem, mem_len), 1);
}

static Gfx gfx_init(const char* window_title)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        panic("failed to initialize SDL: %s", SDL_GetError());
    }

    SDL_Rect display_bounds;
    SDL_GetDisplayUsableBounds(0, &display_bounds);

    SDL_Window* window = SDL_CreateWindow(window_title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        display_bounds.w * 0.8, display_bounds.h * 0.8,
        SDL_WINDOW_RESIZABLE);
    if (!window) {
        panic("failed to initialize window: %s", SDL_GetError());
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    Gfx self = {
        .window = window,
        .renderer = renderer,
    };
    self.textures[TEXTURE_TILE_CLOSED] = load_texture(renderer, data_tile_closed_png, arrlen(data_tile_closed_png));
    self.textures[TEXTURE_TILE_OPEN] = load_texture(renderer, data_tile_open_png, arrlen(data_tile_open_png));
    self.textures[TEXTURE_MINE] = load_texture(renderer, data_mine_png, arrlen(data_mine_png));
    self.textures[TEXTURE_MINE_FLAGGED] = load_texture(renderer, data_mine_flagged_png, arrlen(data_mine_flagged_png));
    self.textures[TEXTURE_FLAG] = load_texture(renderer, data_flag_png, arrlen(data_flag_png));
    self.textures[TEXTURE_NUMBERS] = load_texture(renderer, data_numbers_png, arrlen(data_numbers_png));
    self.textures[TEXTURE_GAME_OVER] = load_texture(renderer, data_game_over_png, arrlen(data_game_over_png));
    self.textures[TEXTURE_VICTORY] = load_texture(renderer, data_victory_png, arrlen(data_victory_png));
    self.textures[TEXTURE_DIFFICULTIES] = load_texture(renderer, data_difficulties_png, arrlen(data_difficulties_png));

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    return self;
}

static void gfx_deinit(const Gfx* self)
{
    for (usize i = 0; i < arrlen(self->textures); i++) {
        SDL_DestroyTexture(self->textures[i]);
    }
    SDL_DestroyRenderer(self->renderer);
    SDL_DestroyWindow(self->window);
    SDL_Quit();
}

typedef struct {
    bool mine;
    bool flag;
    bool open;
    u8 nearby_mines;
} Tile;

typedef struct {
    Tile* tiles;
    usize w;
    usize h;
} Board;

static Board board_init(usize width, usize height)
{
    Board self = {
        .w = width,
        .h = height,
    };

    if (!(self.tiles = calloc(self.w * self.h, sizeof(Tile)))) {
        panic("Out of memory!");
    }

    return self;
}

static void board_deinit(const Board* self)
{
    free(self->tiles);
}

static void board_generate(Board* self, RNG* rng, usize mines, usize safe_x, usize safe_y)
{
    usize* random_tile_indices;
    if (!(random_tile_indices = calloc(self->w * self->h, sizeof(usize)))) {
        panic("Out of memory!");
    }
    for (usize i = 0; i < self->w * self->h; i++) {
        random_tile_indices[i] = i;
    }

    // Fisher-Yates shuffle
    for (usize i = self->w * self->h - 1; i > 0; i--) {
        usize j = rng_u64_cap(rng, i + 1);
        swap(usize, random_tile_indices[i], random_tile_indices[j]);
    }

    // Place mines, ensuring that there is a 3x3 "safe" area around safe_x and safe_y
    for (usize i = 0, n = 0; n < mines; i++) {
        isize offsets[9][2] = {
            { -1, -1 },
            { 0, -1 },
            { 1, -1 },
            { -1, 0 },
            { 0, 0 },
            { 1, 0 },
            { -1, 1 },
            { 0, 1 },
            { 1, 1 },
        };
        bool outside_safe_area = true;
        for (usize j = 0; j < arrlen(offsets); j++) {
            isize sx = safe_x + offsets[j][0];
            isize sy = safe_y + offsets[j][1];
            if (sx < 0 || sy < 0 || sx >= self->w || sy >= self->h) {
                continue;
            }
	    usize safe_tile_index = sy * self->w + sx;
            if (safe_tile_index == random_tile_indices[i]) {
                outside_safe_area = false;
                break;
            }
        }
	if (i >= self->w * self->h) {
		panic("ran out of tile indices placing while mines");
	}
        if (outside_safe_area) {
            self->tiles[random_tile_indices[i]].mine = true;
            n++;
        }
    }

    free(random_tile_indices);

    for (usize y = 0; y < self->h; y++) {
        for (usize x = 0; x < self->w; x++) {
            isize offsets[8][2] = {
                { -1, -1 },
                { 0, -1 },
                { 1, -1 },
                { -1, 0 },
                { 1, 0 },
                { -1, 1 },
                { 0, 1 },
                { 1, 1 },
            };
            for (usize i = 0; i < arrlen(offsets); i++) {
                isize cx = x + offsets[i][0];
                isize cy = y + offsets[i][1];
                if (cx < 0 || cy < 0 || cx >= self->w || cy >= self->h) {
                    continue;
                }
                if (self->tiles[cy * self->w + cx].mine) {
                    self->tiles[y * self->w + x].nearby_mines++;
                }
            }
        }
    }
}

static void board_explore(Board* self, usize x, usize y)
{
    usize index = y * self->w + x;

    if (self->tiles[index].open) {
        return;
    }
    self->tiles[index].open = true;

    if (self->tiles[index].nearby_mines > 0) {
        return;
    }

    isize offsets[8][2] = {
        { -1, -1 },
        { 0, -1 },
        { 1, -1 },
        { -1, 0 },
        { 1, 0 },
        { -1, 1 },
        { 0, 1 },
        { 1, 1 },
    };
    for (usize i = 0; i < arrlen(offsets); i++) {
        isize cx = x + offsets[i][0];
        isize cy = y + offsets[i][1];
        if (cx < 0 || cy < 0 || cx >= self->w || cy >= self->h) {
            continue;
        }
        board_explore(self, cx, cy);
    }
}

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD,
    DIFFICULTIES_LEN,
} Difficulty;

int main(int argc, const char** argv)
{
    Gfx gfx = gfx_init("Minesweeper");

    RNG_XoShiRo256ss rng_xoshiro = rng_xoshiro256ss(time(NULL));
    RNG* rng = (RNG*)&rng_xoshiro;

    Difficulty difficulty = DIFFICULTY_EASY;
    bool difficulty_changed = false;

    Board board = board_init(9, 9);
    usize mines = 10;
    bool board_generated = false;

    bool run = true;
    bool game_over = false;
    bool victory = false;
    while (run) {
        if (!board_generated && difficulty_changed) {
            board_deinit(&board);
            switch (difficulty) {
            case DIFFICULTY_EASY:
                board = board_init(9, 9);
                mines = 10;
                break;
            case DIFFICULTY_MEDIUM:
                board = board_init(16, 16);
                mines = 40;
                break;
            case DIFFICULTY_HARD:
                board = board_init(20, 20);
                mines = 80;
                break;
            default:
                unreachable();
            }
            difficulty_changed = false;
        }

        int render_w, render_h;
        SDL_GetRendererOutputSize(gfx.renderer, &render_w, &render_h);

        f32 tile_size, tile_offset_x = 0.0, tile_offset_y = 0.0;
        if (render_w < render_h) {
            tile_size = (f32)render_w / (f32)board.w;
            tile_offset_x = 0.0;
            tile_offset_y = (render_h - render_w) / 2.0;
        } else {
            tile_size = (f32)render_h / (f32)board.h;
            tile_offset_x = (render_w - render_h) / 2.0;
            tile_offset_y = 0.0;
        }

        SDL_SetRenderDrawColor(gfx.renderer, 128, 128, 128, 255);
        SDL_RenderClear(gfx.renderer);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                run = false;
                break;
            case SDL_KEYDOWN:
                if (!board_generated) {
                    if (event.key.keysym.sym == SDLK_RIGHT
                        || event.key.keysym.sym == SDLK_DOWN) {
                        difficulty = (difficulty + 1) % DIFFICULTIES_LEN;
                    } else if (event.key.keysym.sym == SDLK_LEFT
                        || event.key.keysym.sym == SDLK_UP) {
                        difficulty = (difficulty + DIFFICULTIES_LEN - 1) % DIFFICULTIES_LEN;
                    }
                }
                difficulty_changed = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (!game_over && !victory) {
                    usize tile_x = (event.button.x - tile_offset_x) / tile_size;
                    usize tile_y = (event.button.y - tile_offset_y) / tile_size;
                    if (tile_x >= board.w || tile_y >= board.h) {
                        break;
                    }
                    usize tile_index = tile_y * board.w + tile_x;
                    switch (event.button.button) {
                    case SDL_BUTTON_LEFT:
                        if (!board.tiles[tile_index].flag) {
                            if (!board_generated) {
                                board_generate(&board, rng, mines, tile_x, tile_y);
                                board_generated = true;
                            }
                            if (board.tiles[tile_index].mine) {
                                game_over = true;
                            } else {
                                board_explore(&board, tile_x, tile_y);
                            }
                        }
                        break;
                    case SDL_BUTTON_RIGHT:
                        if (!board.tiles[tile_index].open) {
                            board.tiles[tile_index].flag = !board.tiles[tile_index].flag;
                        }
                        break;
                    }
                    break;
                }
            }
        }

        victory = true;
        for (usize i = 0; i < board.w * board.h; i++) {
            if (!board.tiles[i].open && !board.tiles[i].mine) {
                victory = false;
                break;
            }
        }

        for (usize y = 0; y < board.h; y++) {
            for (usize x = 0; x < board.w; x++) {
                usize tile_index = y * board.w + x;
                Texture texture = board.tiles[tile_index].open ? TEXTURE_TILE_OPEN : TEXTURE_TILE_CLOSED;
                SDL_FRect dest = {
                    tile_offset_x + x * tile_size,
                    tile_offset_y + y * tile_size,
                    tile_size,
                    tile_size,
                };
                SDL_RenderCopyF(gfx.renderer, gfx.textures[texture],
                    NULL,
                    &dest);
                if ((game_over || board.tiles[tile_index].open)
                    && !board.tiles[tile_index].mine
                    && !board.tiles[tile_index].flag
                    && board.tiles[tile_index].nearby_mines > 0) {
                    SDL_Rect src = {
                        (board.tiles[tile_index].nearby_mines - 1) * 16,
                        0,
                        16,
                        16,
                    };
                    SDL_RenderCopyF(gfx.renderer, gfx.textures[TEXTURE_NUMBERS],
                        &src,
                        &dest);
                }
                if ((game_over || victory) && board.tiles[tile_index].mine) {
                    Texture texture = board.tiles[tile_index].flag ? TEXTURE_MINE_FLAGGED : TEXTURE_MINE;
                    SDL_RenderCopyF(gfx.renderer, gfx.textures[texture],
                        NULL,
                        &dest);
                } else if (!board.tiles[tile_index].open && board.tiles[tile_index].flag) {
                    SDL_RenderCopyF(gfx.renderer, gfx.textures[TEXTURE_FLAG],
                        NULL,
                        &dest);
                }
            }
        }

        if (!board_generated) {
            SDL_FRect dest = {
                tile_offset_x,
                tile_offset_y,
                board.w * tile_size,
                board.w * tile_size / 256.0 * 128.0,
            };
            SDL_Rect src = {
                0,
                difficulty * 128,
                256,
                128,
            };
            SDL_RenderCopyF(gfx.renderer, gfx.textures[TEXTURE_DIFFICULTIES],
                &src,
                &dest);
        }

        if (game_over || victory) {
            {
                SDL_FRect dest = {
                    tile_offset_x,
                    tile_offset_y,
                    board.w * tile_size,
                    board.h * tile_size,
                };
                SDL_SetRenderDrawColor(gfx.renderer, 128, 128, 128, 64);
                SDL_RenderFillRectF(gfx.renderer, &dest);
            }
            {
                SDL_FRect dest = {
                    tile_offset_x,
                    tile_offset_y,
                    board.w * tile_size,
                    board.w * tile_size / 480.0 * 240.0,
                };
                SDL_RenderCopyF(gfx.renderer, gfx.textures[victory ? TEXTURE_VICTORY : TEXTURE_GAME_OVER],
                    NULL,
                    &dest);
            }
        }

        SDL_RenderPresent(gfx.renderer);
    }

    board_deinit(&board);
    gfx_deinit(&gfx);
}
