//doomgeneric for soso os

#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

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
}

void DG_SleepMs(uint32_t ms)
{
    usleep(ms * 1000);
}

uint32_t DG_GetTicksMs()
{
    struct timeval  tp;
    struct timezone tzp;

    gettimeofday(&tp, &tzp);

    return (tp.tv_sec * 1000) + (tp.tv_usec / 1000); /* return milliseconds */
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
    int8_t key;
    int ret = read(TO_DOOM_KEY_FD, &key, 1);
    if (ret == -1) {
        perror("failed to read key");
        exit(1);
    }
    if (ret == 0) {
        fprintf(stderr, "unexpected eof when reading key\n");
        exit(1);
    }
    switch ((int) key) {
        case (int) TOTEM_END_OF_FRAME:
            return 0;
        case (int) TOTEM_UP:
            *pressed = 1;
            *doomKey = KEY_UPARROW;
            break;
        case -((int) TOTEM_UP):
            *pressed = 0;
            *doomKey = KEY_UPARROW;
            break;
        case (int) TOTEM_DOWN:
            *pressed = 1;
            *doomKey = KEY_DOWNARROW;
            break;
        case -(int)  TOTEM_DOWN:
            *pressed = 0;
            *doomKey = KEY_DOWNARROW;
            break;
        case (int) TOTEM_LEFT:
            *pressed = 1;
            *doomKey = KEY_STRAFE_L;
            break;
        case -(int) TOTEM_LEFT:
            *pressed = 0;
            *doomKey = KEY_STRAFE_L;
            break;
        case (int) TOTEM_RIGHT:
            *pressed = 1;
            *doomKey = KEY_STRAFE_R;
            break;
        case -(int) TOTEM_RIGHT:
            *pressed = 0;
            *doomKey = KEY_STRAFE_R;
            break;
        case (int) TOTEM_TURNLEFT:
            *pressed = 1;
            *doomKey = KEY_LEFTARROW;
            break;
        case -(int) TOTEM_TURNLEFT:
            *pressed = 0;
            *doomKey = KEY_LEFTARROW;
            break;
        case (int) TOTEM_TURNRIGHT:
            *pressed = 1;
            *doomKey = KEY_RIGHTARROW;
            break;
        case -(int) TOTEM_TURNRIGHT:
            *pressed = 0;
            *doomKey = KEY_RIGHTARROW;
            break;
        case (int) TOTEM_FIRE:
            *pressed = 1;
            *doomKey = KEY_FIRE;
            break;
        case -(int) TOTEM_FIRE:
            *pressed = 0;
            *doomKey = KEY_FIRE;
            break;
        case (int) TOTEM_USE:
            *pressed = 1;
            *doomKey = KEY_USE;
            break;
        case -(int) TOTEM_USE:
            *pressed = 0;
            *doomKey = KEY_USE;
            break;
        case (int) TOTEM_ESC:
            *pressed = 1;
            *doomKey = KEY_ESCAPE;
            break;
        case -(int)  TOTEM_ESC:
            *pressed = 0;
            *doomKey = KEY_ESCAPE;
            break;
        case (int) TOTEM_ENTER:
            *pressed = 1;
            *doomKey = KEY_ENTER;
            break;
        case -(int) TOTEM_ENTER:
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
    return 1;
}

void DG_SetWindowTitle(const char * title)
{
}
