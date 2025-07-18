#include "shell.h"
#include "vga.h"

void execute_command(char* input)
{
    if (compare_string(input, "EXIT") == 0) {
        print_string("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    } else if (compare_string(input, "CLEAR") == 0) {
        clear_screen();
        print_string("> ");
        return;
    }
    print_string("Unknown command: ");
    print_string(input);
    print_string("\n> ");
}
