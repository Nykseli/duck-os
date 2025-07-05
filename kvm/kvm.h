#ifndef _KVM_KVM_H_
#define _KVM_KVM_H_

#include <linux/kvm.h>
#include <stdint.h>
#include <unistd.h>

#define VGA_CTRL_REGISTER 0x3d4
#define VGA_DATA_REGISTER 0x3d5
#define VGA_OFFSET_LOW 0x0f
#define VGA_OFFSET_HIGH 0x0e

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
};

int kvm_vm_setup(struct vm* vm, const char* exec_file);
int kvm_vm_free(struct vm* vm);
int kvm_vm_run(struct vm* vm);

#endif
