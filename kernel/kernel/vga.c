#include <stddef.h>
#include <stdint.h>

#include "util.h"

#define VGA_CTRL_REGISTER 0x3d4
#define VGA_DATA_REGISTER 0x3d5
#define VGA_OFFSET_LOW 0x0f
#define VGA_OFFSET_HIGH 0x0e

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f

void set_cursor(int offset)
{
    offset /= 2;
    port_byte_out(VGA_CTRL_REGISTER, VGA_OFFSET_HIGH);
    port_byte_out(VGA_DATA_REGISTER, (uint8_t)(offset >> 8));
    port_byte_out(VGA_CTRL_REGISTER, VGA_OFFSET_LOW);
    port_byte_out(VGA_DATA_REGISTER, (uint8_t)(offset & 0xff));
}

int get_cursor()
{
    port_byte_out(VGA_CTRL_REGISTER, VGA_OFFSET_HIGH);
    int offset = port_byte_in(VGA_DATA_REGISTER) << 8;
    port_byte_out(VGA_CTRL_REGISTER, VGA_OFFSET_LOW);
    offset += port_byte_in(VGA_DATA_REGISTER);
    return offset * 2;
}

void set_char_at_video_memory(char character, int offset)
{
    uint8_t* vidmem = (uint8_t*)VIDEO_ADDRESS;
    vidmem[offset] = character;
    vidmem[offset + 1] = WHITE_ON_BLACK;
}

void memory_copy(char* source, char* dest, int nbytes)
{
    int idx;
    for (idx = 0; idx < nbytes; idx++) {
        *(dest + idx) = *(source + idx);
    }
}

int get_row_from_offset(int offset)
{
    return offset / (2 * MAX_COLS);
}

int get_offset(int col, int row)
{
    return 2 * (row * MAX_COLS + col);
}

int move_offset_to_new_line(int offset)
{
    return get_offset(0, get_row_from_offset(offset) + 1);
}

int scroll_ln(int offset)
{
    memory_copy(
        (char*)(size_t)(get_offset(0, 1) + VIDEO_ADDRESS),
        (char*)(size_t)(get_offset(0, 0) + VIDEO_ADDRESS),
        MAX_COLS * (MAX_ROWS - 1) * 2);

    for (int col = 0; col < MAX_COLS; col++) {
        set_char_at_video_memory(' ', get_offset(col, MAX_ROWS - 1));
    }

    return offset - 2 * MAX_COLS;
}

void print_string(char* string)
{
    int offset = get_cursor();
    int idx = 0;
    while (string[idx] != 0) {
        if (offset >= MAX_ROWS * MAX_COLS * 2) {
            offset = scroll_ln(offset);
        }
        if (string[idx] == '\n') {
            offset = move_offset_to_new_line(offset);
        } else {
            set_char_at_video_memory(string[idx], offset);
            offset += 2;
        }
        idx++;
    }
    set_cursor(offset);
}

void clear_screen()
{
    for (int idx = 0; idx < MAX_COLS * MAX_ROWS; idx++) {
        set_char_at_video_memory(' ', idx * 2);
    }
    set_cursor(get_offset(0, 0));
}

void print_nl()
{
    int newOffset = move_offset_to_new_line(get_cursor());
    if (newOffset >= MAX_ROWS * MAX_COLS * 2) {
        newOffset = scroll_ln(newOffset);
    }
    set_cursor(newOffset);
}
