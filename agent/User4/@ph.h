#pragma once

#define _May_raise_
struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

#define PH_OBJECT_TYPE_TABLE_SIZE 256

/** The object was allocated from the small free list. */
#define PH_OBJECT_FROM_SMALL_FREE_LIST 0x1
/** The object was allocated from the type free list. */
#define PH_OBJECT_FROM_TYPE_FREE_LIST 0x2

#define PH_OBJECT_SMALL_OBJECT_SIZE 48
#define PH_OBJECT_SMALL_OBJECT_COUNT 512

// Object type flags
#define PH_OBJECT_TYPE_USE_FREE_LIST 0x00000001
#define PH_OBJECT_TYPE_VALID_FLAGS 0x00000001

#define assert(expression) ((void)0)

// Default columns should go first
#define PHPRTLC_NAME 0
#define PHPRTLC_PID 1
#define PHPRTLC_CPU 2
#define PHPRTLC_IOTOTALRATE 3
#define PHPRTLC_PRIVATEBYTES 4
#define PHPRTLC_USERNAME 5
#define PHPRTLC_DESCRIPTION 6

#define PHPRTLC_COMPANYNAME 7
#define PHPRTLC_VERSION 8
#define PHPRTLC_FILENAME 9
#define PHPRTLC_COMMANDLINE 10
#define PHPRTLC_PEAKPRIVATEBYTES 11
#define PHPRTLC_WORKINGSET 12
#define PHPRTLC_PEAKWORKINGSET 13
#define PHPRTLC_PRIVATEWS 14
#define PHPRTLC_SHAREDWS 15
#define PHPRTLC_SHAREABLEWS 16
#define PHPRTLC_VIRTUALSIZE 17
#define PHPRTLC_PEAKVIRTUALSIZE 18
#define PHPRTLC_PAGEFAULTS 19
#define PHPRTLC_SESSIONID 20
#define PHPRTLC_PRIORITYCLASS 21
#define PHPRTLC_BASEPRIORITY 22

#define PHPRTLC_THREADS 23
#define PHPRTLC_HANDLES 24
#define PHPRTLC_GDIHANDLES 25
#define PHPRTLC_USERHANDLES 26
#define PHPRTLC_IORORATE 27
#define PHPRTLC_IOWRATE 28
#define PHPRTLC_INTEGRITY 29
#define PHPRTLC_IOPRIORITY 30
#define PHPRTLC_PAGEPRIORITY 31
#define PHPRTLC_STARTTIME 32
#define PHPRTLC_TOTALCPUTIME 33
#define PHPRTLC_KERNELCPUTIME 34
#define PHPRTLC_USERCPUTIME 35
#define PHPRTLC_VERIFICATIONSTATUS 36
#define PHPRTLC_VERIFIEDSIGNER 37
#define PHPRTLC_ASLR 38
#define PHPRTLC_RELATIVESTARTTIME 39
#define PHPRTLC_BITS 40
#define PHPRTLC_ELEVATION 41
#define PHPRTLC_WINDOWTITLE 42
#define PHPRTLC_WINDOWSTATUS 43
#define PHPRTLC_CYCLES 44
#define PHPRTLC_CYCLESDELTA 45
#define PHPRTLC_CPUHISTORY 46
#define PHPRTLC_PRIVATEBYTESHISTORY 47
#define PHPRTLC_IOHISTORY 48
#define PHPRTLC_DEP 49
#define PHPRTLC_VIRTUALIZED 50
#define PHPRTLC_CONTEXTSWITCHES 51
#define PHPRTLC_CONTEXTSWITCHESDELTA 52
#define PHPRTLC_PAGEFAULTSDELTA 53

#define PHPRTLC_IOREADS 54
#define PHPRTLC_IOWRITES 55
#define PHPRTLC_IOOTHER 56
#define PHPRTLC_IOREADBYTES 57
#define PHPRTLC_IOWRITEBYTES 58
#define PHPRTLC_IOOTHERBYTES 59
#define PHPRTLC_IOREADSDELTA 60
#define PHPRTLC_IOWRITESDELTA 61
#define PHPRTLC_IOOTHERDELTA 62

#define PHPRTLC_OSCONTEXT 63
#define PHPRTLC_PAGEDPOOL 64
#define PHPRTLC_PEAKPAGEDPOOL 65
#define PHPRTLC_NONPAGEDPOOL 66
#define PHPRTLC_PEAKNONPAGEDPOOL 67
#define PHPRTLC_MINIMUMWORKINGSET 68
#define PHPRTLC_MAXIMUMWORKINGSET 69
#define PHPRTLC_PRIVATEBYTESDELTA 70
#define PHPRTLC_SUBSYSTEM 71
#define PHPRTLC_PACKAGENAME 72
#define PHPRTLC_APPID 73
#define PHPRTLC_DPIAWARENESS 74
#define PHPRTLC_CFGUARD 75
#define PHPRTLC_TIMESTAMP 76
#define PHPRTLC_FILEMODIFIEDTIME 77
#define PHPRTLC_FILESIZE 78
#define PHPRTLC_SUBPROCESSCOUNT 79
#define PHPRTLC_JOBOBJECTID 80
#define PHPRTLC_PROTECTION 81
#define PHPRTLC_DESKTOP 82
#define PHPRTLC_CRITICAL 83
#define PHPRTLC_PIDHEX 84
#define PHPRTLC_CPUCORECYCLES 85
#define PHPRTLC_CET 86
#define PHPRTLC_IMAGE_COHERENCY 87
#define PHPRTLC_ERRORMODE 88
#define PHPRTLC_CODEPAGE 89
#define PHPRTLC_TIMELINE 90
#define PHPRTLC_MAXIMUM 91

#define PH_ALIGN_CENTER 0x0
#define PH_ALIGN_LEFT 0x1
#define PH_ALIGN_RIGHT 0x2
#define PH_ALIGN_TOP 0x4
#define PH_ALIGN_BOTTOM 0x8


#define TNM_FIRST (WM_USER + 1)
#define TNM_SETCALLBACK (WM_USER + 1)
#define TNM_NODESADDED (WM_USER + 2) // unimplemented
#define TNM_NODESREMOVED (WM_USER + 3) // unimplemented
#define TNM_NODESSTRUCTURED (WM_USER + 4)
#define TNM_ADDCOLUMN (WM_USER + 5)
#define TNM_REMOVECOLUMN (WM_USER + 6)
#define TNM_GETCOLUMN (WM_USER + 7)
#define TNM_SETCOLUMN (WM_USER + 8)
#define TNM_GETCOLUMNORDERARRAY (WM_USER + 9)
#define TNM_SETCOLUMNORDERARRAY (WM_USER + 10)
#define TNM_SETCURSOR (WM_USER + 11)
#define TNM_GETSORT (WM_USER + 12)
#define TNM_SETSORT (WM_USER + 13)
#define TNM_SETTRISTATE (WM_USER + 14)
#define TNM_ENSUREVISIBLE (WM_USER + 15)
#define TNM_SCROLL (WM_USER + 16)
#define TNM_GETFLATNODECOUNT (WM_USER + 17)
#define TNM_GETFLATNODE (WM_USER + 18)
#define TNM_GETCELLTEXT (WM_USER + 19)
#define TNM_SETNODEEXPANDED (WM_USER + 20)
#define TNM_GETMAXID (WM_USER + 21)
#define TNM_SETMAXID (WM_USER + 22)
#define TNM_INVALIDATENODE (WM_USER + 23)
#define TNM_INVALIDATENODES (WM_USER + 24)
#define TNM_GETFIXEDHEADER (WM_USER + 25)
#define TNM_GETHEADER (WM_USER + 26)
#define TNM_GETTOOLTIPS (WM_USER + 27)
#define TNM_SELECTRANGE (WM_USER + 28)
#define TNM_DESELECTRANGE (WM_USER + 29)
#define TNM_GETCOLUMNCOUNT (WM_USER + 30)
#define TNM_SETREDRAW (WM_USER + 31)
#define TNM_GETVIEWPARTS (WM_USER + 32)
#define TNM_GETFIXEDCOLUMN (WM_USER + 33)
#define TNM_GETFIRSTCOLUMN (WM_USER + 34)
#define TNM_SETFOCUSNODE (WM_USER + 35)
#define TNM_SETMARKNODE (WM_USER + 36)
#define TNM_SETHOTNODE (WM_USER + 37)
#define TNM_SETEXTENDEDFLAGS (WM_USER + 38)
#define TNM_GETCALLBACK (WM_USER + 39)
#define TNM_HITTEST (WM_USER + 40)
#define TNM_GETVISIBLECOLUMNCOUNT (WM_USER + 41)
#define TNM_AUTOSIZECOLUMN (WM_USER + 42)
#define TNM_SETEMPTYTEXT (WM_USER + 43)
#define TNM_SETROWHEIGHT (WM_USER + 44)
#define TNM_ISFLATNODEVALID (WM_USER + 45)
#define TNM_THEMESUPPORT (WM_USER + 46)
#define TNM_SETIMAGELIST (WM_USER + 47)
#define TNM_LAST (WM_USER + 48)

#define HRGN_FULL ((HRGN)1) // passed by WM_NCPAINT even though it's completely undocumented

#define TreeNew_SetCallback(hWnd, Callback, Context) \
    SendMessage((hWnd), TNM_SETCALLBACK, (WPARAM)(Context), (LPARAM)(Callback))

#define TreeNew_NodesStructured(hWnd) \
    SendMessage((hWnd), TNM_NODESSTRUCTURED, 0, 0)

#define TreeNew_AddColumn(hWnd, Column) \
    SendMessage((hWnd), TNM_ADDCOLUMN, 0, (LPARAM)(Column))

#define TreeNew_RemoveColumn(hWnd, Id) \
    SendMessage((hWnd), TNM_REMOVECOLUMN, (WPARAM)(Id), 0)

#define TreeNew_GetColumn(hWnd, Id, Column) \
    SendMessage((hWnd), TNM_GETCOLUMN, (WPARAM)(Id), (LPARAM)(Column))

#define TreeNew_SetColumn(hWnd, Mask, Column) \
    SendMessage((hWnd), TNM_SETCOLUMN, (WPARAM)(Mask), (LPARAM)(Column))

#define TreeNew_GetColumnOrderArray(hWnd, Count, Array) \
    SendMessage((hWnd), TNM_GETCOLUMNORDERARRAY, (WPARAM)(Count), (LPARAM)(Array))

#define TreeNew_SetColumnOrderArray(hWnd, Count, Array) \
    SendMessage((hWnd), TNM_SETCOLUMNORDERARRAY, (WPARAM)(Count), (LPARAM)(Array))

#define TreeNew_SetCursor(hWnd, Cursor) \
    SendMessage((hWnd), TNM_SETCURSOR, 0, (LPARAM)(Cursor))

#define TreeNew_GetSort(hWnd, Column, Order) \
    SendMessage((hWnd), TNM_GETSORT, (WPARAM)(Column), (LPARAM)(Order))

#define TreeNew_SetSort(hWnd, Column, Order) \
    SendMessage((hWnd), TNM_SETSORT, (WPARAM)(Column), (LPARAM)(Order))

#define TreeNew_SetTriState(hWnd, TriState) \
    SendMessage((hWnd), TNM_SETTRISTATE, (WPARAM)(TriState), 0)

#define TreeNew_EnsureVisible(hWnd, Node) \
    SendMessage((hWnd), TNM_ENSUREVISIBLE, 0, (LPARAM)(Node))

#define TreeNew_Scroll(hWnd, DeltaRows, DeltaX) \
    SendMessage((hWnd), TNM_SCROLL, (WPARAM)(DeltaRows), (LPARAM)(DeltaX))

#define TreeNew_GetFlatNodeCount(hWnd) \
    ((ULONG)SendMessage((hWnd), TNM_GETFLATNODECOUNT, 0, 0))

#define TreeNew_GetFlatNode(hWnd, Index) \
    ((PPH_TREENEW_NODE)SendMessage((hWnd), TNM_GETFLATNODE, (WPARAM)(Index), 0))

#define TreeNew_GetCellText(hWnd, GetCellText) \
    SendMessage((hWnd), TNM_GETCELLTEXT, 0, (LPARAM)(GetCellText))

#define TreeNew_SetNodeExpanded(hWnd, Node, Expanded) \
    SendMessage((hWnd), TNM_SETNODEEXPANDED, (WPARAM)(Expanded), (LPARAM)(Node))

#define TreeNew_GetMaxId(hWnd) \
    ((ULONG)SendMessage((hWnd), TNM_GETMAXID, 0, 0))

#define TreeNew_SetMaxId(hWnd, MaxId) \
    SendMessage((hWnd), TNM_SETMAXID, (WPARAM)(MaxId), 0)

#define TreeNew_InvalidateNode(hWnd, Node) \
    SendMessage((hWnd), TNM_INVALIDATENODE, 0, (LPARAM)(Node))

#define TreeNew_InvalidateNodes(hWnd, Start, End) \
    SendMessage((hWnd), TNM_INVALIDATENODES, (WPARAM)(Start), (LPARAM)(End))

#define TreeNew_GetFixedHeader(hWnd) \
    ((HWND)SendMessage((hWnd), TNM_GETFIXEDHEADER, 0, 0))

#define TreeNew_GetHeader(hWnd) \
    ((HWND)SendMessage((hWnd), TNM_GETHEADER, 0, 0))

#define TreeNew_GetTooltips(hWnd) \
    ((HWND)SendMessage((hWnd), TNM_GETTOOLTIPS, 0, 0))

#define TreeNew_SelectRange(hWnd, Start, End) \
    SendMessage((hWnd), TNM_SELECTRANGE, (WPARAM)(Start), (LPARAM)(End))

#define TreeNew_DeselectRange(hWnd, Start, End) \
    SendMessage((hWnd), TNM_DESELECTRANGE, (WPARAM)(Start), (LPARAM)(End))

#define TreeNew_GetColumnCount(hWnd) \
    ((ULONG)SendMessage((hWnd), TNM_GETCOLUMNCOUNT, 0, 0))

#define TreeNew_SetRedraw(hWnd, Redraw) \
    ((LONG)SendMessage((hWnd), TNM_SETREDRAW, (WPARAM)(Redraw), 0))

#define TreeNew_GetViewParts(hWnd, Parts) \
    SendMessage((hWnd), TNM_GETVIEWPARTS, 0, (LPARAM)(Parts))

#define TreeNew_GetFixedColumn(hWnd) \
    ((PPH_TREENEW_COLUMN)SendMessage((hWnd), TNM_GETFIXEDCOLUMN, 0, 0))

#define TreeNew_GetFirstColumn(hWnd) \
    ((PPH_TREENEW_COLUMN)SendMessage((hWnd), TNM_GETFIRSTCOLUMN, 0, 0))

#define TreeNew_SetFocusNode(hWnd, Node) \
    SendMessage((hWnd), TNM_SETFOCUSNODE, 0, (LPARAM)(Node))

#define TreeNew_SetMarkNode(hWnd, Node) \
    SendMessage((hWnd), TNM_SETMARKNODE, 0, (LPARAM)(Node))

#define TreeNew_SetHotNode(hWnd, Node) \
    SendMessage((hWnd), TNM_SETHOTNODE, 0, (LPARAM)(Node))

#define TreeNew_SetExtendedFlags(hWnd, Mask, Value) \
    SendMessage((hWnd), TNM_SETEXTENDEDFLAGS, (WPARAM)(Mask), (LPARAM)(Value))

#define TreeNew_GetCallback(hWnd, Callback, Context) \
    SendMessage((hWnd), TNM_GETCALLBACK, (WPARAM)(Context), (LPARAM)(Callback))

#define TreeNew_HitTest(hWnd, HitTest) \
    SendMessage((hWnd), TNM_HITTEST, 0, (LPARAM)(HitTest))

#define TreeNew_GetVisibleColumnCount(hWnd) \
    ((ULONG)SendMessage((hWnd), TNM_GETVISIBLECOLUMNCOUNT, 0, 0))

#define TreeNew_AutoSizeColumn(hWnd, Id, Flags) \
    SendMessage((hWnd), TNM_AUTOSIZECOLUMN, (WPARAM)(Id), (LPARAM)(Flags))

#define TreeNew_SetEmptyText(hWnd, Text, Flags) \
    SendMessage((hWnd), TNM_SETEMPTYTEXT, (WPARAM)(Flags), (LPARAM)(Text))

#define TreeNew_SetRowHeight(hWnd, RowHeight) \
    SendMessage((hWnd), TNM_SETROWHEIGHT, (WPARAM)(RowHeight), 0)

#define TreeNew_IsFlatNodeValid(hWnd) \
    ((BOOLEAN)SendMessage((hWnd), TNM_ISFLATNODEVALID, 0, 0))

#define TreeNew_ThemeSupport(hWnd, Enable) \
    SendMessage((hWnd), TNM_THEMESUPPORT, (WPARAM)(Enable), 0);

#define TreeNew_SetImageList(hWnd, ImageListHandle) \
    SendMessage((hWnd), TNM_SETIMAGELIST, (WPARAM)(ImageListHandle), 0);

#define TN_STYLE_ICONS 0x1
#define TN_STYLE_DOUBLE_BUFFERED 0x2
#define TN_STYLE_NO_DIVIDER 0x4
#define TN_STYLE_ANIMATE_DIVIDER 0x8
#define TN_STYLE_NO_COLUMN_SORT 0x10
#define TN_STYLE_NO_COLUMN_REORDER 0x20
#define TN_STYLE_THIN_ROWS 0x40
#define TN_STYLE_NO_COLUMN_HEADER 0x80
#define TN_STYLE_CUSTOM_COLORS 0x100
#define TN_STYLE_ALWAYS_SHOW_SELECTION 0x200

// Extended flags
#define TN_FLAG_ITEM_DRAG_SELECT 0x1
#define TN_FLAG_NO_UNFOLDING_TOOLTIPS 0x2

#define TNP_TOOLTIPS_ITEM 0
#define TNP_TOOLTIPS_FIXED_HEADER 1
#define TNP_TOOLTIPS_HEADER 2
#define TNP_TOOLTIPS_DEFAULT_MAXIMUM_WIDTH 550

// Object type flags
#define PH_OBJECT_TYPE_USE_FREE_LIST 0x00000001
#define PH_OBJECT_TYPE_VALID_FLAGS 0x00000001

#define PhAddObjectHeaderSize(Size) ((Size) + UFIELD_OFFSET(PH_OBJECT_HEADER, Body))

typedef VOID (NTAPI *PPH_TYPE_DELETE_PROCEDURE)(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

typedef struct _PH_FREE_LIST
{
    SLIST_HEADER ListHead;

    ULONG Count;
    ULONG MaximumCount;
    SIZE_T Size;
} PH_FREE_LIST, *PPH_FREE_LIST;
typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _QUAD_PTR
{
    ULONG_PTR DoNotUseThisField1;
    ULONG_PTR DoNotUseThisField2;
} QUAD_PTR, *PQUAD_PTR;
typedef struct _PH_FREE_LIST_ENTRY
{
    SLIST_ENTRY ListEntry;
    QUAD_PTR Body;
} PH_FREE_LIST_ENTRY, *PPH_FREE_LIST_ENTRY;
typedef struct _PH_OBJECT_TYPE
{
    /** The flags that were used to create the object type. */
    USHORT Flags;
    UCHAR TypeIndex;
    UCHAR Reserved;
    /** The total number of objects of this type that are alive. */
    ULONG NumberOfObjects;
    /** An optional procedure called when objects of this type are freed. */
    PPH_TYPE_DELETE_PROCEDURE DeleteProcedure;
    /** The name of the type. */
    PWSTR Name;
    /** A free list to use when allocating for this type. */
    PH_FREE_LIST FreeList;
} PH_OBJECT_TYPE, *PPH_OBJECT_TYPE;

typedef struct _PH_OBJECT_HEADER
{
    union
    {
        struct
        {
            USHORT TypeIndex;
            UCHAR Flags;
            UCHAR Reserved1;
#ifdef _WIN64
            ULONG Reserved2;
#endif
            union
            {
                LONG RefCount;
                struct
                {
                    LONG SavedTypeIndex : 16;
                    LONG SavedFlags : 8;
                    LONG Reserved : 7;
                    LONG DeferDelete : 1; // MUST be the high bit, so that RefCount < 0 when deferring delete
                };
            };
#ifdef _WIN64
            ULONG Reserved3;
#endif
        };
        SLIST_ENTRY DeferDeleteListEntry;
    };

#ifdef DEBUG
    PVOID StackBackTrace[16];
    LIST_ENTRY ObjectListEntry;
#endif

    /**
    * The body of the object. For use by the \ref PhObjectToObjectHeader and
    * \ref PhObjectHeaderToObject macros.
    */
    QUAD_PTR Body;
} PH_OBJECT_HEADER, *PPH_OBJECT_HEADER;


#define PH_TREENEW_CLASSNAME L"PhTreeNew"
#ifndef VSCLASS_TREEVIEW
#define VSCLASS_TREEVIEW	L"TREEVIEW"
#endif

typedef enum _PH_SORT_ORDER
{
    NoSortOrder = 0,
    AscendingSortOrder,
    DescendingSortOrder
} PH_SORT_ORDER, *PPH_SORT_ORDER;

typedef ULONG LOGICAL;
typedef ULONG *PLOGICAL;
typedef enum _PH_TREENEW_MESSAGE
{
    TreeNewGetChildren, // PPH_TREENEW_GET_CHILDREN Parameter1
    TreeNewIsLeaf, // PPH_TREENEW_IS_LEAF Parameter1
    TreeNewGetCellText, // PPH_TREENEW_GET_CELL_TEXT Parameter1
    TreeNewGetNodeColor, // PPH_TREENEW_GET_NODE_COLOR Parameter1
    TreeNewGetNodeFont, // PPH_TREENEW_GET_NODE_FONT Parameter1
    TreeNewGetNodeIcon, // PPH_TREENEW_GET_NODE_ICON Parameter1
    TreeNewGetCellTooltip, // PPH_TREENEW_GET_CELL_TOOLTIP Parameter1
    TreeNewCustomDraw, // PPH_TREENEW_CUSTOM_DRAW Parameter1

                       // Notifications
                       TreeNewNodeExpanding, // PPH_TREENEW_NODE Parameter1, PPH_TREENEW_NODE_EVENT Parameter2
                       TreeNewNodeSelecting, // PPH_TREENEW_NODE Parameter1

                       TreeNewSortChanged,
                       TreeNewSelectionChanged,

                       TreeNewKeyDown, // PPH_TREENEW_KEY_EVENT Parameter1
                       TreeNewLeftClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
                       TreeNewRightClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
                       TreeNewLeftDoubleClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
                       TreeNewRightDoubleClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
                       TreeNewContextMenu, // PPH_TREENEW_CONTEXT_MENU Parameter1

                       TreeNewHeaderRightClick, // PPH_TREENEW_HEADER_MOUSE_EVENT Parameter1
                       TreeNewIncrementalSearch, // PPH_TREENEW_SEARCH_EVENT Parameter1

                       TreeNewColumnResized, // PPH_TREENEW_COLUMN Parameter1
                       TreeNewColumnReordered,

                       TreeNewDestroying,
                       TreeNewGetDialogCode, // ULONG Parameter1, PULONG Parameter2

                       MaxTreeNewMessage
} PH_TREENEW_MESSAGE;
typedef struct _PH_LIST
{
    /** The number of items in the list. */
    ULONG Count;
    /** The number of items for which storage is allocated. */
    ULONG AllocatedCount;
    /** The array of list items. */
    PVOID *Items;
} PH_LIST, *PPH_LIST;
typedef BOOLEAN (NTAPI *PPH_TREENEW_CALLBACK)(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

typedef struct _PH_TREENEW_COLUMN
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Visible : 1;
            ULONG CustomDraw : 1;
            ULONG Fixed : 1; // Whether this is the fixed column
            ULONG SortDescending : 1; // Sort descending on initial click rather than ascending
            ULONG DpiScaleOnAdd : 1; // Whether to DPI scale the width (only when adding)
            ULONG SpareFlags : 27;
        };
    };
    ULONG Id;
    PVOID Context;
    PWSTR Text;
    LONG Width;
    ULONG Alignment;
    ULONG DisplayIndex; // -1 for fixed column or invalid

    ULONG TextFlags;

    struct
    {
        LONG ViewIndex; // Actual index in header control
        LONG ViewX; // 0 for the fixed column, and an offset from the divider for normal columns
    } s;
} PH_TREENEW_COLUMN, *PPH_TREENEW_COLUMN;
typedef struct _PH_TREENEW_NODE
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Visible : 1;
            ULONG Selected : 1;
            ULONG Expanded : 1;
            ULONG UseAutoForeColor : 1;
            ULONG UseTempBackColor : 1;
            ULONG Unselectable : 1;
            ULONG SpareFlags : 26;
        };
    };

    COLORREF BackColor;
    COLORREF ForeColor;
    COLORREF TempBackColor;
    HFONT Font;
    HICON Icon;

    //PPH_STRINGREF TextCache;
    ULONG TextCacheSize;

    ULONG Index; // Index within the flat list
    ULONG Level; // 0 for root, 1, 2, ...

    struct
    {
        union
        {
            ULONG Flags2;
            struct
            {
                ULONG IsLeaf : 1;
                ULONG CachedColorValid : 1;
                ULONG CachedFontValid : 1;
                ULONG CachedIconValid : 1;
                ULONG PlusMinusHot : 1;
                ULONG SpareFlags2 : 27;
            };
        };

        // Temp. drawing data
        COLORREF DrawBackColor;
        COLORREF DrawForeColor;
    } s;
} PH_TREENEW_NODE, *PPH_TREENEW_NODE;
typedef struct _PH_TREENEW_CONTEXT
{
    HWND Handle;
    PVOID InstanceHandle;
    HWND FixedHeaderHandle;
    HWND HeaderHandle;
    HWND VScrollHandle;
    HWND HScrollHandle;
    HWND FillerBoxHandle;
    HWND TooltipsHandle;

    union
    {
        struct
        {
            ULONG FontOwned : 1;
            ULONG Tracking : 1; // tracking for fixed divider
            ULONG VScrollVisible : 1;
            ULONG HScrollVisible : 1;
            ULONG FixedColumnVisible : 1;
            ULONG FixedDividerVisible : 1;
            ULONG AnimateDivider : 1;
            ULONG AnimateDividerFadingIn : 1;
            ULONG AnimateDividerFadingOut : 1;
            ULONG CanAnyExpand : 1;
            ULONG TriState : 1;
            ULONG HasFocus : 1;
            ULONG ThemeInitialized : 1; // delay load theme data
            ULONG ThemeActive : 1;
            ULONG ThemeHasItemBackground : 1;
            ULONG ThemeHasGlyph : 1;
            ULONG ThemeHasHotGlyph : 1;
            ULONG FocusNodeFound : 1; // used to preserve the focused node across restructuring
            ULONG SearchFailed : 1; // used to prevent multiple beeps
            ULONG SearchSingleCharMode : 1; // LV style single-character search
            ULONG TooltipUnfolding : 1; // whether the current tooltip is unfolding
            ULONG DoubleBuffered : 1;
            ULONG SuspendUpdateStructure : 1;
            ULONG SuspendUpdateLayout : 1;
            ULONG SuspendUpdateMoveMouse : 1;
            ULONG DragSelectionActive : 1;
            ULONG SelectionRectangleAlpha : 1; // use alpha blending for the selection rectangle
            ULONG CustomRowHeight : 1;
            ULONG CustomColors : 1;
            ULONG ContextMenuActive : 1;
            ULONG ThemeSupport : 1;
            ULONG ImageListSupport : 1;
        };
        ULONG Flags;
    };
    ULONG Style;
    ULONG ExtendedStyle;
    ULONG ExtendedFlags;

    HFONT Font;
    HCURSOR Cursor;
    HCURSOR DividerCursor;

    RECT ClientRect;
    LONG HeaderHeight;
    LONG RowHeight;
    ULONG VScrollWidth;
    ULONG HScrollHeight;
    LONG VScrollPosition;
    LONG HScrollPosition;
    LONG FixedWidth; // width of the fixed part of the tree list
    LONG FixedWidthMinimum;
    LONG NormalLeft; // FixedWidth + 1 if there is a fixed column, otherwise 0

    PPH_TREENEW_NODE FocusNode;
    ULONG HotNodeIndex;
    ULONG MarkNodeIndex; // selection mark

    ULONG MouseDownLast;
    POINT MouseDownLocation;

    PPH_TREENEW_CALLBACK Callback;
    PVOID CallbackContext;

    PPH_TREENEW_COLUMN *Columns; // columns, indexed by ID
    ULONG NextId;
    ULONG AllocatedColumns;
    ULONG NumberOfColumns; // just a statistic; do not use for actual logic

    PPH_TREENEW_COLUMN *ColumnsByDisplay; // columns, indexed by display order (excluding the fixed column)
    ULONG AllocatedColumnsByDisplay;
    ULONG NumberOfColumnsByDisplay; // the number of visible columns (excluding the fixed column)
    LONG TotalViewX; // total width of normal columns
    PPH_TREENEW_COLUMN FixedColumn;
    PPH_TREENEW_COLUMN FirstColumn; // first column, by display order (including the fixed column)
    PPH_TREENEW_COLUMN LastColumn; // last column, by display order (including the fixed column)

    PPH_TREENEW_COLUMN ResizingColumn;
    LONG OldColumnWidth;
    LONG TrackStartX;
    LONG TrackOldFixedWidth;
    ULONG DividerHot; // 0 for un-hot, 100 for completely hot

    PPH_LIST FlatList;

    ULONG SortColumn; // ID of the column to sort by
    PH_SORT_ORDER SortOrder;

    FLOAT VScrollRemainder;
    FLOAT HScrollRemainder;

    LONG SearchMessageTime;
    PWSTR SearchString;
    ULONG SearchStringCount;
    ULONG AllocatedSearchString;

    ULONG TooltipIndex;
    ULONG TooltipId;
    //PPH_STRING TooltipText;
    RECT TooltipRect; // text rectangle of an unfolding tooltip
    HFONT TooltipFont;
    HFONT NewTooltipFont;
    ULONG TooltipColumnId;

    TEXTMETRIC TextMetrics;
    HTHEME ThemeData;
    COLORREF DefaultBackColor;
    COLORREF DefaultForeColor;

    // User configurable colors.
    COLORREF CustomTextColor;
    COLORREF CustomFocusColor;
    COLORREF CustomSelectedColor;

    LONG SystemBorderX;
    LONG SystemBorderY;
    LONG SystemEdgeX;
    LONG SystemEdgeY;

    HDC BufferedContext;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;

    LONG SystemDragX;
    LONG SystemDragY;
    RECT DragRect;

    LONG EnableRedraw;
    HRGN SuspendUpdateRegion;

    //PH_STRINGREF EmptyText;

    WNDPROC HeaderWindowProc;
    WNDPROC FixedHeaderWindowProc;
    HIMAGELIST ImageListHandle;
} PH_TREENEW_CONTEXT, *PPH_TREENEW_CONTEXT;

#define REF_STAT_UP(Name) PHLIB_INC_STATISTIC(Name)
#ifdef DEBUG
#define PHLIB_INC_STATISTIC(Name) (_InterlockedIncrement(&PhLibStatisticsBlock.Name))
#else
#define PHLIB_INC_STATISTIC(Name)
#endif

PVOID           PhAllocateZero(
    _In_ SIZE_T Size
);
PVOID           PhAllocate(
    _In_ SIZE_T Size
);
void            PhFree(_Frees_ptr_opt_ PVOID Memory);


PPH_LIST PhCreateList(
    _In_ ULONG InitialCapacity
);
