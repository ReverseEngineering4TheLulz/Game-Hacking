#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <inttypes.h>

void WaitForInput()
{
    int chr;
    while ((chr = getchar()) != '\n' && chr != EOF) {}
}

struct _SPEEDHACK_DATA
{
    uintptr_t address[4];
    unsigned char* speed[4];
}SPEEDHACK_DATA;


int main()
{
    SPEEDHACK_DATA.address[0] = 0x004bfd18; // backward  , negative speed   "\xc6\x40\x74\xf8";  | mov byte ptr [eax+74],-08
    SPEEDHACK_DATA.address[1] = 0x004bfcb8; // forward   , positive speed   "\xc6\x40\x74\x08";  | mov byte ptr [eax+74],08
    SPEEDHACK_DATA.address[2] = 0x004bfc08; // right     , negative speed   "\xc6\x40\x75\xf8";  | mov byte ptr [eax+75],-08
    SPEEDHACK_DATA.address[3] = 0x004bfc58; // left      , positive speed   "\xc6\x40\x75\08";   | mov byte ptr [eax+75],08

    SPEEDHACK_DATA.speed[0] = "\xf8"; // backward speed
    SPEEDHACK_DATA.speed[1] = "\x08"; // forward speed
    SPEEDHACK_DATA.speed[2] = "\xf8"; // right speed
    SPEEDHACK_DATA.speed[3] = "\x08"; // left speed

    HWND hwnd = FindWindow(0, L"AssaultCube");

    if (hwnd == 0)
    {
        printf("[FindWindow] error code: %lu\n", GetLastError());
        WaitForInput();
        return 0;
    }

    unsigned long pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == 0)
    {
        printf("[GetWindowThreadProcessId] error code: %lu\n", GetLastError());
        WaitForInput();
        return 0;
    }

    printf("PID: %lu\n", pid);

    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

    if (process_handle == NULL)
    {
        printf("The process could not be found. Error code: %lu\n", GetLastError());
        WaitForInput();
        return 0;
    }

    printf("ProcessHandle: %p\n", process_handle);

    for (int i = 0; i < 4; i++)
    {
        if (WriteProcessMemory(process_handle, (void*)(SPEEDHACK_DATA.address[i] + 3), SPEEDHACK_DATA.speed[i], strlen(SPEEDHACK_DATA.speed[i]), 0) == false)
        {
            printf("[WriteProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
            WaitForInput();
            return 0;
        }
    }

    return 0;
}
