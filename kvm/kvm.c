#include <errno.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int kvm_error(const char* pmsg, const char* efmt, ...)
{
    if (efmt != NULL) {
        va_list args;
        va_start(args, efmt);
        vfprintf(stderr, efmt, args);
        va_end(args);
    }

    if (pmsg != NULL)
        perror(pmsg);

    return errno;
}

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
};

void vm_init(struct vm* vm)
{
    vm->shared_memory = NULL;
    vm->shared_memory_size = 0;
    vm->kvm_run = NULL;
    vm->kvm_run_size = 0;
    vm->kvm_fd = -1;
    vm->vm_fd = -1;
    vm->vcpu_fd = -1;
}

void vm_free(struct vm* vm)
{
    if (vm->kvm_run != NULL) {
        munmap(vm->kvm_run, vm->kvm_run_size);
    }

    if (vm->kvm_run != NULL) {
        munmap(vm->kvm_run, vm->kvm_run_size);
    }

    if (vm->kvm_fd != -1) {
        close(vm->kvm_fd);
    }

    if (vm->vm_fd != -1) {
        close(vm->vm_fd);
    }

    if (vm->vcpu_fd != -1) {
        close(vm->vcpu_fd);
    }
}

int vm_create_error(struct vm* vm, const char* pmsg, const char* efmt, ...)
{
    va_list args;
    if (efmt != NULL)
        va_start(args, efmt);
    kvm_error(pmsg, efmt, args);
    va_end(args);
    vm_free(vm);
    return 1;
}

int create_kvm(struct vm* vm)
{
    vm->kvm_fd = open("/dev/kvm", O_RDWR);
    if (vm->kvm_fd < 0) {
        return vm_create_error(vm, "Failed to open /dev/kvm", NULL);
    }

    int api_ver = ioctl(vm->kvm_fd, KVM_GET_API_VERSION, 0);
    if (api_ver < 0) {
        return vm_create_error(vm, "KVM_GET_API_VERSION", NULL);
    }

    if (api_ver != KVM_API_VERSION) {
        return vm_create_error(vm, NULL, "Got KVM api version %d, expected %d\n",
            api_ver, KVM_API_VERSION);
    }

    vm->vm_fd = ioctl(vm->kvm_fd, KVM_CREATE_VM, 0);
    if (vm->vm_fd < 0) {
        return vm_create_error(vm, "KVM_CREATE_VM", NULL);
    }

    // 0x200000 is the amount x86 real mode can use so lets use that for now
    size_t mem_size = 0x200000;
    vm->shared_memory = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->shared_memory == MAP_FAILED) {
        return vm_create_error(vm, "Failed to map shared memory", NULL);
    }
    vm->shared_memory_size = mem_size;

    madvise(vm->shared_memory, mem_size, MADV_MERGEABLE);

    struct kvm_userspace_memory_region memreg;
    memreg.slot = 0;
    memreg.flags = 0;
    memreg.guest_phys_addr = 0;
    memreg.memory_size = vm->shared_memory_size;
    memreg.userspace_addr = (uint64_t)vm->shared_memory;

    if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
        return vm_create_error(vm, "KVM_SET_USER_MEMORY_REGION", NULL);
    }

    return 0;
}

int create_vcpu(struct vm* vm)
{

    vm->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
    if (vm->vcpu_fd < 0) {
        return vm_create_error(vm, "KVM_CREATE_VCPU", NULL);
    }

    ssize_t vcpu_mmap_size = (ssize_t)ioctl(vm->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (vcpu_mmap_size <= 0) {
        return vm_create_error(vm, "KVM_GET_VCPU_MMAP_SIZE", NULL);
    }

    vm->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
        vm->vcpu_fd, 0);
    if (vm->kvm_run == MAP_FAILED) {
        return vm_create_error(vm, "Failed to map kvm_run", NULL);
    }
    vm->kvm_run_size = (size_t)vcpu_mmap_size;

    return 0;
}

int vm_create(struct vm* vm)
{
    int ret = 0;
    if ((ret = create_kvm(vm)) != 0) {
        return ret;
    }

    if ((ret = create_vcpu(vm)) != 0) {
        return ret;
    }

    return 0;
}

struct executable {
    uint8_t* data;
    size_t size;
};

void executable_init(struct executable* exec)
{
    exec->data = NULL;
    exec->size = 0;
}

void executable_free(struct executable* exec)
{
    if (exec->data != NULL)
        free(exec->data);
    executable_init(exec);
}

int read_executable(struct executable* exec, const char* filename)
{
    int fd = open(filename, O_RDONLY);

    if (fd < 0) {
        return kvm_error("Cannot open file", "Cannot open file '%s'\n", filename);
    }

    struct stat st;
    if (fstat(fd, &st)) {
        return kvm_error("Cannot stat file.", "Cannot stat file '%s'\n", filename);
    }

    uint8_t* filedata = malloc(st.st_size);
    if (read(fd, filedata, st.st_size) < 0) {
        executable_free(exec);
        return kvm_error("Cannot read file", "Cannot read file '%s'\n", filename);
    }
    close(fd);

    exec->data = filedata;
    exec->size = st.st_size;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Give executable as an argument\n");
        return 1;
    }

    int ret = 0;
    struct vm vm;
    vm_init(&vm);

    if ((ret = vm_create(&vm)) != 0) {
        return ret;
    }

    struct executable exec;
    executable_init(&exec);
    if ((ret = read_executable(&exec, argv[1])) != 0) {
        vm_free(&vm);
        return ret;
    }

    vm_free(&vm);
    executable_free(&exec);
    return 0;
}
