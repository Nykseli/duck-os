#ifndef _KERNEL_VGA_H_
#define _KERNEL_VGA_H_

void clear_screen();
void print_string(char* string);
void print_nl();
void print_backspace();

void set_bg_color(int bg);
void set_fg_color(int fg);

#endif
