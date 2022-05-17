//doomgeneric for soso os

#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>
#include <SDL.h>

#include <fcntl.h>
#include <sys/ioctl.h>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture;

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

// down = positive, up = negative, character = lower ascii
enum TotemKey {
    TOTEM_END_OF_FRAME = 0,
    TOTEM_UP = 1,
    TOTEM_DOWN = 2,
    TOTEM_LEFT = 3,
    TOTEM_RIGHT = 4,
    TOTEM_TURNLEFT = 5,
    TOTEM_TURNRIGHT = 6,
    TOTEM_FIRE = 7,
    TOTEM_USE = 8,
    TOTEM_ESC = 9,
    TOTEM_ENTER = 10,
};

static int TO_DOOM_KEY_FD;
static int FROM_DOOM_FRAME_BUFFER_FD;

static unsigned char convertToDoomKey(unsigned int key){
  switch (key)
    {
    case SDLK_RETURN:
      key = KEY_ENTER;
      break;
    case SDLK_ESCAPE:
      key = KEY_ESCAPE;
      break;
    case SDLK_LEFT:
      key = KEY_LEFTARROW;
      break;
    case SDLK_RIGHT:
      key = KEY_RIGHTARROW;
      break;
    case SDLK_UP:
      key = KEY_UPARROW;
      break;
    case SDLK_DOWN:
      key = KEY_DOWNARROW;
      break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      key = KEY_FIRE;
      break;
    case SDLK_SPACE:
      key = KEY_USE;
      break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      key = KEY_RSHIFT;
      break;
    default:
      key = tolower(key);
      break;
    }

  return key;
}

static void addKeyToQueue(int pressed, unsigned int keyCode){
  unsigned char key = convertToDoomKey(keyCode);

  unsigned short keyData = (pressed << 8) | key;

  s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
  s_KeyQueueWriteIndex++;
  s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}
static void handleKeyInput(){
  SDL_Event e;
  while (SDL_PollEvent(&e)){
    if (e.type == SDL_QUIT){
      puts("Quit requested");
      atexit(SDL_Quit);
      exit(1);
    }
    if (e.type == SDL_KEYDOWN) {
      //KeySym sym = XKeycodeToKeysym(s_Display, e.xkey.keycode, 0);
      //printf("KeyPress:%d sym:%d\n", e.xkey.keycode, sym);
      addKeyToQueue(1, e.key.keysym.sym);
    } else if (e.type == SDL_KEYUP) {
      //KeySym sym = XKeycodeToKeysym(s_Display, e.xkey.keycode, 0);
      //printf("KeyRelease:%d sym:%d\n", e.xkey.keycode, sym);
      addKeyToQueue(0, e.key.keysym.sym);
    }
  }
}


void DG_Init(){
    char *to_doom_key_fifo = getenv("TO_DOOM_KEY_FIFO");
    if (to_doom_key_fifo == NULL) {
        fprintf(stderr, "did not specifly TO_DOOM_KEY_FIFO env var\n");
        exit(1);
    }
    char *from_doom_frame_buffer = getenv("FROM_DOOM_FRAME_BUFFER");
    if (from_doom_frame_buffer == NULL) {
        fprintf(stderr, "did not specify FROM_DOOM_FRAME_BUFFER env var\n");
        exit(1);
    }
    TO_DOOM_KEY_FD = open(to_doom_key_fifo, O_RDONLY);
    if (TO_DOOM_KEY_FD == -1) {
        perror("failed to open to_doom_key fifo");
        exit(1);
    }
    FROM_DOOM_FRAME_BUFFER_FD = open(from_doom_frame_buffer, O_WRONLY);
    if (FROM_DOOM_FRAME_BUFFER_FD == -1) {
        perror("failed to open from_doom_frame_buffer fifo");
        exit(1);
    }
  window = SDL_CreateWindow("DOOM",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            DOOMGENERIC_RESX,
                            DOOMGENERIC_RESY,
                            SDL_WINDOW_SHOWN
                            );

  // Setup renderer
  renderer =  SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED);
  // Clear winow
  SDL_RenderClear( renderer );
  // Render the rect to the screen
  SDL_RenderPresent(renderer);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

static void write_all(int fd, void *buf, size_t len) {
    size_t bytes_written = 0;
    while (bytes_written < len) {
        ssize_t ret = write(fd, buf + bytes_written, len - bytes_written);
        if (ret == -1) {
            perror("failed to write");
            exit(1);
        }
        if (ret == 0) {
            fprintf(stderr, "unexpected EOF on write\n");
            exit(1);
        }
        bytes_written += ret;
    }
}

void DG_DrawFrame()
{
    write_all(FROM_DOOM_FRAME_BUFFER_FD, DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

  SDL_UpdateTexture(texture, NULL, DG_ScreenBuffer, DOOMGENERIC_RESX*sizeof(uint32_t));

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);

  handleKeyInput();
}

void DG_SleepMs(uint32_t ms)
{
  SDL_Delay(ms);
}

uint32_t DG_GetTicksMs()
{
  return SDL_GetTicks();
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
    int sz;
    int ioctl_ret = ioctl(TO_DOOM_KEY_FD, FIONREAD, &sz);
    if (ioctl_ret == -1) {
        perror("failed to ioctl key");
        exit(1);
    }
    if (sz == 0) {
        return 0;
    }
    char key;
    int ret = read(TO_DOOM_KEY_FD, &key, 1);
    if (ret == -1) {
        perror("failed to read key");
        exit(1);
    }
    if (ret == 0) {
        fprintf(stderr, "unexpected eof when reading key\n");
        exit(1);
    }
//    printf("got key %i\n", key);
    switch (key) {
        case TOTEM_END_OF_FRAME:
            return 0;
        case TOTEM_UP:
            *pressed = 1;
            *doomKey = KEY_UPARROW;
            break;
        case -(char) TOTEM_UP:
            *pressed = 0;
            *doomKey = KEY_UPARROW;
            break;
        case TOTEM_DOWN:
            *pressed = 1;
            *doomKey = KEY_DOWNARROW;
            break;
        case -(char) TOTEM_DOWN:
            *pressed = 0;
            *doomKey = KEY_DOWNARROW;
            break;
        case TOTEM_LEFT:
            *pressed = 1;
            *doomKey = KEY_STRAFE_L;
            break;
        case -(char) TOTEM_LEFT:
            *pressed = 0;
            *doomKey = KEY_STRAFE_L;
            break;
        case TOTEM_RIGHT:
            *pressed = 1;
            *doomKey = KEY_STRAFE_R;
            break;
        case -(char) TOTEM_RIGHT:
            *pressed = 0;
            *doomKey = KEY_STRAFE_R;
            break;
        case TOTEM_TURNLEFT:
            *pressed = 1;
            *doomKey = KEY_LEFTARROW;
            break;
        case -(char) TOTEM_TURNLEFT:
            *pressed = 0;
            *doomKey = KEY_LEFTARROW;
            break;
        case TOTEM_TURNRIGHT:
            *pressed = 1;
            *doomKey = KEY_RIGHTARROW;
            break;
        case -(char) TOTEM_TURNRIGHT:
            *pressed = 0;
            *doomKey = KEY_RIGHTARROW;
            break;
        case TOTEM_FIRE:
            *pressed = 1;
            *doomKey = KEY_FIRE;
            break;
        case -(char) TOTEM_FIRE:
            *pressed = 0;
            *doomKey = KEY_FIRE;
            break;
        case TOTEM_USE:
            *pressed = 1;
            *doomKey = KEY_USE;
            break;
        case -(char) TOTEM_USE:
            *pressed = 0;
            *doomKey = KEY_USE;
            break;
        case TOTEM_ESC:
            *pressed = 1;
            *doomKey = KEY_ESCAPE;
            break;
        case -(char) TOTEM_ESC:
            *pressed = 0;
            *doomKey = KEY_ESCAPE;
            break;
        case TOTEM_ENTER:
            *pressed = 1;
            *doomKey = KEY_ENTER;
            break;
        case -(char) TOTEM_ENTER:
            *pressed = 0;
            *doomKey = KEY_ENTER;
            break;
        default:
            if ((key >= '0' && key <= '9') || (key >= 'a' && key <= 'z')) {
                *pressed = 1;
                *doomKey = key;
            } else if ((key >= -'9' && key <= -'0') || (key >= -'z' && key <= -'a')) {
                *pressed = 0;
                *doomKey = -key;
            }
            break;
    }
//    printf("pressed = %i, doomKey = %i\n", *pressed, *doomKey);
    return 1;
//  if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex){
//    //key queue is empty
//    return 0;
//  }else{
//    unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
//    s_KeyQueueReadIndex++;
//    s_KeyQueueReadIndex %= KEYQUEUE_SIZE;
//
//    *pressed = keyData >> 8;
//    *doomKey = keyData & 0xFF;
//
//    return 1;
//  }
//
//  return 0;
}

void DG_SetWindowTitle(const char * title)
{
  if (window != NULL){
    SDL_SetWindowTitle(window, title);
  }
}
