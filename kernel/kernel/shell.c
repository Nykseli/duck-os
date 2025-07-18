#include "shell.h"
#include "vga.h"

void execute_fcol(char* input)
{
    // offset to "FCOL "
    char* number = input + 5;
    // TODO: handle errors
    int value = string_to_int(number);
    set_fg_color(value);
}

void execute_bcol(char* input)
{
    // offset to "bcol "
    char* number = input + 5;
    // TODO: handle errors
    int value = string_to_int(number);
    set_bg_color(value);
}

void execute_command(char* input)
{
    if (compare_string(input, "EXIT") == 0) {
        print_string("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    } else if (compare_string(input, "CLEAR") == 0) {
        clear_screen();
        print_string("> ");
        return;
    } else if (string_starts_with(input, "FCOL ") == 0) {
        execute_fcol(input);
        print_string("> ");
        return;
    } else if (string_starts_with(input, "BCOL ") == 0) {
        execute_bcol(input);
        print_string("> ");
        return;
    }

    print_string("Unknown command: ");
    print_string(input);
    print_string("\n> ");
}
