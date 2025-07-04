/* This will force us to create a kernel entry function instead of jumping to kernel.c:0x00 */
void dummy_test_entrypoint()
{
}

void main()
{
    char* video_memory = (char*)0xb8000;
    *video_memory = 'D';
    *(video_memory + 2) = 'u';
    *(video_memory + 4) = 'c';
    *(video_memory + 6) = 'k';
}
