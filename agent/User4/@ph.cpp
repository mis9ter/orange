#include "CControl.h"
#include <winnt.h>
#include <exception>


PPH_OBJECT_TYPE     PhObjectTypeObject = NULL;
PH_FREE_LIST        PhObjectSmallFreeList;
PPH_OBJECT_TYPE     PhObjectTypeTable[PH_OBJECT_TYPE_TABLE_SIZE];
SLIST_HEADER        PhObjectDeferDeleteListHead;


PVOID           PhAllocateZero(
    _In_ SIZE_T Size
)
{
    PVOID buffer    = PhAllocate(Size);
    if( buffer )
        memset(buffer, 0, Size);
    return buffer;
}
PVOID           PhAllocate(
    _In_ SIZE_T Size
)
{
    PVOID buffer;
    try {
        buffer  = new char[Size];        
    }
    catch( std::exception & e ) {
        UNREFERENCED_PARAMETER(e);
        buffer  = NULL;
    }
    return buffer;
}
void            PhFree(_Frees_ptr_opt_ PVOID Memory)
{
    if( Memory ) {
        delete Memory;
    }
}

PPH_OBJECT_TYPE PhCreateObjectTypeEx(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ PPH_TYPE_DELETE_PROCEDURE DeleteProcedure,
    _In_opt_ PPH_OBJECT_TYPE_PARAMETERS Parameters
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PPH_OBJECT_TYPE objectType;

    // Check the flags.
    if ((Flags & PH_OBJECT_TYPE_VALID_FLAGS) != Flags) /* Valid flag mask */
        PhRaiseStatus(STATUS_INVALID_PARAMETER_3);
    if ((Flags & PH_OBJECT_TYPE_USE_FREE_LIST) && !Parameters)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_MIX);

    // Create the type object.
    objectType = PhCreateObject(sizeof(PH_OBJECT_TYPE), PhObjectTypeObject);

    // Initialize the type object.
    objectType->Flags = (USHORT)Flags;
    objectType->TypeIndex = (USHORT)_InterlockedIncrement(&PhObjectTypeCount) - 1;
    objectType->NumberOfObjects = 0;
    objectType->DeleteProcedure = DeleteProcedure;
    objectType->Name = Name;

    if (objectType->TypeIndex < PH_OBJECT_TYPE_TABLE_SIZE)
        PhObjectTypeTable[objectType->TypeIndex] = objectType;
    else
        PhRaiseStatus(STATUS_UNSUCCESSFUL);

    if (Parameters)
    {
        if (Flags & PH_OBJECT_TYPE_USE_FREE_LIST)
        {
            PhInitializeFreeList(
                &objectType->FreeList,
                PhAddObjectHeaderSize(Parameters->FreeListSize),
                Parameters->FreeListCount
            );
        }
    }

    return objectType;
}
PPH_OBJECT_TYPE PhCreateObjectType(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ PPH_TYPE_DELETE_PROCEDURE DeleteProcedure
)
{
    return PhCreateObjectTypeEx(
        Name,
        Flags,
        DeleteProcedure,
        NULL
    );
}

VOID PhInitializeFreeList(
    _Out_ PPH_FREE_LIST FreeList,
    _In_ SIZE_T Size,
    _In_ ULONG MaximumCount
)
{
    RtlInitializeSListHead(&FreeList->ListHead);
    FreeList->Count = 0;
    FreeList->MaximumCount = MaximumCount;
    FreeList->Size = Size;
}

BOOLEAN PhRefInitialization(
    VOID
)
{
    PH_OBJECT_TYPE dummyObjectType;

#ifdef DEBUG
    InitializeListHead(&PhDbgObjectListHead);
#endif

    RtlInitializeSListHead(&PhObjectDeferDeleteListHead);
    PhInitializeFreeList(
        &PhObjectSmallFreeList,
        PhAddObjectHeaderSize(PH_OBJECT_SMALL_OBJECT_SIZE),
        PH_OBJECT_SMALL_OBJECT_COUNT
    );

    // Create the fundamental object type.

    memset(&dummyObjectType, 0, sizeof(PH_OBJECT_TYPE));
    PhObjectTypeObject = &dummyObjectType; // PhCreateObject expects an object type.
    PhObjectTypeTable[0] = &dummyObjectType; // PhCreateObject also expects PhObjectTypeTable[0] to be filled in.
    PhObjectTypeObject = PhCreateObjectType(L"Type", 0, NULL);

    // Now that the fundamental object type exists, fix it up.
    PhObjectToObjectHeader(PhObjectTypeObject)->TypeIndex = PhObjectTypeObject->TypeIndex;
    PhObjectTypeObject->NumberOfObjects = 1;

    // Create the allocated memory object type.
    PhAllocType = PhCreateObjectType(L"Alloc", 0, NULL);

    // Reserve a slot for the auto pool.
    PhpAutoPoolTlsIndex = TlsAlloc();

    if (PhpAutoPoolTlsIndex == TLS_OUT_OF_INDEXES)
        return FALSE;

    return TRUE;
}

PPH_LIST PhCreateList(
    _In_ ULONG InitialCapacity
)
{
    PPH_LIST list;

    list = PhCreateObject(sizeof(PH_LIST), PhListType);

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    list->Count = 0;
    list->AllocatedCount = InitialCapacity;
    list->Items = PhAllocate(list->AllocatedCount * sizeof(PVOID));

    return list;
}
_May_raise_ PVOID PhCreateObject(
    _In_ SIZE_T ObjectSize,
    _In_ PPH_OBJECT_TYPE ObjectType
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PPH_OBJECT_HEADER objectHeader;

    // Allocate storage for the object. Note that this includes the object header followed by the
    // object body.
    objectHeader = PhpAllocateObject(ObjectType, ObjectSize);

    // Object type statistics.
    _InterlockedIncrement((PLONG)&ObjectType->NumberOfObjects);

    // Initialize the object header.
    objectHeader->RefCount = 1;
    objectHeader->TypeIndex = ObjectType->TypeIndex;
    // objectHeader->Flags is set by PhpAllocateObject.

    REF_STAT_UP(RefObjectsCreated);

#ifdef DEBUG
    {
        USHORT capturedFrames;

        capturedFrames = RtlCaptureStackBackTrace(1, 16, objectHeader->StackBackTrace, NULL);
        memset(
            &objectHeader->StackBackTrace[capturedFrames],
            0,
            sizeof(objectHeader->StackBackTrace) - capturedFrames * sizeof(PVOID)
        );
    }

    PhAcquireQueuedLockExclusive(&PhDbgObjectListLock);
    InsertTailList(&PhDbgObjectListHead, &objectHeader->ObjectListEntry);
    PhReleaseQueuedLockExclusive(&PhDbgObjectListLock);

    {
        PPH_CREATE_OBJECT_HOOK dbgCreateObjectHook;

        dbgCreateObjectHook = PhDbgCreateObjectHook;

        if (dbgCreateObjectHook)
        {
            dbgCreateObjectHook(
                PhObjectHeaderToObject(objectHeader),
                ObjectSize,
                0,
                ObjectType
            );
        }
    }
#endif

    return PhObjectHeaderToObject(objectHeader);
}