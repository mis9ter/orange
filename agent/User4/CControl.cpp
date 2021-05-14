#include "framework.h"

// begin_phapppub
typedef LONG (NTAPI *PPH_CM_POST_SORT_FUNCTION)(
	_In_ LONG Result,
	_In_ PVOID Node1,
	_In_ PVOID Node2,
	_In_ PH_SORT_ORDER SortOrder
	);
// end_phapppub

typedef struct _PH_CM_MANAGER
{
	HWND Handle;
	ULONG MinId;
	ULONG NextId;
	PPH_CM_POST_SORT_FUNCTION PostSortFunction;
	LIST_ENTRY ColumnListHead;
	PPH_LIST NotifyList;
} PH_CM_MANAGER, *PPH_CM_MANAGER;


typedef struct _PH_PROCESS_NODE *PPH_PROCESS_NODE;


bool	CTab::ProcessPageCallback(
	_In_ struct _MAIN_TAB_PAGE *Page,
	_In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
	_In_opt_ PVOID Parameter1,
	_In_opt_ PVOID Parameter2
) {
	CTab	*pClass	= (CTab *)Page->pContext;

	switch (Message)
	{
		case MainTabPageCreate:
		{
			pClass->Log("%-32s MainTabPageCreate", __func__);
		}
		break;
	}
	return false;
}
static PH_SORT_ORDER ProcessTreeListSortOrder;
#define SORT_FUNCTION(Column) PhpProcessTreeNewCompare##Column

bool	CTab::TreeNewCallback(
	_In_ HWND hwnd,
	_In_ PH_TREENEW_MESSAGE Message,
	_In_opt_ PVOID Parameter1,
	_In_opt_ PVOID Parameter2,
	_In_opt_ PVOID Context
) {
	//PPH_PROCESS_NODE node;

	//if (PhCmForwardMessage(hwnd, Message, Parameter1, Parameter2, &ProcessTreeListCm))
	//	return TRUE;
    switch (Message)
    {
        case TreeNewGetChildren:
        /*
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            node = (PPH_PROCESS_NODE)getChildren->Node;

            if (ProcessTreeListSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)ProcessNodeRootList->Items;
                    getChildren->NumberOfChildren = ProcessNodeRootList->Count;
                }
                else
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
                    getChildren->NumberOfChildren = node->Children->Count;
                }
            }
            else
            {
                if (!node)
                {
                    static PVOID sortFunctions[] =
                    {
                        SORT_FUNCTION(Name),
                        SORT_FUNCTION(Pid),
                        SORT_FUNCTION(Cpu),
                        SORT_FUNCTION(IoTotalRate),
                        SORT_FUNCTION(PrivateBytes),
                        SORT_FUNCTION(UserName),
                        SORT_FUNCTION(Description),
                        SORT_FUNCTION(CompanyName),
                        SORT_FUNCTION(Version),
                        SORT_FUNCTION(FileName),
                        SORT_FUNCTION(CommandLine),
                        SORT_FUNCTION(PeakPrivateBytes),
                        SORT_FUNCTION(WorkingSet),
                        SORT_FUNCTION(PeakWorkingSet),
                        SORT_FUNCTION(PrivateWs),
                        SORT_FUNCTION(SharedWs),
                        SORT_FUNCTION(ShareableWs),
                        SORT_FUNCTION(VirtualSize),
                        SORT_FUNCTION(PeakVirtualSize),
                        SORT_FUNCTION(PageFaults),
                        SORT_FUNCTION(SessionId),
                        SORT_FUNCTION(BasePriority), // Priority Class
                        SORT_FUNCTION(BasePriority),
                        SORT_FUNCTION(Threads),
                        SORT_FUNCTION(Handles),
                        SORT_FUNCTION(GdiHandles),
                        SORT_FUNCTION(UserHandles),
                        SORT_FUNCTION(IoRoRate),
                        SORT_FUNCTION(IoWRate),
                        SORT_FUNCTION(Integrity),
                        SORT_FUNCTION(IoPriority),
                        SORT_FUNCTION(PagePriority),
                        SORT_FUNCTION(StartTime),
                        SORT_FUNCTION(TotalCpuTime),
                        SORT_FUNCTION(KernelCpuTime),
                        SORT_FUNCTION(UserCpuTime),
                        SORT_FUNCTION(VerificationStatus),
                        SORT_FUNCTION(VerifiedSigner),
                        SORT_FUNCTION(Aslr),
                        SORT_FUNCTION(RelativeStartTime),
                        SORT_FUNCTION(Bits),
                        SORT_FUNCTION(Elevation),
                        SORT_FUNCTION(WindowTitle),
                        SORT_FUNCTION(WindowStatus),
                        SORT_FUNCTION(Cycles),
                        SORT_FUNCTION(CyclesDelta),
                        SORT_FUNCTION(Cpu), // CPU History
                        SORT_FUNCTION(PrivateBytes), // Private Bytes History
                        SORT_FUNCTION(IoTotalRate), // I/O History
                        SORT_FUNCTION(Dep),
                        SORT_FUNCTION(Virtualized),
                        SORT_FUNCTION(ContextSwitches),
                        SORT_FUNCTION(ContextSwitchesDelta),
                        SORT_FUNCTION(PageFaultsDelta),
                        SORT_FUNCTION(IoReads),
                        SORT_FUNCTION(IoWrites),
                        SORT_FUNCTION(IoOther),
                        SORT_FUNCTION(IoReadBytes),
                        SORT_FUNCTION(IoWriteBytes),
                        SORT_FUNCTION(IoOtherBytes),
                        SORT_FUNCTION(IoReadsDelta),
                        SORT_FUNCTION(IoWritesDelta),
                        SORT_FUNCTION(IoOtherDelta),
                        SORT_FUNCTION(OsContext),
                        SORT_FUNCTION(PagedPool),
                        SORT_FUNCTION(PeakPagedPool),
                        SORT_FUNCTION(NonPagedPool),
                        SORT_FUNCTION(PeakNonPagedPool),
                        SORT_FUNCTION(MinimumWorkingSet),
                        SORT_FUNCTION(MaximumWorkingSet),
                        SORT_FUNCTION(PrivateBytesDelta),
                        SORT_FUNCTION(Subsystem),
                        SORT_FUNCTION(PackageName),
                        SORT_FUNCTION(AppId),
                        SORT_FUNCTION(DpiAwareness),
                        SORT_FUNCTION(CfGuard),
                        SORT_FUNCTION(TimeStamp),
                        SORT_FUNCTION(FileModifiedTime),
                        SORT_FUNCTION(FileSize),
                        SORT_FUNCTION(Subprocesses),
                        SORT_FUNCTION(JobObjectId),
                        SORT_FUNCTION(Protection),
                        SORT_FUNCTION(DesktopInfo),
                        SORT_FUNCTION(Critical),
                        SORT_FUNCTION(HexPid),
                        SORT_FUNCTION(CpuCore),
                        SORT_FUNCTION(Cet),
                        SORT_FUNCTION(ImageCoherency),
                        SORT_FUNCTION(ErrorMode),
                        SORT_FUNCTION(CodePage),
                        SORT_FUNCTION(StartTime), // Timeline
                    };
                    int (__cdecl *sortFunction)(const void *, const void *);

                    if (!PhCmForwardSort(
                        (PPH_TREENEW_NODE *)ProcessNodeList->Items,
                        ProcessNodeList->Count,
                        ProcessTreeListSortColumn,
                        ProcessTreeListSortOrder,
                        &ProcessTreeListCm
                    ))
                    {
                        if (ProcessTreeListSortColumn < PHPRTLC_MAXIMUM)
                            sortFunction = sortFunctions[ProcessTreeListSortColumn];
                        else
                            sortFunction = NULL;

                        if (sortFunction)
                        {
                            qsort(ProcessNodeList->Items, ProcessNodeList->Count, sizeof(PVOID), sortFunction);
                        }
                    }

                    getChildren->Children = (PPH_TREENEW_NODE *)ProcessNodeList->Items;
                    getChildren->NumberOfChildren = ProcessNodeList->Count;
                }
            }
        }
        */
        return TRUE;
    }
	return FALSE;
}