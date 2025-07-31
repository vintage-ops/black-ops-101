#include <windows.h>
#include <stdio.h>

int main() {
    // Target process ID (replace with actual PID)
    DWORD pid = 1234;
    // Path to DLL
    const char* dllPath = "C:\\path\\to\\mydll.dll";
    
    // Open target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        printf("Failed to open process\n");
        return 1;
    }
    
    // Allocate memory in target process
    LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!remoteMem) {
        printf("Failed to allocate memory\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    // Write DLL path to target process
    if (!WriteProcessMemory(hProcess, remoteMem, dllPath, strlen(dllPath) + 1, NULL)) {
        printf("Failed to write memory\n");
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    
    // Get address of LoadLibraryA
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    
    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remoteMem, 0, NULL);
    if (!hThread) {
        printf("Failed to create thread\n");
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    
    // Clean up
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    printf("DLL injected successfully\n");
    return 0;
}