#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "RVCOS.h"
#include "RVCGraphics.h"
#include "tests.h"

TMutexID TheMutex;

int test_mutex_error() {
    TThreadID ThisThreadID;
    TThreadID MutexOwner;
    TMutexID BadMutexID;

    int error_count = 0;

    WriteString("Main testing RVCMutexCreate.\n");
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMutexCreate(NULL)){
        WriteString("MutexCreate doesn't handle NULL mutexref.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMutexCreate(&TheMutex)){
        WriteString("MutexCreate doesn't handle valid inputs.\n");
        ++error_count;
    }
    BadMutexID = TheMutex + 16;
    WriteString("Main RVCMutexCreate appears OK.\n");
    WriteString("Main testing RVCMutexQuery.\n");
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMutexQuery(TheMutex, NULL)){
        WriteString("MutexQuery doesn't handle NULL ownerref.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMutexQuery(BadMutexID, &MutexOwner)){
        WriteString("MutexQuery doesn't handle bad mutex.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMutexQuery(TheMutex, &MutexOwner)){
        WriteString("MutexQuery doesn't handle valid inputs.\n");
        ++error_count;
    }
    if(RVCOS_THREAD_ID_INVALID != MutexOwner){
        WriteString("MutexQuery doesn't correct value for owner.\n");
        ++error_count;
    }
    WriteString("Main RVCMutexQuery appears OK.\n");


    WriteString("Main testing RVCMutexAcquire.\n");
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMutexAcquire(BadMutexID, RVCOS_TIMEOUT_INFINITE)){
        WriteString("MutexAcquire doesn't handle bad mutex.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMutexAcquire(TheMutex, RVCOS_TIMEOUT_IMMEDIATE)){
        WriteString("MutexAcquire doesn't handle RVCOS_TIMEOUT_IMMEDIATE.\n");
        ++error_count;
    }
    WriteString("Main RVCMutexAcquire appears OK.\n");

    WriteString("Main testing RVCMutexDelete.\n");
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMutexDelete(BadMutexID)){
        WriteString("MutexDelete doesn't handle bad thead.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_STATE != RVCMutexDelete(TheMutex)){
        WriteString("MutexDelete doesn't handle held mutexes.\n");
        ++error_count;
    }
    WriteString("Main RVCMutexDelete appears OK.\n");
    WriteString("Main testing RVCMutexRelease.\n");
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMutexRelease(BadMutexID)){
        WriteString("MutexRelease doesn't handle bad thead.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMutexRelease(TheMutex)){
        WriteString("MutexRelease doesn't handle held mutexes.\n");
        ++error_count;
    }
    WriteString("Main RVCMutexRelease appears OK.\n");


    if(RVCOS_STATUS_SUCCESS != RVCThreadID(&ThisThreadID)){
        WriteString("ThreadID doesn't handle valid threadref.\n");
        ++error_count;
    }

    WriteString("Main testing RVCMutexAcquire\n");
    if(RVCOS_STATUS_SUCCESS != RVCMutexAcquire(TheMutex, RVCOS_TIMEOUT_IMMEDIATE)){
        WriteString("MutexAcquire doesn't handle terminated owners.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMutexQuery(TheMutex, &MutexOwner)){
        WriteString("MutexQuery doesn't handle valid inputs.\n");
        ++error_count;
    }
    if(MutexOwner != ThisThreadID){
        WriteString("MutexQuery returned wrong owner of mutex.\n");
        ++error_count;
    }
    WriteString("Main RVCMutexAcquire appears OK.\n");


    WriteString("Main testing RVCMutexRelease.\n");
    if(RVCOS_STATUS_SUCCESS != RVCMutexRelease(TheMutex)){
        WriteString("MutexRelease doesn't handle held mutexes.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMutexQuery(TheMutex, &MutexOwner)){
        WriteString("MutexQuery doesn't handle valid inputs.\n");
        ++error_count;
    }
    if(RVCOS_THREAD_ID_INVALID != MutexOwner){
        WriteString("MutexQuery doesn't correct value for owner.\n");
        ++error_count;
    }
    WriteString("Main RVCMutexRelease appears OK.\n");
    WriteString("Main testing RVCMutexDelete.\n");
    if(RVCOS_STATUS_SUCCESS != RVCMutexDelete(TheMutex)){
        WriteString("MutexDelete doesn't handle held mutexes.\n");
        ++error_count;
    }
    WriteString("Main RVCMutexDelete appears OK.\n");

    WriteString("\n\ntest_mutex_error finished with %d errors.\n", error_count);

    return check_proceed("Test passed?");
}


TMutexID MutexHigh, MutexMedium, MutexLow, MutexMain;

TThreadReturn MTX_ThreadHigh(void *param){
    WriteString("  5  ThreadHigh Alive\n\n");
    RVCMutexAcquire(MutexHigh, RVCOS_TIMEOUT_INFINITE);
    WriteString("  6  ThreadHigh Awake\n\n");
    return 0;
}

TThreadReturn MTX_ThreadMedium(void *param){
    WriteString("  5  ThreadMedium Alive\n\n");
    RVCMutexAcquire(MutexMedium, RVCOS_TIMEOUT_INFINITE);
    WriteString("  6  ThreadMedium Awake\n\n");
    return 0;
}

TThreadReturn MTX_ThreadLow(void *param){
    RVCMutexAcquire(MutexMain, RVCOS_TIMEOUT_INFINITE);
    WriteString("  5  ThreadLow Alive\n\n");
    RVCMutexAcquire(MutexLow, RVCOS_TIMEOUT_INFINITE);
    WriteString("  6  ThreadLow Awake\n\n");
    RVCMutexRelease(MutexMain);
    return 0;
}

int test_mutex_functionality() {

    TThreadID ThreadIDHigh, ThreadIDMedium, ThreadIDLow;

    WriteString("  1  Main creating threads.\n\n");
    RVCThreadCreate(MTX_ThreadLow, NULL, 0x1000, RVCOS_THREAD_PRIORITY_LOW, &ThreadIDLow);
    RVCThreadCreate(MTX_ThreadMedium, NULL, 0x1000, RVCOS_THREAD_PRIORITY_NORMAL, &ThreadIDMedium);
    RVCThreadCreate(MTX_ThreadHigh, NULL, 0x1000, RVCOS_THREAD_PRIORITY_HIGH, &ThreadIDHigh);
    WriteString("  2  Main creating mutexes.\n\n");
    RVCMutexCreate(&MutexMain);
    RVCMutexCreate(&MutexLow);
    RVCMutexCreate(&MutexMedium);
    RVCMutexCreate(&MutexHigh);
    WriteString("  3  Main locking mutexes.\n\n");
    RVCMutexAcquire(MutexLow, RVCOS_TIMEOUT_INFINITE);
    RVCMutexAcquire(MutexMedium, RVCOS_TIMEOUT_INFINITE);
    RVCMutexAcquire(MutexHigh, RVCOS_TIMEOUT_INFINITE);
    WriteString("  4  Main activating processes.\n\n");
    RVCThreadActivate(ThreadIDLow);
    RVCThreadActivate(ThreadIDMedium);
    RVCThreadActivate(ThreadIDHigh);
    WriteString("  5  Main releasing mutexes.\n\n");
    RVCMutexRelease(MutexLow);
    RVCMutexRelease(MutexMedium);
    RVCMutexRelease(MutexHigh);
    WriteString("  6  Main acquiring main mutex.\n\n");
    RVCMutexAcquire(MutexMain, RVCOS_TIMEOUT_INFINITE);
    WriteString("  7  Main acquired main mutex.\nGoodbye\n");

    return check_proceed("Test passed?");
}
