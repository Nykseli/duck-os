#ifndef _KVM_KVM_H_
#define _KVM_KVM_H_

#include <linux/kvm.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define VGA_CTRL_REGISTER 0x3d4
#define VGA_DATA_REGISTER 0x3d5
#define VGA_OFFSET_LOW 0x0f
#define VGA_OFFSET_HIGH 0x0e

// https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_1
enum ps2_scan_code {
    PS2_ERROR = 0x0,
    PS2_ESC = 0x1,
    PS2_Num_1 = 0x2,
    PS2_Num_2 = 0x3,
    PS2_Num_3 = 0x4,
    PS2_Num_4 = 0x5,
    PS2_Num_5 = 0x6,
    PS2_Num_6 = 0x7,
    PS2_Num_7 = 0x8,
    PS2_Num_8 = 0x9,
    PS2_Num_9 = 0x0A,
    PS2_Num_0 = 0x0B,
    PS2_Minus_Sign = 0x0C,
    PS2_Plus_sign = 0x0D,
    PS2_Backspace = 0x0E,
    PS2_Tab = 0x0F,
    PS2_Q = 0x10,
    PS2_W = 0x11,
    PS2_E = 0x12,
    PS2_R = 0x13,
    PS2_T = 0x14,
    PS2_Y = 0x15,
    PS2_U = 0x16,
    PS2_I = 0x17,
    PS2_O = 0x18,
    PS2_P = 0x19,
    PS2_Braclet_Open = 0x1A,
    PS2_Braclet_Close = 0x1B,
    PS2_ENTER = 0x1C,
    PS2_LCtrl = 0x1D,
    PS2_A = 0x1E,
    PS2_S = 0x1F,
    PS2_D = 0x20,
    PS2_F = 0x21,
    PS2_G = 0x22,
    PS2_H = 0x23,
    PS2_J = 0x24,
    PS2_K = 0x25,
    PS2_L = 0x26,
    PS2_Semi_Colon = 0x27,
    PS2_Quote = 0x28,
    PS2_Back_Tick = 0x29,
    PS2_LShift = 0x2A,
    PS2_Backlash = 0x2B,
    PS2_Z = 0x2C,
    PS2_X = 0x2D,
    PS2_C = 0x2E,
    PS2_V = 0x2F,
    PS2_B = 0x30,
    PS2_N = 0x31,
    PS2_M = 0x32,
    PS2_Comma = 0x33,
    PS2_Dot = 0x34,
    PS2_Slash = 0x35,
    PS2_Rshift = 0x36,
    PS2_Keypad_Star = 0x37,
    PS2_LAlt = 0x38,
    PS2_Space = 0x39,
};

/**
 * http://www.osdever.net/FreeVGA/vga/portidx.htm
 * 3D4h -- CRTC Controller Address Register
 * 3D5h -- CRTC Controller Data Register
 */
struct kvm_vga {
    uint16_t cursor_location;
};

struct executable {
    uint8_t* data;
    size_t size;
};

struct vm_irq {
    int32_t irq;
    union {
        struct {
            uint8_t data;
        } keyboard;
    } data;
};

struct vm {
    /**
     * fd for /dev/kvm
     */
    int kvm_fd;
    /**
     * fd for KVM_CREATE_VM
     */
    int vm_fd;

    /**
     * Shared memory between KVM and userspace
     */
    void* shared_memory;
    size_t shared_memory_size;

    /**
     * fd for KVM_CREATE_VCPU
     */
    int vcpu_fd;
    struct kvm_run* kvm_run;
    size_t kvm_run_size;

    struct executable exec;
    struct kvm_vga vga;
    struct vm_irq irq;
};

int kvm_vm_setup(struct vm* vm, const char* exec_file);
int kvm_vm_free(struct vm* vm);
int kvm_vm_run(struct vm* vm);
int kvm_vm_interrupt(struct vm* vm, uint32_t irq);
int kvm_vm_send_key(struct vm* vm, enum ps2_scan_code key, bool released);

#endif
