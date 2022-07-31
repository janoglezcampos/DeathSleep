#ifndef DEATHSLEEP_H
#define DEATHSLEEP_H

#define InitializeCallbackInfo(ci, functionAddres, parameterAddres) \
    {                                                               \
        (ci)->timer = NULL;                                         \
        (ci)->isImpersonating = 0;                                  \
        (ci)->flags = 0;                                            \
        (ci)->callbackAddr = (WAITORTIMERCALLBACK)functionAddres;   \
        (ci)->paramAddr = parameterAddres;                          \
        (ci)->timerQueue = NULL;                                    \
        (ci)->isPeriodic = 0;                                       \
        (ci)->execControl = 0;                                      \
    }

#define InitializeFiletimeMs(ft, millis)                                                  \
    {                                                                                     \
        (ft)->dwHighDateTime = (DWORD)(((ULONGLONG) - ((millis)*10 * 1000)) >> 32);       \
        (ft)->dwLowDateTime  = (DWORD)(((ULONGLONG) - ((millis)*10 * 1000)) & 0xffffffff); \
    }

typedef struct
{                                     //      NOTE                REQUIRED
    PTP_TIMER timer;                  // 0     Timer                   X
    DWORD64 m2;                       // 8     NULL
    DWORD64 isImpersonating;          // 16    0                       X
    ULONG flags;                      // 24    Flags                   X
    DWORD32 m5;                       // 28    NULL
    WAITORTIMERCALLBACK callbackAddr; // 32    Callback Address        X
    PVOID paramAddr;                  // 40    Parameter Address       X
    DWORD32 m7;                       // 48    0
    DWORD32 m8;                       // 52    Padding
    HANDLE timerQueue;                // 56    NULL                    X
    DWORD64 m9;                       // 64    0
    DWORD64 m10;                      // 72    0
    DWORD64 m11;                      // 80    0
    DWORD32 isPeriodic;               // 88    0                       X
    DWORD32 execControl;              // 92    0                       X
} CallbackInfo;

VOID   awake (PVOID lpParam);
VOID   rebirth (PTP_CALLBACK_INSTANCE Instance, PVOID lpParam, PTP_TIMER Timer);
PVOID  InitilizeRopStack (PVOID ropStackMemBlock, DWORD sizeRopStack, PVOID function, PVOID arg, PVOID rcxGadgetAddr, PVOID shadowFixerGadgetAddr);
VOID   coma (ULONGLONG time);

DWORD  WINAPI    mainProgram();

extern DWORD_PTR getRsp();
extern void      moveRsp(DWORD, DWORD);

#endif
