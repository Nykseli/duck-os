#ifndef _KERNEL_UTIL_H_
#define _KERNEL_UTIL_H_

#include <stdbool.h>
#include <stdint.h>

// in instruction
uint8_t port_byte_in(uint16_t port);

// out instruction
void port_byte_out(uint16_t port, uint8_t data);

int string_length(char s[]);
void reverse(char s[]);
void int_to_string(int n, char str[]);
void append(char s[], char n);
bool backspace(char s[]);
/* K&R
 * Returns <0 if s1<s2, 0 if s1==s2, >0 if s1>s2 */
int compare_string(char s1[], char s2[]);

#endif
