#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "RVCOS.h"
#include "RVCGraphics.h"
#include "tests.h"

volatile char *MemoryBase1;
volatile char *MemoryBase2;


int do_test_memory_functionality() {
    int Index, Inner, Groups;
    int *Pointers[4];
    int *MemoryBase;
    TMemoryPoolID MemoryPoolID;
    TMemorySize AvailableSpace;

    WriteString("Allocating pool: ");
    if(RVCOS_STATUS_SUCCESS != RVCMemoryAllocate(256, (void **)&MemoryBase)){
        WriteString("Failed to allocate memory pool\n");
        return -1;
    }
    WriteString("Done\nCreating pool: ");
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolCreate(MemoryBase, 256, &MemoryPoolID)){
        RVCMemoryPoolDeallocate(RVCOS_MEMORY_POOL_ID_SYSTEM, MemoryBase);
        WriteString("Failed to create memory pool\n");
        return -1;
    }
    WriteString("Done\nQuerying space: ");
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolQuery(MemoryPoolID,&AvailableSpace)){
        WriteString("Failed to query memory pool\n");
        RVCMemoryPoolDelete(MemoryPoolID);
        RVCMemoryDeallocate(MemoryBase);
        return -1;
    }
    Groups = 256 == AvailableSpace ? 4 : 2;
    WriteString("Done\nAllocating spaces: ");
    for(Index = 0; Index < 4; Index++){
        if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolAllocate(MemoryPoolID, 64, (void **)&Pointers[Index])){
            char TempStr[] = "Failed to allocate space 0\n";
            TempStr[25] = '0' + Index;
            WriteString(TempStr);
            return -1;
        }
    }
    WriteString("Done\nAssigning values: ");
    for(Index = 0; Index < 4; Index++){
        for(Inner = 0; Inner < 64 / sizeof(int); Inner++){
            Pointers[Index][Inner] = Index * (64 / sizeof(int)) + Inner;
        }
    }
    WriteString("Done\nDeallocating spaces: ");
    for(Index = 0; Index < 4; Index++){
        if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolDeallocate(MemoryPoolID, Pointers[Index])){
            char TempStr[] = "Failed to deallocate space 0\n";
            TempStr[27] = '0' + Index;
            WriteString(TempStr);
            return -1;
        }
    }
    WriteString("Done\nAllocating full space: ");
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolAllocate(MemoryPoolID, AvailableSpace, (void **)&Pointers[0])){
        WriteString("Failed to allocate full space\n");
        return -1;
    }
    WriteString("Done\nPrinting values:\n");
    for(Index = 0; Index < (AvailableSpace / sizeof(int)); Index++){
        WriteString("%4d", Pointers[0][Index]);
    }
    WriteString("\nDeallocating space: ");
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolDeallocate(MemoryPoolID, Pointers[0])){
        WriteString("Failed to deallocate full space\n");
        return -1;
    }
    WriteString("Done\nDeleting memory pool: ");
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolDelete(MemoryPoolID)){
        WriteString("Failed to delete memory pool\n");
        return -1;
    }
    WriteString("Done\nDeallocating memory pool space: ");
    if(RVCOS_STATUS_SUCCESS != RVCMemoryDeallocate(MemoryBase)){
        WriteString("Failed to deallocate memory pool space\n");
        return -1;
    }
    WriteString("Done\n");

    return 0;
}

int test_memory_functionality() {

    int rc = do_test_memory_functionality();

    WriteString("\n\ntest_memory_functionality finished\n");

    if (rc) {
        WriteString(" - with errors.\n");
    } else {
        WriteString(" - successfully.\n");
    }
    return check_proceed("Test passed?");
}


int test_memory_error() {
    TMemorySize SystemPoolSize;
    TMemoryPoolID MemoryPool1;
    char *LocalAllocation;

    int error_count = 0;
    int rc;

    WriteString("Main testing RVCMemoryPoolQuery.\n");
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMemoryPoolQuery(RVCOS_MEMORY_POOL_ID_INVALID, &SystemPoolSize)){
        WriteString("MemoryPoolQuery doesn't handle bad memoryid.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMemoryPoolQuery(RVCOS_MEMORY_POOL_ID_SYSTEM, NULL)){
        WriteString("MemoryPoolQuery doesn't handle NULL bytesleft.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolQuery(RVCOS_MEMORY_POOL_ID_SYSTEM, &SystemPoolSize)){
        WriteString("MemoryPoolQuery doesn't return success with valid inputs.\n");
        ++error_count;
    }
    WriteString("Main RVCMemoryPoolQuery appears OK.\n");
    WriteString("Main testing RVCMemoryPoolAllocate.\n");
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMemoryPoolAllocate(RVCOS_MEMORY_POOL_ID_SYSTEM, 0, (void **)&MemoryBase1)){
        WriteString("MemoryPoolAllocate doesn't handle zero size.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMemoryPoolAllocate(RVCOS_MEMORY_POOL_ID_SYSTEM, 64, NULL)){
        WriteString("MemoryPoolAllocate doesn't handle NULL pointer.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMemoryPoolAllocate(RVCOS_MEMORY_POOL_ID_INVALID, 64, (void **)&MemoryBase1)){
        WriteString("MemoryPoolAllocate doesn't handle bad memoryid.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INSUFFICIENT_RESOURCES  != RVCMemoryPoolAllocate(RVCOS_MEMORY_POOL_ID_SYSTEM, SystemPoolSize + 256, (void **)&MemoryBase1)){
        WriteString("MemoryPoolAllocate doesn't handle insufficient resources.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolAllocate(RVCOS_MEMORY_POOL_ID_SYSTEM, 64, (void **)&MemoryBase1)){
        WriteString("MemoryPoolAllocate doesn't return success with valid inputs.\n");
        ++error_count;
    }
    WriteString("Main RVCMemoryPoolAllocate appears OK.\n");

    WriteString("Main testing RVCMemoryPoolCreate.\n");
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMemoryPoolCreate(NULL, 128, &MemoryPool1)){
        WriteString("MemoryPoolCreate doesn't handle NULL base.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMemoryPoolCreate((void *)MemoryBase1, 0, &MemoryPool1)){
        WriteString("MemoryPoolCreate doesn't handle zero size.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMemoryPoolCreate((void *)MemoryBase1, 128, NULL)){
        WriteString("MemoryPoolCreate doesn't handle NULL memoryid.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolCreate((void *)MemoryBase1, 128, &MemoryPool1)){
        WriteString("MemoryPoolCreate doesn't return success with valid inputs.\n");
        ++error_count;
    }
    WriteString("Main RVCMemoryPoolCreate appears OK.\n");
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolAllocate(MemoryPool1, 96, (void **)&LocalAllocation)){
        WriteString("MemoryPoolAllocate doesn't return success with valid inputs.\n");
        ++error_count;
    }
    if(LocalAllocation != MemoryBase1){
        WriteString("MemoryPoolAllocate allocated from the wrong pool.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolQuery(MemoryPool1, &SystemPoolSize)){
        WriteString("MemoryPoolQuery doesn't return success with valid inputs.\n");
        ++error_count;
    }
    if(0 != SystemPoolSize){
        WriteString("MemoryPoolQuery doesn't keep track of actual available memory.\n");
        ++error_count;
    }
    WriteString("Main testing RVCMemoryPoolDeallocate.\n");
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMemoryPoolDeallocate(MemoryPool1, NULL)){
        WriteString("MemoryPoolDeallocate doesn't handle NULL base.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_PARAMETER != RVCMemoryPoolDeallocate(MemoryPool1, LocalAllocation + 1)){
        WriteString("MemoryPoolDeallocate doesn't handle zero size.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMemoryPoolDeallocate(RVCOS_MEMORY_POOL_ID_INVALID, LocalAllocation)){
        WriteString("MemoryPoolDeallocate doesn't handle NULL memoryid.\n");
        ++error_count;
    }
    WriteString("Main RVCMemoryPoolDeallocate appears OK.\n");
    WriteString("Main testing RVCMemoryPoolDelete.\n");
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMemoryPoolDelete(RVCOS_MEMORY_POOL_ID_INVALID)){
        WriteString("MemoryPoolDelete doesn't handle bad memoryid.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_STATE != RVCMemoryPoolDelete(MemoryPool1)){
        WriteString("MemoryPoolDelete doesn't handle allocated memory pools.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolDeallocate(MemoryPool1, LocalAllocation)){
        WriteString("MemoryPoolDeallocate doesn't handle valid inputs.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_SUCCESS != RVCMemoryPoolDelete(MemoryPool1)){
        WriteString("MemoryPoolDelete doesn't handle valid inputs.\n");
        ++error_count;
    }
    WriteString("Main RVCMemoryPoolDelete appears OK.\n");
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMemoryPoolAllocate(MemoryPool1, 32, (void **)&LocalAllocation)){
        WriteString("MemoryPoolAllocate doesn't handle bad memoryid.\n");
        ++error_count;
    }
    if(RVCOS_STATUS_ERROR_INVALID_ID != RVCMemoryPoolQuery(MemoryPool1, &SystemPoolSize)){
        WriteString("MemoryPoolQuery doesn't handle bad memoryid.\n");
        ++error_count;
    }

    WriteString("\n\ntest_memory_error finished with %d errors.\n", error_count);

    return check_proceed("Test passed?");
}
