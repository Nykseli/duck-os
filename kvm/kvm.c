#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "kvm.h"

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

    if (errno == 0) {
        return 1;
    }

    return errno;
}

void vm_init(struct vm* vm)
{
    vm->shared_memory = NULL;
    vm->shared_memory_size = 0;
    vm->kvm_run = NULL;
    vm->kvm_run_size = 0;
    vm->kvm_fd = -1;
    vm->vm_fd = -1;
    vm->vcpu_fd = -1;
    vm->vga.cursor_location = 0;
    vm->irq.irq = -1;
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

    vm_init(vm);
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

int setup_real_mode(struct vm* vm)
{
    if (vm->exec.size < 512) {
        return kvm_error(NULL, "Cannot load MBR from executable\nNeeds to be at least 512 bytes, was: %d\n", vm->exec.size);
    }

    struct kvm_regs regs;
    // make sure all registers are 0
    memset(&regs, 0, sizeof(regs));
    // bit 1 is assumed to be always set
    regs.rflags = 2;
    // bios booloader exists in 0x7C00 - 0x7DFF
    regs.rip = 0x7c00;

    if (ioctl(vm->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
        return kvm_error("KVM_SET_REGS", NULL);
    }

    struct kvm_sregs sregs;
    if (ioctl(vm->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        return kvm_error("KVM_GET_SREGS", NULL);
    }

    // Make sure cs register is 0 since we are not doing any far registers
    // at the start of the boot
    sregs.cs.selector = 0;
    sregs.cs.base = 0;

    if (ioctl(vm->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
        return kvm_error("KVM_SET_SREGS", NULL);
    }

    // load the executable to memory
    memcpy(vm->shared_memory + 0x7c00, vm->exec.data, 512);
    return 0;
}

int setup_bios(struct vm* vm)
{
    /*
        ; 0x011 is valid, 0x010 is unused
        out 0x011, ax
        iret
    */
    const uint8_t bios_code[] = {
        0xe7,
        0x11,
        0xcf
    };

    // TODO: figure a right place for each bios interrupt callback
    // 0xf000 puts the farpointer offset to 0xf0000 which is mapped to "System BIOS"
    // we don't do anything with that address space so we can freely map stuff there
    uint32_t far_pointer = (0xf000 << 16) | 0x1234;
    memcpy(vm->shared_memory + 0xf1234, bios_code, sizeof(bios_code));
    // save interrupt 0x13.
    // interrupt descriptor table / interrupt vector table  is 4 bytes per
    // each 4 bytes is a far poiter
    memcpy(vm->shared_memory + (0x13 * 4), &far_pointer, sizeof(uint32_t));

    return 0;
}

// TODO: bios functions should be put somewhere else
int bios_read(struct vm* vm)
{
    struct kvm_regs regs;
    if (ioctl(vm->vcpu_fd, KVM_GET_REGS, &regs) < 0) {
        return kvm_error("KVM_GET_REGS", NULL);
    }

    // number of 512 bytes sectors (to be read)
    uint8_t al = regs.rax & 0xff;
    // mode = 0x2 read from disk
    uint8_t ah = (regs.rax >> 8) & 0xff;
    // memory address, (ignoring es for now)
    uint16_t bx = regs.rbx;
    // sector: 0x1 boot loader, 0x2 start of kernel
    uint8_t cl = regs.rcx & 0xff;
    // cylinder, don't matter to us?
    uint8_t ch = (regs.rcx >> 8) & 0xff;
    // drive number, we can ignore this
    uint8_t dl = regs.rdx & 0xff;
    // head number
    uint8_t dh = (regs.rdx >> 8) & 0xff;

    if (ah != 0x2)
        return kvm_error(NULL, "Only BIOS read (0x02) is supported\n");

    struct executable kernel;
    kernel.data = vm->exec.data + (512 * (cl - 1));
    kernel.size = vm->exec.size - (512 * (cl - 1));

    int read_size = 512 * al;
    if (kernel.size < read_size)
        read_size = kernel.size;

    memcpy(vm->shared_memory + bx, kernel.data, read_size);
    return 0;
}

int vga_cntl_register(struct vm* vm)
{
    uint8_t* reg = (uint8_t*)vm->shared_memory + VGA_CTRL_REGISTER;
    uint8_t io_value = *(uint8_t*)((void*)(vm->kvm_run) + vm->kvm_run->io.data_offset);

    if (vm->kvm_run->io.direction != KVM_EXIT_IO_OUT)
        return kvm_error(NULL, "Only OUT io direction is supported for VGA_CTRL_REGISTER\n");

    if (io_value != VGA_OFFSET_HIGH && io_value != VGA_OFFSET_LOW)
        return kvm_error(NULL, "Only cursor location registers are supported for CRT ports\n");

    // probably not the best idea to save the value to the memory location itself
    // but it shouldn't be used for anything else currently so we should be able to get away with it
    *reg = io_value;

    return 0;
}

int vga_data_register(struct vm* vm)
{
    uint8_t ctrl_val = *((uint8_t*)vm->shared_memory + VGA_CTRL_REGISTER);
    uint8_t* io_data = (uint8_t*)((void*)(vm->kvm_run) + vm->kvm_run->io.data_offset);

    if (ctrl_val != VGA_OFFSET_HIGH && ctrl_val != VGA_OFFSET_LOW)
        return kvm_error(NULL, "VGA cntrl register needs to be set to high or low, was 0x%02x\n", ctrl_val);

    if (vm->kvm_run->io.direction == KVM_EXIT_IO_OUT) {
        if (ctrl_val == VGA_OFFSET_HIGH)
            vm->vga.cursor_location = (vm->vga.cursor_location & 0x00ff) | ((uint16_t)*io_data << 8);
        else
            vm->vga.cursor_location = (vm->vga.cursor_location & 0xff00) | *io_data;
    } else {
        uint8_t in_value;
        if (ctrl_val == VGA_OFFSET_HIGH)
            in_value = vm->vga.cursor_location >> 8;
        else
            in_value = vm->vga.cursor_location & 0xff;

        *io_data = in_value;
    }

    return 0;
}

int run_vm(struct vm* vm)
{
    int ret;
    while (1) {
        if (ioctl(vm->vcpu_fd, KVM_RUN, 0) < 0)
            return kvm_error("KVM_RUN", NULL);

        switch (vm->kvm_run->exit_reason) {
        case KVM_EXIT_HLT:
            printf("program halted. exiting...\n");
            return 0;
        case KVM_EXIT_IO:
            printf("IO port: %x, data: %x\n", vm->kvm_run->io.port,
                *(int*)((char*)(vm->kvm_run) + vm->kvm_run->io.data_offset));
            if (vm->kvm_run->io.port == 0x11) {
                ret = bios_read(vm);
                if (ret != 0)
                    return ret;
            } else if (vm->kvm_run->io.port == VGA_CTRL_REGISTER) {
                if ((ret = vga_cntl_register(vm)) != 0)
                    return ret;
            } else if (vm->kvm_run->io.port == VGA_DATA_REGISTER) {
                if ((ret = vga_data_register(vm)) != 0)
                    return ret;
            } else if (vm->kvm_run->io.port == 0x60) {
                uint8_t* io_data = (uint8_t*)((void*)(vm->kvm_run) + vm->kvm_run->io.data_offset);
                *io_data = vm->irq.data.keyboard.data;
            }
            break;
        case KVM_EXIT_IRQ_WINDOW_OPEN: {
            struct kvm_interrupt intr;
            // TODO: get the offset mappings and set the irq to 1 on keyboard event
            intr.irq = vm->irq.irq;
            if (ioctl(vm->vcpu_fd, KVM_INTERRUPT, &intr) < 0) {
                return kvm_error("KVM_INTERRUPT", NULL);
            }
            vm->kvm_run->request_interrupt_window = 0;
            break;
        }
        default:
            return kvm_error(NULL, "Got expected KVM_EXIT_HLT (%d)\n", vm->kvm_run->exit_reason);
        }
    }

    return 0;
}

int kvm_vm_setup(struct vm* vm, const char* exec_file)
{
    int ret = 0;
    vm_init(vm);
    executable_init(&vm->exec);

    if ((ret = vm_create(vm)) != 0) {
        return ret;
    }

    if ((ret = read_executable(&vm->exec, exec_file)) != 0) {
        return ret;
    }

    if ((ret = setup_real_mode(vm)) != 0) {
        return ret;
    }

    if ((ret = setup_bios(vm)) != 0) {
        return ret;
    }

    return ret;
}

int kvm_vm_free(struct vm* vm)
{
    vm_free(vm);
    executable_free(&vm->exec);
    return 0;
}

int kvm_vm_run(struct vm* vm)
{
    int ret = 0;
    if ((ret = run_vm(vm)) != 0) {
        return ret;
    }

    return 0;
}

int kvm_vm_interrupt(struct vm* vm, uint32_t irq)
{
    vm->kvm_run->request_interrupt_window = 1;
    return 0;
}

int kvm_vm_send_key(struct vm* vm, enum ps2_scan_code key, bool released)
{
    vm->kvm_run->request_interrupt_window = 1;
    // TODO: set this based in the IRQ remapping
    vm->irq.irq = 33;
    vm->irq.data.keyboard.data = key;
    if (released)
        vm->irq.data.keyboard.data += 0x80;
    return 0;
}
