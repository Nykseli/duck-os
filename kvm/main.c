#include <pthread.h>
#include <signal.h>

#include "kvm.h"
#include "window.h"

void* kvm_vm_thread(void* arg)
{
    struct vm* vm = (struct vm*)arg;
    int ret = 0;

    // Handle the case where kvm exits
    if ((ret = kvm_vm_run(vm)) != 0)
        return NULL;

    return NULL;
};

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        printf("Give executable as an argument\n");
        return 1;
    }

    int ret = 0;
    struct vm vm;
    struct kvm_window window;
    pthread_t kvm_thead;

    if ((ret = kvm_window_init(&window)) != 0)
        goto main_end;
    if ((ret = kvm_vm_setup(&vm, argv[1])) != 0)
        goto main_end;

    pthread_create(&kvm_thead, NULL, kvm_vm_thread, &vm);

    if ((ret = kvm_window_run(&window)) != 0)
        goto main_end;

    pthread_kill(kvm_thead, SIGTERM);

main_end:
    kvm_vm_free(&vm);
    if ((ret = kvm_window_free(&window)) != 0)
        return ret;
    return 0;
}
