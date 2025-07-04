#ifndef _KVM_KVM_H_
#define _KVM_KVM_H_

#include <linux/kvm.h>
#include <stdint.h>
#include <unistd.h>

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
};

int kvm_vm_setup(struct vm* vm, const char* exec_file);
int kvm_vm_free(struct vm* vm);
int kvm_vm_run(struct vm* vm);

#endif
