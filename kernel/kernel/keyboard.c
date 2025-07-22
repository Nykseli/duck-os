#include "keyboard.h"
#include "../cpu/isr.h"
#include "shell.h"
#include "util.h"
#include "vga.h"

#define BACKSPACE 0x0E
#define LSHIFT 0x2A
#define ENTER 0x1C

static char key_buffer[256];
static uint8_t shit_down = 0;

#define SC_MAX 57

const char* sc_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6",
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E",
    "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`",
    "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".",
    "/", "RShift", "Keypad *", "LAlt", "Spacebar" };
const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', '?', '?', 'q', 'w', 'e', 'r', 't', 'y',
    'u', 'i', 'o', 'p', '[', ']', '?', '?', 'a', 's', 'd', 'f', 'g',
    'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' ' };
const char sc_ascii_upper[] = { '?', '?', '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '_', '+', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y',
    'U', 'I', 'O', 'P', '{', '}', '?', '?', 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ':', '"', '|', '?', '\\', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', '?', '?', '?', ' ' };

static void keyboard_callback(registers_t* regs)
{
    uint8_t scancode = port_byte_in(0x60);
    uint8_t keyup = scancode & 0x80;
    uint8_t base_key = scancode & 0x7f;

    if (base_key == LSHIFT) {
        if (keyup)
            shit_down = 0;
        else
            shit_down = 1;
        return;
    }

    if (scancode > SC_MAX)
        return;
    if (scancode == BACKSPACE) {
        if (backspace(key_buffer)) {
            print_backspace();
        }
    } else if (scancode == ENTER) {
        print_nl();
        execute_command(key_buffer);
        key_buffer[0] = '\0';
    } else {
        char letter;
        if (shit_down)
            letter = sc_ascii_upper[(int)scancode];
        else
            letter = sc_ascii[(int)scancode];
        append(key_buffer, letter);
        char str[2] = { letter, '\0' };
        print_string(str);
    }
}

void init_keyboard()
{
    register_interrupt_handler(IRQ1, keyboard_callback);
}
