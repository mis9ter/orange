#pragma once

#include <vector>

typedef std::shared_ptr<PH_TREENEW_COLUMN>	ColumnPtr;
typedef std::shared_ptr<PH_TREENEW_NODE>	NodePtr;

typedef struct _WINDOW_CONTEXT {
	_WINDOW_CONTEXT() {
		DWORD	dwOffset	= FIELD_OFFSET(_WINDOW_CONTEXT, hInstance);
		ZeroMemory((char *)this + dwOffset, sizeof(_WINDOW_CONTEXT) - dwOffset);

	}
	~_WINDOW_CONTEXT() {

	}
	std::vector<ColumnPtr>	columns;
	std::wstring			EmptyText;
	std::vector<NodePtr>	FlatList;
	NodePtr					FocusNode;

	std::wstring			TooltipText;
	std::vector<ColumnPtr>	ColumnsByDisplay;
	ColumnPtr				FixedColumn;
	ColumnPtr				FirstColumn;
	ColumnPtr				LastColumn;
	std::vector<ColumnPtr>	Columns;

	HINSTANCE		hInstance;
	DWORD			dwStyle;
	DWORD			dwExStyle;
	Setting::Color	color;
	PVOID			pClass;
	DWORD			dwExtendedFlags;
	PPH_TREENEW_CALLBACK	pCallback;
	PVOID			pCallbackContext;

	ULONG			NextId;
	ULONG			AllocatedColumns;
	LONG			nEnableRedraw;
	HRGN			SuspendUpdateRegion;

	HIMAGELIST		hImageList;

	LONG SystemBorderX;
	LONG SystemBorderY;
	LONG SystemEdgeX;
	LONG SystemEdgeY;
	LONG SystemDragX;
	LONG SystemDragY;
	RECT DragRect;

	WNDPROC HeaderWindowProc;
	WNDPROC FixedHeaderWindowProc;


	ULONG HotNodeIndex;
	ULONG MarkNodeIndex; // selection mark
	HTHEME ThemeData;
	//PPH_LIST FlatList;
	
	ULONG DividerHot; // 0 for un-hot, 100 for completely hot

	ULONG TooltipIndex;
	ULONG TooltipId;
	
	RECT TooltipRect; // text rectangle of an unfolding tooltip
	HFONT TooltipFont;
	HFONT NewTooltipFont;
	HFONT	Font;
	ULONG TooltipColumnId;

	ULONG SortColumn; // ID of the column to sort by
	PH_SORT_ORDER SortOrder;
	

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
	LONG TotalViewX; // total width of normal columns

	TEXTMETRIC TextMetrics;

	union 
	{
		struct {
			int		bDoubleBuffered		: 1;
			int		bTracking			: 1;
			int		bAnimateDivider		: 1;
			int		bCustomColor		: 1;
			int		bImageList			: 1;
			ULONG	SuspendUpdateStructure : 1;
			ULONG SuspendUpdateLayout : 1;
			ULONG SuspendUpdateMoveMouse : 1;
			ULONG FixedDividerVisible : 1;
			ULONG FocusNodeFound : 1; // used to preserve the focused node across restructuring
			ULONG CanAnyExpand : 1;
			ULONG VScrollVisible : 1;
			ULONG HScrollVisible : 1;
			ULONG	AnimateDivider : 1;
			ULONG AnimateDividerFadingIn : 1;
			ULONG AnimateDividerFadingOut : 1;
			ULONG FixedColumnVisible : 1;
			ULONG FontOwned : 1;
			ULONG CustomRowHeight : 1;
			ULONG TooltipUnfolding : 1; // whether the current tooltip is unfolding
		};
		DWORD		dwFlags;
	};

	struct {
		HANDLE		hDummy;
	
	} handle;
	HWND			hWnd;
	struct {
		HWND		hFixedHeader;
		HWND		hHeader;
		HWND		hVScroll;
		HWND		hHScroll;
		HWND		hFilterBox;	
		HWND		hTooltip;
		HWND		hFillerBox;
	}	window;

} WINDOW_CONTEXT, *PWINDOW_CONTEXT;

typedef std::shared_ptr<WINDOW_CONTEXT>			WindowContextPtr;
typedef std::map<HWND, WindowContextPtr>		WindowContextTable;
typedef std::function<void (PWINDOW_CONTEXT)>	WindowContextCallback;

class CWindowContextTable
	:
		virtual public	CAppLog
{
public:
	CWindowContextTable() {

	}
	~CWindowContextTable() {
		m_table.clear();
	}
	WindowContextPtr		AllocateWindowContext(IN HWND hWnd, IN WindowContextCallback pCallback)
	{
		WindowContextPtr		ptr	= nullptr;
		try {
			ptr	= std::make_shared<WINDOW_CONTEXT>();
			if( pCallback ) {
				pCallback(ptr.get());
			}
		}
		catch( std::exception & e ) {
			UNREFERENCED_PARAMETER(e);
			//Log("%-32s %s", __FUNCTION__, e.what());			
		}
		if( ptr ) {
			__function_lock(m_lock.Get());
			m_table[hWnd]	= ptr;
		}
		return ptr;
	}
	bool					AddWindowContext(IN HWND hWnd, IN WindowContextPtr ptr) {
		__function_lock(m_lock.Get());
		auto	t	= m_table.find(hWnd);
		if( m_table.end() == t )	{
			m_table[hWnd]	= ptr;
			//Log("%-32s %p %p", __func__, hWnd, ptr.get());
			return true;
		}
		Log("%-32s %p %p failed.", __func__, hWnd, ptr.get());
		return false;
	}
	bool						RemoveWindowContext(IN HWND hWnd) {
		__function_lock(m_lock.Get());
		auto	t	= m_table.find(hWnd);
		if( m_table.end() == t )	{
			Log("%-32s %p failed.", __func__, hWnd);
			return false;
		}
		m_table.erase(t);
		//Log("%-32s %p", __func__, hWnd);
		return true;
	}
	void					FreeWindowContext(IN HWND hWnd, IN WindowContextCallback pCallback)
	{
		__function_lock(m_lock.Get());
		auto	t	= m_table.find(hWnd);
		if( m_table.end() != t ) {
			if( pCallback )	pCallback(t->second.get());
			m_table.erase(t);
		}
	}
	WindowContextPtr		GetWindowContext(IN HWND hWnd) {
		__function_lock(m_lock.Get());
		auto	t	= m_table.find(hWnd);
		if( m_table.end() == t     )	{
			Log("%-32s %p failed.", __func__, hWnd);
			return nullptr;
		}
		WindowContextPtr	ptr	= t->second;
		//Log("%-32s %p %p", __func__, hWnd, ptr.get());
		return ptr;
	}
private:
	WindowContextTable		m_table;
	CLock					m_lock;
};
