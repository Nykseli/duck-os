#ifndef _KERNEL_UTIL_H_
#define _KERNEL_UTIL_H_

#include <stdint.h>

// in instruction
uint8_t port_byte_in(uint16_t port);

// out instruction
void port_byte_out(uint16_t port, uint8_t data);

int string_length(char s[]);
void reverse(char s[]);
void int_to_string(int n, char str[]);

#endif
