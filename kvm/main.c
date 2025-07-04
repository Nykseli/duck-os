#include "window.h"

int main()
{
    int ret = 0;
    struct kvm_window window;

    if ((ret = kvm_window_init(&window)) != 0)
        return ret;
    if ((ret = kvm_window_run(&window)) != 0)
        return ret;
    if ((ret = kvm_window_free(&window)) != 0)
        return ret;

    return 0;
}
