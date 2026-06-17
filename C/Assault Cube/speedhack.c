#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <inttypes.h>

#define VK_O 0x4F
#define VK_P 0x50

typedef struct SPEEDHACK_DATA
{
    uintptr_t address[4];
    unsigned char speed[4];
}SPEEDHACK_DATA;

void WaitForInput()
{
    int chr;
    while ((chr = getchar()) != '\n' && chr != EOF) {}
}

void GetSpeed(HANDLE process_handle, SPEEDHACK_DATA* speedhack_data)
{
    int count = sizeof(speedhack_data->speed);

    for (int i = 0; i < count; i++)
    {
        if (ReadProcessMemory(process_handle, (void*)(speedhack_data->address[i] + 3), &speedhack_data->speed[i], sizeof(speedhack_data->speed[i]), 0) == false)
        {
            printf("[ReadProcessMemory] error: %d", GetLastError());
            WaitForInput();
            return;
        }
    }
}

void IncreaseSpeed(HANDLE process_handle, SPEEDHACK_DATA* speedhack_data)
{
    int count = sizeof(speedhack_data->speed) / sizeof(unsigned char);

    for (int i = 0; i < count; i++)
    {
        if (speedhack_data->speed[i] & -128) {
            speedhack_data->speed[i] -= 1;
        }

        else {
            speedhack_data->speed[i] += 1;
        }

        if (WriteProcessMemory(process_handle, (void*)(speedhack_data->address[i] + 3), &speedhack_data->speed[i], sizeof(speedhack_data->speed[i]), 0) == false)
        {
            printf("[WriteProcessMemory] error: %d", GetLastError());
            WaitForInput();
            return;
        }
    }
}

void DecreaseSpeed(HANDLE process_handle, SPEEDHACK_DATA* speedhack_data)
{
    int count = sizeof(speedhack_data->speed) / sizeof(unsigned char);

    for (int i = 0; i < count; i++)
    {
        if (speedhack_data->speed[i] != 0xff && speedhack_data->speed[i] != 1)
        {
            if (speedhack_data->speed[i] & -128) {  // -128 = 0x80
                speedhack_data->speed[i] += 1;
            }

            else {
                speedhack_data->speed[i] -= 1;
            }

            printf("::addr: %x | speed: %x\n", speedhack_data->address[i], speedhack_data->speed[i]);
            WriteProcessMemory(process_handle, (void*)(speedhack_data->address[i] + 3), &speedhack_data->speed[i], sizeof(speedhack_data->speed[i]), 0);
        }

    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{

	// Speedhack for AssaultCube 1.3.0.2 Lockdown Edition
	// Download: https://github.com/assaultcube/AC/releases/download/v1.3.0.2/AssaultCube_v1.3.0.2_LockdownEdition.exe

    SPEEDHACK_DATA speedhack_data;

    speedhack_data.address[0] = 0x004bfd18; // forward    , negative speed   "\xc6\x40\x74\xf8";
    speedhack_data.address[1] = 0x004bfcb8; // backwards , positive speed   "\xc6\x40\x74\x08";
    speedhack_data.address[2] = 0x004bfc08; // right      , negative speed   "\xc6\x40\x75\xf8";
    speedhack_data.address[3] = 0x004bfc58; // left       , positive speed   "\xc6\x40\x75\08";

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

    int keys[] = { VK_O, VK_P };
    bool key_was_pressed[2] = { false, false };

    while (1)
    {
        for (int i = 0; i < 2; i++)
        {
            bool key_is_down = (GetAsyncKeyState(keys[i]) & 0x8000) != 0;

            if (key_is_down == 0 && key_was_pressed[i])
            {
                if (keys[i] == VK_O)
                {
                    printf("o was pressed\n");
                    GetSpeed(process_handle, &speedhack_data);
                    IncreaseSpeed(process_handle, &speedhack_data);

                }

                else if (keys[i] == VK_P)
                {
                    printf("o was pressed\n");
                    GetSpeed(process_handle, &speedhack_data);
                    DecreaseSpeed(process_handle, &speedhack_data);
                }
            }

            key_was_pressed[i] = key_is_down;
        }
    }

    return 0;
}
