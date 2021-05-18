#pragma once

//#include <intrin.h>
//#include <ntstatus.h>

#pragma comment(lib, "ntdll.lib")

extern "C" {


	NTSYSAPI
		DECLSPEC_NORETURN
		VOID
		NTAPI
		RtlRaiseStatus(
			_In_ NTSTATUS Status
		);

};

#define _May_raise_

#define PH_VECTOR_LEVEL_NONE 0
#define PH_VECTOR_LEVEL_SSE2 1
#define PH_VECTOR_LEVEL_AVX 2


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

// Styles
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

// Callback flags
#define TN_CACHE 0x1
#define TN_AUTO_FORECOLOR 0x1000

// Column change flags
#define TN_COLUMN_CONTEXT 0x1
#define TN_COLUMN_TEXT 0x2
#define TN_COLUMN_WIDTH 0x4
#define TN_COLUMN_ALIGNMENT 0x8
#define TN_COLUMN_DISPLAYINDEX 0x10
#define TN_COLUMN_TEXTFLAGS 0x20
#define TN_COLUMN_FLAG_VISIBLE 0x100000
#define TN_COLUMN_FLAG_CUSTOMDRAW 0x200000
#define TN_COLUMN_FLAG_FIXED 0x400000
#define TN_COLUMN_FLAG_SORTDESCENDING 0x800000
#define TN_COLUMN_FLAG_NODPISCALEONADD 0x1000000
#define TN_COLUMN_FLAGS 0xfff00000

// Cache flags
#define TN_CACHE_COLOR 0x1
#define TN_CACHE_FONT 0x2
#define TN_CACHE_ICON 0x4

// Cell part input flags
#define TN_MEASURE_TEXT 0x1

// Cell part flags
#define TN_PART_CELL 0x1
#define TN_PART_PLUSMINUS 0x2
#define TN_PART_ICON 0x4
#define TN_PART_CONTENT 0x8
#define TN_PART_TEXT 0x10

// Hit test input flags
#define TN_TEST_COLUMN 0x1
#define TN_TEST_SUBITEM 0x2 // requires TN_TEST_COLUMN

#define TNP_TOOLTIPS_ITEM 0
#define TNP_TOOLTIPS_FIXED_HEADER 1
#define TNP_TOOLTIPS_HEADER 2
#define TNP_TOOLTIPS_DEFAULT_MAXIMUM_WIDTH 550

#define TNP_TIMER_NULL 1
#define TNP_TIMER_ANIMATE_DIVIDER 2

#define TNP_ANIMATE_DIVIDER_INTERVAL 10
#define TNP_ANIMATE_DIVIDER_INCREMENT 17
#define TNP_ANIMATE_DIVIDER_DECREMENT 2

#define TNP_CELL_LEFT_MARGIN 6
#define TNP_CELL_RIGHT_MARGIN 6
#define TNP_ICON_RIGHT_PADDING 4


// Hit test flags
#define TN_HIT_LEFT 0x1
#define TN_HIT_RIGHT 0x2
#define TN_HIT_ABOVE 0x4
#define TN_HIT_BELOW 0x8
#define TN_HIT_ITEM 0x10
#define TN_HIT_ITEM_PLUSMINUS 0x20 // requires TN_TEST_SUBITEM
#define TN_HIT_ITEM_ICON 0x40 // requires TN_TEST_SUBITEM
#define TN_HIT_ITEM_CONTENT 0x80 // requires TN_TEST_SUBITEM
#define TN_HIT_DIVIDER 0x100

// Selection flags
#define TN_SELECT_DESELECT 0x1
#define TN_SELECT_TOGGLE 0x2
#define TN_SELECT_RESET 0x4

// Auto-size flags
#define TN_AUTOSIZE_REMAINING_SPACE 0x1

#define PH_ALIGN_CENTER 0x0
#define PH_ALIGN_LEFT 0x1
#define PH_ALIGN_RIGHT 0x2
#define PH_ALIGN_TOP 0x4
#define PH_ALIGN_BOTTOM 0x8

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#define     SelectFont(hdc, hfont)  ((HFONT)SelectObject((hdc), (HGDIOBJ)(HFONT)(hfont)))

#define     DeleteRgn(hrgn)      DeleteObject((HGDIOBJ)(HRGN)(hrgn))
#define TNP_HIT_TEST_FIXED_DIVIDER(X, Context) \
    ((Context)->FixedDividerVisible && (X) >= (Context)->FixedWidth - 8 && (X) < (Context)->FixedWidth + 8)

#define TreeNew_GetTooltips(hWnd) \
    ((HWND)SendMessage((hWnd), TNM_GETTOOLTIPS, 0, 0))
#define TreeNew_SetCallback(hWnd, Callback, Context) \
    SendMessage((hWnd), TNM_SETCALLBACK, (WPARAM)(Context), (LPARAM)(Callback))
#define TreeNew_SetImageList(hWnd, ImageListHandle) \
    SendMessage((hWnd), TNM_SETIMAGELIST, (WPARAM)(ImageListHandle), 0);
#define TreeNew_AddColumn(hWnd, Column) \
    SendMessage((hWnd), TNM_ADDCOLUMN, 0, (LPARAM)(Column))
#define TreeNew_SetMaxId(hWnd, MaxId) \
    SendMessage((hWnd), TNM_SETMAXID, (WPARAM)(MaxId), 0)
#define TreeNew_SetRedraw(hWnd, Redraw) \
    ((LONG)SendMessage((hWnd), TNM_SETREDRAW, (WPARAM)(Redraw), 0))

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

typedef enum _PH_SORT_ORDER
{
	NoSortOrder = 0,
	AscendingSortOrder,
	DescendingSortOrder
} PH_SORT_ORDER, *PPH_SORT_ORDER;

typedef bool (NTAPI *PPH_TREENEW_CALLBACK)(
	_In_ HWND hwnd,
	_In_ PH_TREENEW_MESSAGE Message,
	_In_opt_ PVOID Parameter1,
	_In_opt_ PVOID Parameter2,
	_In_opt_ PVOID Context
	);

typedef ULONG LOGICAL;
typedef ULONG *PLOGICAL;

typedef struct _PH_WINDOW_PROPERTY_CONTEXT
{
	ULONG PropertyHash;
	HWND WindowHandle;
	PVOID Context;
} PH_WINDOW_PROPERTY_CONTEXT, *PPH_WINDOW_PROPERTY_CONTEXT;

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

	std::wstring TextCache;
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


typedef struct _PH_TREENEW_GET_CHILDREN
{
	ULONG Flags;
	PPH_TREENEW_NODE Node;

	ULONG NumberOfChildren;
	PPH_TREENEW_NODE *Children; // can be NULL if no children
} PH_TREENEW_GET_CHILDREN, *PPH_TREENEW_GET_CHILDREN;

typedef struct _PH_TREENEW_IS_LEAF
{
	ULONG Flags;
	PPH_TREENEW_NODE Node;

	BOOLEAN IsLeaf;
} PH_TREENEW_IS_LEAF, *PPH_TREENEW_IS_LEAF;

typedef struct _PH_TREENEW_HIT_TEST
{
	POINT Point;
	ULONG InFlags;

	ULONG Flags;
	PPH_TREENEW_NODE Node;
	PPH_TREENEW_COLUMN Column; // requires TN_TEST_COLUMN
} PH_TREENEW_HIT_TEST, *PPH_TREENEW_HIT_TEST;

typedef struct _PH_LIST
{
	/** The number of items in the list. */
	ULONG Count;
	/** The number of items for which storage is allocated. */
	ULONG AllocatedCount;
	/** The array of list items. */
	PVOID *Items;
} PH_LIST, *PPH_LIST;





typedef struct _PH_LAYOUT_PADDING_DATA
{
	RECT Padding;
} PH_LAYOUT_PADDING_DATA, *PPH_LAYOUT_PADDING_DATA;

typedef enum _PH_MAIN_TAB_PAGE_MESSAGE
{
	MainTabPageCreate,
	MainTabPageDestroy,
	MainTabPageCreateWindow, // HWND *Parameter1 (WindowHandle)
	MainTabPageSelected, // BOOLEAN Parameter1 (Selected)
	MainTabPageInitializeSectionMenuItems, // PPH_MAIN_TAB_PAGE_MENU_INFORMATION Parameter1

	MainTabPageLoadSettings,
	MainTabPageSaveSettings,
	MainTabPageExportContent, // PPH_MAIN_TAB_PAGE_EXPORT_CONTENT Parameter1
	MainTabPageFontChanged, // HFONT Parameter1 (Font)
	MainTabPageUpdateAutomaticallyChanged, // BOOLEAN Parameter1 (UpdateAutomatically)

	MaxMainTabPageMessage
} PH_MAIN_TAB_PAGE_MESSAGE;

struct _MAIN_TAB_PAGE;
typedef	bool	(*PMainTabPageCallback)	
(
	_In_		_MAIN_TAB_PAGE				*pPage,
	_In_		PH_MAIN_TAB_PAGE_MESSAGE	Message,
	_In_opt_	PVOID						pParameter1,
	_In_opt_	PVOID						pParameter2
	);

typedef struct _MAIN_TAB_PAGE {
	_MAIN_TAB_PAGE() {
		DWORD	dwOffset	= FIELD_OFFSET(_MAIN_TAB_PAGE, dwFlags);
		ZeroMemory((char *)this + dwOffset, sizeof(_MAIN_TAB_PAGE) - dwOffset);
		//CONTAINING_RECORD
	
	}
	std::wstring			strName;
	DWORD					dwFlags;
	PMainTabPageCallback	pCallback;
	PVOID					pContext;
	int						nIndex;

	union {
		DWORD				dwStateFlags;
		struct {
		
			DWORD			dwSelected				: 1;
			DWORD			dwCreateWindowCalled	: 1;
			DWORD			dwSpareStateFlags		: 30;
		};
	};
	PVOID	pReserved[2];
	HWND	hWnd;

} MAIN_TAB_PAGE, *PMAIN_TAB_PAGE;

typedef	std::shared_ptr<MAIN_TAB_PAGE>		MainTabPagePtr;



