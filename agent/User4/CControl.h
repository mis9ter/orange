#pragma once

#include <CommCtrl.h>
#include <Uxtheme.h>
#include <functional>

#include "user.define.h"
#include "CWindowContext.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "UxTheme.lib")

#define TAB_CLASSNAME		L"TabTabTab"
#define __log_function__	Log("%-32s", __func__)

// Styles
#define TN_STYLE_ICONS					0x1
#define TN_STYLE_DOUBLE_BUFFERED		0x2
#define TN_STYLE_NO_DIVIDER				0x4
#define TN_STYLE_ANIMATE_DIVIDER		0x8
#define TN_STYLE_NO_COLUMN_SORT			0x10
#define TN_STYLE_NO_COLUMN_REORDER		0x20
#define TN_STYLE_THIN_ROWS				0x40
#define TN_STYLE_NO_COLUMN_HEADER		0x80
#define TN_STYLE_CUSTOM_COLORS			0x100
#define TN_STYLE_ALWAYS_SHOW_SELECTION	0x200


typedef struct {
	PVOID			pClass;
	bool			bCustomColor;
	Setting::Color	color;

} WND_CREATE_PARAMS, *PWND_CREATE_PARAMS;


class CTab
	:
	virtual public	CAppLog,
	virtual public	CSettings,
	virtual	public	CWindowContextTable
{
public:
	CTab() 
	:
		m_hTabWnd(NULL),
		m_LayoutPadding({0,0,0,0}),
		m_bLayoutPaddingValid(true),
		m_nGlobalDpi(96)
	
	{


	}
	~CTab() {
	
	}

	int					AddTabControlTab(IN HWND hTab, IN int nIndex, IN PCWSTR pText) {
		TCITEM item;
		item.mask		= TCIF_TEXT;
		item.pszText	= (PWSTR)pText;
		item.iImage		= -1;
		return TabCtrl_InsertItem(hTab, nIndex, &item);	
	}
	VOID	UpdateLayoutPadding(
		VOID
	)
	{
		PH_LAYOUT_PADDING_DATA data;
		memset(&data, 0, sizeof(PH_LAYOUT_PADDING_DATA));
		//PhInvokeCallback(&LayoutPaddingCallback, &data);
		m_LayoutPadding = data.Padding;
	}
	VOID ApplyLayoutPadding(
		_Inout_ PRECT Rect,
		_In_ PRECT Padding
	)
	{
		Rect->left += Padding->left;
		Rect->top += Padding->top;
		Rect->right -= Padding->right;
		Rect->bottom -= Padding->bottom;
	}
	VOID LayoutTabControl
	(
		_In_	HWND	hMainWnd,
		_In_	HWND	hTab,
		_Inout_ HDWP	*DeferHandle
	)
	{
		RECT rect;
		if (m_bLayoutPaddingValid)
		{
			Log("%-32s LayoutPaddingValid", __func__);
			UpdateLayoutPadding();
			m_bLayoutPaddingValid	= true;
		}
		GetClientRect(hMainWnd, &rect);
		Log("%-32s %d %d %d %d", __func__, rect.left, rect.top, rect.right, rect.bottom);
		ApplyLayoutPadding(&rect, &m_LayoutPadding);
		TabCtrl_AdjustRect(hTab, FALSE, &rect);

		if (m_currentPage && m_currentPage->hWnd)
		{
			Log("%-32s currentPage", __func__);
			*DeferHandle = DeferWindowPos(*DeferHandle, m_currentPage->hWnd, NULL,
				rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
				SWP_NOACTIVATE | SWP_NOZORDER);
		}
	}
	FORCEINLINE BOOLEAN AddColumn(
		_In_ HWND hwnd,
		_In_ ULONG Id,
		_In_ BOOLEAN Visible,
		_In_ PCWSTR Text,
		_In_ ULONG Width,
		_In_ ULONG Alignment,
		_In_ ULONG DisplayIndex,
		_In_ ULONG TextFlags
	)
	{
		PH_TREENEW_COLUMN column;


		Log("%-32s %ws", __FUNCTION__, Text);

		memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
		column.Id = Id;
		column.Visible = Visible;
		column.Text			= (PWSTR)Text;
		column.Width = Width;
		column.Alignment = Alignment;
		column.DisplayIndex = DisplayIndex;
		column.TextFlags = TextFlags;
		column.DpiScaleOnAdd = TRUE;

		if (DisplayIndex == -2)
			column.Fixed = TRUE;

		return !!TreeNew_AddColumn(hwnd, &column);
	}
	MainTabPagePtr		CreatePage(
		_In_	HWND			hMainWnd,
		_In_	HWND			hTab, 
		_In_	PMAIN_TAB_PAGE	pTemplate
	)
	{
		MainTabPagePtr		ptr;
		std::wstring		name;
		HDWP				deferHandle;

		try {
			ptr		= std::make_shared<MAIN_TAB_PAGE>();
			ptr->strName	= pTemplate->strName;
			ptr->dwFlags	= pTemplate->dwFlags;
			ptr->pCallback	= pTemplate->pCallback;
			ptr->pContext	= pTemplate->pContext;
			ptr->nIndex		= AddTabControlTab(hTab, MAXINT, ptr->strName.c_str());

			Log("%-32s nIndex=%d", __func__, ptr->nIndex);

			if( ptr->pCallback ) 
				ptr->pCallback(ptr.get(), MainTabPageCreate, NULL, NULL);

			deferHandle = BeginDeferWindowPos(1);
			LayoutTabControl(hMainWnd, hTab, &deferHandle);
			EndDeferWindowPos(deferHandle);
		}
		catch( std::exception & e ) {
			UNREFERENCED_PARAMETER(e);



			Log("%-32s exception:%s", __func__, e.what());

		}
		return ptr;
	}
	bool	CreateTab(IN HINSTANCE hInstance, IN HWND hWnd,
		IN bool bCustomColor, IN Setting::Color	& color)
	{
		Log(__FUNCTION__);

		ULONG	thinRows			= 0;
		ULONG	treelistBorder		= 0;
		ULONG	treelistCustomColors	= 0;

		bool	bEnableThemeSupport	= !!GetInteger(L"EnableThemeSupport");

		thinRows				= GetInteger(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;
		treelistBorder			= (GetInteger(L"TreeListBorderEnable") && !bEnableThemeSupport) ? WS_BORDER : 0;
		treelistCustomColors	= GetInteger(L"TreeListCustomColorsEnable") ? TN_STYLE_CUSTOM_COLORS : 0;

		RegisterTabProc(hInstance);

		WND_CREATE_PARAMS	params;
		params.pClass			= this;
		params.bCustomColor		= bCustomColor;
		params.color			= color;

		m_hTabWnd = CreateWindow(
			WC_TABCONTROL,
			NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
			0,
			0,
			3,
			3,
			hWnd,
			NULL,
			hInstance,
			&params
		);

		Log("%-32s m_hTabWnd=%p", __func__, m_hTabWnd);
		m_hProcessWnd = CreateWindow(
			TAB_CLASSNAME,
			NULL,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | TN_STYLE_ANIMATE_DIVIDER | thinRows | treelistBorder | treelistCustomColors,
			0,
			0,
			3,
			3,
			hWnd,
			NULL,
			hInstance,
			&params
		);
		int		nRet	= 0;

		PCWSTR	pNames[]	= {
			L"Process",
			L"Product",
		};

		MAIN_TAB_PAGE	tab;

		tab.hWnd		= hWnd;
		tab.strName		= L"Process";
		tab.pCallback	= ProcessPageCallback;
		tab.pContext	= this;

		MainTabPagePtr	ptr	= CreatePage(hWnd, m_hTabWnd, &tab);
		SetProcessTab(m_hProcessWnd);





		ShowWindow(m_hTabWnd, SW_SHOW);
		return m_hTabWnd;
	}

	void	SetProcessTab(HWND hWnd)
	{
		SetWindowTheme(hWnd, (PWSTR)L"explorer", NULL);
		SendMessage(hWnd, TNM_SETEXTENDEDFLAGS, (WPARAM)(TN_FLAG_ITEM_DRAG_SELECT), (LPARAM)(TN_FLAG_ITEM_DRAG_SELECT));
		SendMessage(TreeNew_GetTooltips(hWnd), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);
		TreeNew_SetCallback(hWnd, TreeNewCallback, NULL);
		TreeNew_SetImageList(hWnd, m_ProcessSmallImageList);
		TreeNew_SetMaxId(hWnd, 91 - 1);
		TreeNew_SetRedraw(hWnd, FALSE);

		// Default columns
		AddColumn(hWnd, 0, TRUE, L"Name", 200, PH_ALIGN_LEFT, 
			(GetInteger(L"ProcessTreeListNameDefault") ? -2 : 0), 0); // HACK (dmex)
		//AddTreeNewColumn(hWnd, PHPRTLC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 0, DT_RIGHT);
		//AddTreeNewColumnEx(hWnd, PHPRTLC_CPU, TRUE, L"CPU", 45, PH_ALIGN_RIGHT, 1, DT_RIGHT, TRUE);
		//AddTreeNewColumnEx(hWnd, PHPRTLC_IOTOTALRATE, TRUE, L"I/O total rate", 70, PH_ALIGN_RIGHT, 2, DT_RIGHT, TRUE);
		//AddTreeNewColumnEx(hWnd, PHPRTLC_PRIVATEBYTES, TRUE, L"Private bytes", 70, PH_ALIGN_RIGHT, 3, DT_RIGHT, TRUE);
		//AddTreeNewColumn(hWnd, PHPRTLC_USERNAME, TRUE, L"User name", 140, PH_ALIGN_LEFT, 4, DT_PATH_ELLIPSIS);
		//AddTreeNewColumn(hWnd, PHPRTLC_DESCRIPTION, TRUE, L"Description", 180, PH_ALIGN_LEFT, 5, 0);


		//TreeNew_SetRedraw(hWnd, TRUE);
		//TreeNew_SetTriState(hWnd, TRUE);
		//TreeNew_SetSort(hWnd, 0, NoSortOrder);
	}

	void	InitCommonControls()
	{
		INITCOMMONCONTROLSEX icex;

		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC =
			ICC_LISTVIEW_CLASSES |
			ICC_TREEVIEW_CLASSES |
			ICC_BAR_CLASSES |
			ICC_TAB_CLASSES |
			ICC_PROGRESS_CLASS |
			ICC_COOL_CLASSES |
			ICC_STANDARD_CLASSES |
			ICC_LINK_CLASS
			;

		InitCommonControlsEx(&icex);
	}
	bool	RegisterTabProc(HINSTANCE hInstance)
	{
		WNDCLASSEX c = { sizeof(c) };

		c.style = CS_DBLCLKS | CS_GLOBALCLASS;
		c.lpfnWndProc	= TabProc;
		c.cbClsExtra	= 0;
		c.cbWndExtra	= sizeof(PVOID);
		c.hInstance		= hInstance;
		c.hIcon			= NULL;
		c.hCursor		= LoadCursor(NULL, IDC_ARROW);
		c.hbrBackground = NULL;
		c.lpszMenuName	= NULL;
		c.lpszClassName	= TAB_CLASSNAME;
		c.hIconSm = NULL;

		if (!RegisterClassEx(&c))
			return false;

		m_nSmallIconWidth = GetSystemMetrics(SM_CXSMICON);
		m_nSmallIconHeight = GetSystemMetrics(SM_CYSMICON);

		return true;
	}

private:
	HWND			m_hTabWnd;
	HWND			m_hProcessWnd;
	int				m_nSmallIconWidth;
	int				m_nSmallIconHeight;
	RECT			m_LayoutPadding;
	bool			m_bLayoutPaddingValid;
	MainTabPagePtr	m_currentPage;
	HIMAGELIST		m_ProcessLargeImageList = NULL;
	HIMAGELIST		m_ProcessSmallImageList = NULL;
	ULONG			m_nGlobalDpi;

	static	bool	ProcessPageCallback(
		_In_ struct _MAIN_TAB_PAGE *Page,
		_In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
		_In_opt_ PVOID Parameter1,
		_In_opt_ PVOID Parameter2
	);
	static	bool	TreeNewCallback(
		_In_ HWND hwnd,
		_In_ PH_TREENEW_MESSAGE Message,
		_In_opt_ PVOID Parameter1,
		_In_opt_ PVOID Parameter2,
		_In_opt_ PVOID Context
	);
	static	bool	NullCallback(
		_In_ HWND hwnd,
		_In_ PH_TREENEW_MESSAGE Message,
		_In_opt_ PVOID Parameter1,
		_In_opt_ PVOID Parameter2,
		_In_opt_ PVOID Context
	)
	{
		return FALSE;
	}
	static	LRESULT CALLBACK TabProc(
        _In_ HWND	hWnd,
        _In_ UINT	uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
    )
    {
        // Note: if we have suspended restructuring, we *cannot* access any nodes, because all node
        // pointers are now invalid. Below, we disable all input.
		static	CTab	*pClass;
		if( WM_NCCREATE == uMsg ) {
			//	최초 윈도 메세지 오는 순서
			//	WM_NCCREATE
			//	WM_NCCALCSIZE
			//	WM_CREATE
			PWND_CREATE_PARAMS	p	= (PWND_CREATE_PARAMS)((CREATESTRUCT *)lParam)->lpCreateParams;
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)p->pClass);
			pClass	= (CTab *)p->pClass;
			pClass->Log("%-32s WM_NCCREATE", __FUNCTION__);
			pClass->OnCreate(hWnd, (CREATESTRUCT *)lParam);
		}
		//	함수 호출 한번이라도 줄이다.
		//	CTab	*pClass	= (CTab *)GetWindowLongPtr(hWnd, 0);
		if( NULL == pClass ) {
			WCHAR	szBuf[100];

			StringCbPrintf(szBuf, sizeof(szBuf), L"%x", uMsg);
			MessageBox(NULL, szBuf, L"", 0);
			return DefWindowProc(hWnd, uMsg, wParam, lParam);		
		}
		//pClass->Log("%-32s %p %d", __func__, hWnd, uMsg);		

        switch (uMsg)
        {   
			case WM_CREATE:
				pClass->Log("%-32s WM_CREATE", __FUNCTION__);				
	            return 0;

            case WM_NCDESTROY:
				pClass->Log("%-32s WM_NCDESTROY", __FUNCTION__);
				pClass->OnDestroy(hWnd);
	            return 0;
            case WM_SIZE:
            {
                
            }
            break;
            case WM_ERASEBKGND:
                return TRUE;
            case WM_PAINT:
            {
                
            }
            return 0;
            case WM_PRINTCLIENT:
            {
            
            }
            return 0;

            case WM_NCPAINT:
            {

            }
            break;
            case WM_GETFONT:
                break;

            case WM_SETFONT:
                break;
            case WM_STYLECHANGED:
                break;
            case WM_SETTINGCHANGE:
				break;
            case WM_THEMECHANGED:
				break;
	        case WM_GETDLGCODE:
		        break;
			case WM_SETFOCUS:
				return 0;
			case WM_KILLFOCUS:
				return 0;
			case WM_SETCURSOR:
				break;
			case WM_TIMER:
                return 0;
			case WM_MOUSEMOVE:
                break;
			case WM_MOUSELEAVE:
		        break;
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MBUTTONDBLCLK:
		        break;
		    case WM_CAPTURECHANGED:
			    break;
			case WM_KEYDOWN:
                break;
			case WM_CHAR:
                return 0;
			case WM_MOUSEWHEEL:
                break;
			case WM_MOUSEHWHEEL:
                break;
			case WM_CONTEXTMENU:
                return 0;
			case WM_VSCROLL:
                return 0;
			case WM_HSCROLL:
                return 0;
			case WM_NOTIFY:
                break;
			case WM_MEASUREITEM:
				break;
			case WM_DRAWITEM:
				break;
        }
		if (uMsg >= TNM_FIRST && uMsg <= TNM_LAST)
		{
			return pClass? pClass->PhTnpOnUserMessage(hWnd, NULL, uMsg, wParam, lParam) : 0;
		}

		switch (uMsg)
		{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		{
			/*
			if (context->TooltipsHandle)
			{
				MSG message;

				message.hwnd = hwnd;
				message.message = uMsg;
				message.wParam = wParam;
				message.lParam = lParam;
				SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
			}
			*/
		}
		break;
		}
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
	BOOLEAN PhTnpGetNodeChildren(
		_In_ PWINDOW_CONTEXT Context,
		_In_opt_ PPH_TREENEW_NODE Node,
		_Out_ PPH_TREENEW_NODE **Children,
		_Out_ PULONG NumberOfChildren
	)
	{
		PH_TREENEW_GET_CHILDREN getChildren;

		getChildren.Flags = 0;
		getChildren.Node = Node;
		getChildren.Children = NULL;
		getChildren.NumberOfChildren = 0;

		if (Context->pCallback(
			Context->hWnd,
			TreeNewGetChildren,
			&getChildren,
			NULL,
			Context->pCallbackContext
		))
		{
			*Children = getChildren.Children;
			*NumberOfChildren = getChildren.NumberOfChildren;

			return TRUE;
		}

		return FALSE;
	}
	VOID PhClearList(
		_Inout_ PPH_LIST List
	)
	{
		List->Count = 0;
	}
	BOOLEAN PhTnpIsNodeLeaf(
		_In_ PWINDOW_CONTEXT	Context,
		_In_ PPH_TREENEW_NODE	Node
	)
	{
		PH_TREENEW_IS_LEAF isLeaf;

		isLeaf.Flags = 0;
		isLeaf.Node = Node;
		isLeaf.IsLeaf = TRUE;

		if (Context->pCallback(
			Context->hWnd,
			TreeNewIsLeaf,
			&isLeaf,
			NULL,
			Context->pCallbackContext
		))
		{
			return isLeaf.IsLeaf;
		}

		// Doesn't matter, decide when we do the get-children callback.
		return FALSE;
	}
	BOOLEAN PhTnpGetNodeChildren(
		_In_		PWINDOW_CONTEXT Context,
		_In_opt_	PPH_TREENEW_NODE Node,
		_Out_		std::vector<NodePtr>	*Children
	)
	{
		PH_TREENEW_GET_CHILDREN getChildren;

		getChildren.Flags = 0;
		getChildren.Node = Node;
		getChildren.Children = NULL;
		getChildren.NumberOfChildren = 0;

		if (Context->pCallback(
			Context->hWnd,
			TreeNewGetChildren,
			Node, 
			Children,
			Context->pCallbackContext
		))
		{
			//*Children = getChildren.Children;
			//*NumberOfChildren = getChildren.NumberOfChildren;
			return TRUE;
		}
		return FALSE;
	}
	VOID PhTnpInsertNodeChildren(
		_In_ PWINDOW_CONTEXT	Context,
		_In_ NodePtr			Node,
		_In_ ULONG				Level
	)
	{
		std::vector<NodePtr>	children;
		ULONG i;
		ULONG nextLevel;

		if (Node->Visible)
		{
			Node->Level = Level;
			Node->Index = (ULONG)Context->FlatList.size();
			Context->FlatList.push_back(Node);

			if (Context->FocusNode == Node)
				Context->FocusNodeFound = TRUE;

			nextLevel = Level + 1;
		}
		else
		{
			nextLevel = 0; // children of this node should be level 0
		}

		if (!(Node->s.IsLeaf = PhTnpIsNodeLeaf(Context, Node.get())))
		{
			Context->CanAnyExpand = TRUE;

			if (Node->Expanded)
			{
				if (PhTnpGetNodeChildren(Context, Node.get(), &children))
				{
					for (i = 0; i < children.size() ; i++)
					{
						PhTnpInsertNodeChildren(Context, children[i], nextLevel);
					}
					if (children.size() == 0)
						Node->s.IsLeaf = TRUE;
				}
			}
		}
	}
	VOID PhTnpRestructureNodes(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		std::vector<NodePtr>	children;
		ULONG i;

		if (!PhTnpGetNodeChildren(Context, NULL, &children))
			return;

		// We try to preserve the hot node, the focused node and the selection mark node. At this point
		// all node pointers must be regarded as invalid, so we must not follow any pointers.

		Context->FocusNodeFound = FALSE;
		Context->FlatList.clear();
		Context->CanAnyExpand = FALSE;

		for (i = 0; i < children.size(); i++)
		{
			PhTnpInsertNodeChildren(Context, children[i], 0);
		}

		if (!Context->FocusNodeFound)
			Context->FocusNode = NULL; // focused node is no longer present

		if (Context->HotNodeIndex >= Context->FlatList.size()) // covers -1 case as well
			Context->HotNodeIndex = -1;

		if (Context->MarkNodeIndex >= Context->FlatList.size())
			Context->MarkNodeIndex = -1;
	}
	VOID PhTnpLayoutHeader(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		RECT rect;
		HDLAYOUT hdl;
		WINDOWPOS windowPos;

		hdl.prc = &rect;
		hdl.pwpos = &windowPos;

		if (!(Context->dwStyle & TN_STYLE_NO_COLUMN_HEADER))
		{
			// Fixed portion header control
			rect.left = 0;
			rect.top = 0;
			rect.right = Context->NormalLeft;
			rect.bottom = Context->ClientRect.bottom;
			Header_Layout(Context->window.hFixedHeader, &hdl);
			SetWindowPos(Context->window.hFixedHeader, NULL, windowPos.x, windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);
			Context->HeaderHeight = windowPos.cy;

			// Normal portion header control
			rect.left = Context->NormalLeft - Context->HScrollPosition;
			rect.top = 0;
			rect.right = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
			rect.bottom = Context->ClientRect.bottom;
			Header_Layout(Context->window.hHeader, &hdl);
			SetWindowPos(Context->window.hHeader, NULL, windowPos.x, windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);
		}
		else
		{
			Context->HeaderHeight = 0;
		}

		if (Context->window.hTooltip)
		{
			TOOLINFO toolInfo;

			memset(&toolInfo, 0, sizeof(TOOLINFO));
			toolInfo.cbSize = sizeof(TOOLINFO);
			toolInfo.hwnd = Context->window.hFixedHeader;
			toolInfo.uId = TNP_TOOLTIPS_FIXED_HEADER;
			GetClientRect(Context->window.hFixedHeader, &toolInfo.rect);
			SendMessage(Context->window.hTooltip, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);

			toolInfo.hwnd = Context->window.hHeader;
			toolInfo.uId = TNP_TOOLTIPS_HEADER;
			GetClientRect(Context->window.hHeader, &toolInfo.rect);
			SendMessage(Context->window.hTooltip, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
		}
	}
	VOID PhTnpProcessScroll(
		_In_ PWINDOW_CONTEXT Context,
		_In_ LONG DeltaRows,
		_In_ LONG DeltaX
	)
	{
		RECT rect;
		LONG deltaY;

		rect.top = Context->HeaderHeight;
		rect.bottom = Context->ClientRect.bottom;

		if (DeltaX == 0)
		{
			rect.left = 0;
			rect.right = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
			ScrollWindowEx(
				Context->hWnd,
				0,
				-DeltaRows * Context->RowHeight,
				&rect,
				NULL,
				NULL,
				NULL,
				SW_INVALIDATE
			);
		}
		else
		{
			// Don't scroll if there are no rows. This is especially important if the user wants us to
			// display empty text.
			if (Context->FlatList.size() != 0)
			{
				deltaY = DeltaRows * Context->RowHeight;

				// If we're scrolling vertically as well, we need to scroll the fixed part and the
				// normal part separately.

				if (DeltaRows != 0)
				{
					rect.left = 0;
					rect.right = Context->NormalLeft;
					ScrollWindowEx(
						Context->hWnd,
						0,
						-deltaY,
						&rect,
						&rect,
						NULL,
						NULL,
						SW_INVALIDATE
					);
				}

				rect.left = Context->NormalLeft;
				rect.right = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
				ScrollWindowEx(
					Context->hWnd,
					-DeltaX,
					-deltaY,
					&rect,
					&rect,
					NULL,
					NULL,
					SW_INVALIDATE
				);
			}

			PhTnpLayoutHeader(Context);
		}
	}
	VOID PhTnpUpdateScrollBars(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		RECT clientRect;
		LONG width;
		LONG height;
		LONG contentWidth;
		LONG contentHeight;
		SCROLLINFO scrollInfo;
		LONG oldPosition;
		LONG deltaRows;
		LONG deltaX;
		LOGICAL oldHScrollVisible;
		RECT rect;

		clientRect = Context->ClientRect;
		width = clientRect.right - Context->FixedWidth;
		height = clientRect.bottom - Context->HeaderHeight;

		contentWidth = Context->TotalViewX;
		contentHeight = (LONG)Context->FlatList.size() * Context->RowHeight;

		if (contentHeight > height)
		{
			// We need a vertical scrollbar, so we can't use that area of the screen for content.
			width -= Context->VScrollWidth;
		}

		if (contentWidth > width)
		{
			height -= Context->HScrollHeight;
		}

		deltaRows = 0;
		deltaX = 0;

		// Vertical scroll bar

		scrollInfo.cbSize = sizeof(SCROLLINFO);
		scrollInfo.fMask = SIF_POS;
		GetScrollInfo(Context->window.hVScroll, SB_CTL, &scrollInfo);
		oldPosition = scrollInfo.nPos;

		scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
		scrollInfo.nMin = 0;
		scrollInfo.nMax = Context->FlatList.size() != 0 ? (ULONG)Context->FlatList.size() - 1 : 0;
		scrollInfo.nPage = height / Context->RowHeight;
		SetScrollInfo(Context->window.hVScroll, SB_CTL, &scrollInfo, TRUE);

		// The scroll position may have changed due to the modified scroll range.
		scrollInfo.fMask = SIF_POS;
		GetScrollInfo(Context->window.hVScroll, SB_CTL, &scrollInfo);
		deltaRows = scrollInfo.nPos - oldPosition;
		Context->VScrollPosition = scrollInfo.nPos;

		if (contentHeight > height && contentHeight != 0)
		{
			ShowWindow(Context->window.hVScroll, SW_SHOW);
			Context->VScrollVisible = TRUE;
		}
		else
		{
			ShowWindow(Context->window.hVScroll, SW_HIDE);
			Context->VScrollVisible = FALSE;
		}

		// Horizontal scroll bar

		scrollInfo.cbSize = sizeof(SCROLLINFO);
		scrollInfo.fMask = SIF_POS;
		GetScrollInfo(Context->window.hHScroll, SB_CTL, &scrollInfo);
		oldPosition = scrollInfo.nPos;

		scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
		scrollInfo.nMin = 0;
		scrollInfo.nMax = contentWidth != 0 ? contentWidth - 1 : 0;
		scrollInfo.nPage = width;
		SetScrollInfo(Context->window.hHScroll, SB_CTL, &scrollInfo, TRUE);

		scrollInfo.fMask = SIF_POS;
		GetScrollInfo(Context->window.hHScroll, SB_CTL, &scrollInfo);
		deltaX = scrollInfo.nPos - oldPosition;
		Context->HScrollPosition = scrollInfo.nPos;

		oldHScrollVisible = Context->HScrollVisible;

		if (contentWidth > width && contentWidth != 0)
		{
			ShowWindow(Context->window.hHScroll, SW_SHOW);
			Context->HScrollVisible = TRUE;
		}
		else
		{
			ShowWindow(Context->window.hHScroll, SW_HIDE);
			Context->HScrollVisible = FALSE;
		}

		if ((Context->HScrollVisible != oldHScrollVisible) && Context->FixedDividerVisible && Context->AnimateDivider)
		{
			rect.left = Context->FixedWidth;
			rect.top = Context->HeaderHeight;
			rect.right = Context->FixedWidth + 1;
			rect.bottom = Context->ClientRect.bottom;
			InvalidateRect(Context->hWnd, &rect, FALSE);
		}

		if (deltaRows != 0 || deltaX != 0)
			PhTnpProcessScroll(Context, deltaRows, deltaX);

		ShowWindow(Context->window.hFillerBox, (Context->VScrollVisible && Context->HScrollVisible) ? SW_SHOW : SW_HIDE);
	}
	VOID PhTnpLayout(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		RECT clientRect;

		if (Context->nEnableRedraw <= 0)
		{
			Context->SuspendUpdateLayout = TRUE;
			return;
		}

		clientRect = Context->ClientRect;

		PhTnpUpdateScrollBars(Context);

		// Vertical scroll bar
		if (Context->VScrollVisible)
		{
			MoveWindow(
				Context->window.hVScroll,
				clientRect.right - Context->VScrollWidth,
				0,
				Context->VScrollWidth,
				clientRect.bottom - (Context->HScrollVisible ? Context->HScrollHeight : 0),
				TRUE
			);
		}

		// Horizontal scroll bar
		if (Context->HScrollVisible)
		{
			MoveWindow(
				Context->window.hHScroll,
				Context->NormalLeft,
				clientRect.bottom - Context->HScrollHeight,
				clientRect.right - Context->NormalLeft - (Context->VScrollVisible ? Context->VScrollWidth : 0),
				Context->HScrollHeight,
				TRUE
			);
		}

		// Filler box
		if (Context->VScrollVisible && Context->HScrollVisible)
		{
			MoveWindow(
				Context->window.hFillerBox,
				clientRect.right - Context->VScrollWidth,
				clientRect.bottom - Context->HScrollHeight,
				Context->VScrollWidth,
				Context->HScrollHeight,
				TRUE
			);
		}

		PhTnpLayoutHeader(Context);

		// Redraw the entire window if we are displaying empty text.
		if (Context->FlatList.size() == 0 && Context->EmptyText.length() != 0)
			InvalidateRect(Context->hWnd, NULL, FALSE);
	}

	VOID PhTnpHitTest(
		_In_ PWINDOW_CONTEXT Context,
		_Inout_ PPH_TREENEW_HIT_TEST HitTest
	)
	{
		RECT clientRect;
		LONG x;
		LONG y;
		ULONG index;
		NodePtr node;

		HitTest->Flags = 0;
		HitTest->Node = NULL;
		HitTest->Column = NULL;

		clientRect = Context->ClientRect;
		x = HitTest->Point.x;
		y = HitTest->Point.y;

		if (x < 0)
			HitTest->Flags |= TN_HIT_LEFT;
		if (x >= clientRect.right)
			HitTest->Flags |= TN_HIT_RIGHT;
		if (y < 0)
			HitTest->Flags |= TN_HIT_ABOVE;
		if (y >= clientRect.bottom)
			HitTest->Flags |= TN_HIT_BELOW;

		if (HitTest->Flags == 0)
		{
			if (TNP_HIT_TEST_FIXED_DIVIDER(x, Context))
			{
				HitTest->Flags |= TN_HIT_DIVIDER;
			}

			if (y >= Context->HeaderHeight && x < Context->FixedWidth + Context->TotalViewX)
			{
				index = (y - Context->HeaderHeight) / Context->RowHeight + Context->VScrollPosition;

				if (index < Context->FlatList.size())
				{
					HitTest->Flags |= TN_HIT_ITEM;
					node = Context->FlatList[index];
					HitTest->Node = node.get();

					if (HitTest->InFlags & TN_TEST_COLUMN)
					{
						ColumnPtr	column;
						LONG columnX;

						column = NULL;

						if (x < Context->FixedWidth && Context->FixedColumnVisible)
						{
							column = Context->FixedColumn;
							columnX = 0;
						}
						else
						{
							LONG currentX;
							ULONG i;
							ColumnPtr currentColumn;

							currentX = Context->NormalLeft - Context->HScrollPosition;

							for (i = 0; i < Context->ColumnsByDisplay.size(); i++)
							{
								currentColumn = Context->ColumnsByDisplay[i];

								if (x >= currentX && x < currentX + currentColumn->Width)
								{
									column = currentColumn;
									columnX = currentX;
									break;
								}

								currentX += currentColumn->Width;
							}
						}

						HitTest->Column = column.get();

						if (column && (HitTest->InFlags & TN_TEST_SUBITEM))
						{
							BOOLEAN isFirstColumn;
							LONG currentX;

							isFirstColumn = HitTest->Column == Context->FirstColumn.get();

							currentX = columnX;
							currentX += TNP_CELL_LEFT_MARGIN;

							if (isFirstColumn)
							{
								currentX += (LONG)node->Level * m_nSmallIconWidth;

								if (!node->s.IsLeaf)
								{
									if (x >= currentX && x < currentX + m_nSmallIconWidth)
										HitTest->Flags |= TN_HIT_ITEM_PLUSMINUS;

									currentX += m_nSmallIconWidth;
								}

								if (node->Icon)
								{
									if (x >= currentX && x < currentX + m_nSmallIconWidth)
										HitTest->Flags |= TN_HIT_ITEM_ICON;

									currentX += m_nSmallIconWidth + TNP_ICON_RIGHT_PADDING;
								}
							}

							if (x >= currentX)
							{
								HitTest->Flags |= TN_HIT_ITEM_CONTENT;
							}
						}
					}
				}
			}
		}
	}
	BOOLEAN PhTnpGetRowRects(
		_In_ PWINDOW_CONTEXT Context,
		_In_ ULONG Start,
		_In_ ULONG End,
		_In_ BOOLEAN Clip,
		_Out_ PRECT Rect
	)
	{
		LONG startY;
		LONG endY;
		LONG viewWidth;

		if (End >= Context->FlatList.size())
			return FALSE;
		if (Start > End)
			return FALSE;

		startY = Context->HeaderHeight + ((LONG)Start - Context->VScrollPosition) * Context->RowHeight;
		endY = Context->HeaderHeight + ((LONG)End - Context->VScrollPosition) * Context->RowHeight;

		Rect->left = 0;
		Rect->right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
		Rect->top = startY;
		Rect->bottom = endY + Context->RowHeight;

		viewWidth = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);

		if (Rect->right > viewWidth)
			Rect->right = viewWidth;

		if (Clip)
		{
			if (Rect->top < Context->HeaderHeight)
				Rect->top = Context->HeaderHeight;
			if (Rect->bottom > Context->ClientRect.bottom)
				Rect->bottom = Context->ClientRect.bottom;
		}

		return TRUE;
	}
	VOID PhTnpSetHotNode(
		_In_ PWINDOW_CONTEXT Context,
		_In_opt_ PPH_TREENEW_NODE NewHotNode,
		_In_ BOOLEAN NewPlusMinusHot
	)
	{
		ULONG newHotNodeIndex;
		RECT rowRect;
		BOOLEAN needsInvalidate;

		if (NewHotNode)
			newHotNodeIndex = NewHotNode->Index;
		else
			newHotNodeIndex = -1;

		needsInvalidate = FALSE;

		if (Context->HotNodeIndex != newHotNodeIndex)
		{
			if (Context->HotNodeIndex != -1)
			{
				if (Context->ThemeData && PhTnpGetRowRects(Context, Context->HotNodeIndex, Context->HotNodeIndex, TRUE, &rowRect))
				{
					// Update the old hot node because it may have a different non-hot background and
					// plus minus part.
					InvalidateRect(Context->hWnd, &rowRect, FALSE);
				}
			}

			Context->HotNodeIndex = newHotNodeIndex;

			if (NewHotNode)
			{
				needsInvalidate = TRUE;
			}
		}

		if (NewHotNode)
		{
			if (NewHotNode->s.PlusMinusHot != NewPlusMinusHot)
			{
				NewHotNode->s.PlusMinusHot = NewPlusMinusHot;
				needsInvalidate = TRUE;
			}

			if (needsInvalidate && Context->ThemeData && PhTnpGetRowRects(Context, newHotNodeIndex, newHotNodeIndex, TRUE, &rowRect))
			{
				InvalidateRect(Context->hWnd, &rowRect, FALSE);
			}
		}
	}
	VOID PhTnpProcessMoveMouse(
		_In_ PWINDOW_CONTEXT Context,
		_In_ LONG CursorX,
		_In_ LONG CursorY
	)
	{
		PH_TREENEW_HIT_TEST hitTest;
		PPH_TREENEW_NODE hotNode;

		hitTest.Point.x = CursorX;
		hitTest.Point.y = CursorY;
		hitTest.InFlags = TN_TEST_COLUMN | TN_TEST_SUBITEM;
		PhTnpHitTest(Context, &hitTest);

		if (hitTest.Flags & TN_HIT_ITEM)
			hotNode = hitTest.Node;
		else
			hotNode = NULL;

		PhTnpSetHotNode(Context, hotNode, !!(hitTest.Flags & TN_HIT_ITEM_PLUSMINUS));

		if (Context->AnimateDivider && Context->FixedDividerVisible)
		{
			if (hitTest.Flags & TN_HIT_DIVIDER)
			{
				if ((Context->DividerHot < 100 || Context->AnimateDividerFadingOut) && !Context->AnimateDividerFadingIn)
				{
					// Begin fading in the divider.
					Context->AnimateDividerFadingIn = TRUE;
					Context->AnimateDividerFadingOut = FALSE;
					SetTimer(Context->hWnd, TNP_TIMER_ANIMATE_DIVIDER, TNP_ANIMATE_DIVIDER_INTERVAL, NULL);
				}
			}
			else
			{
				if ((Context->DividerHot != 0 || Context->AnimateDividerFadingIn) && !Context->AnimateDividerFadingOut)
				{
					Context->AnimateDividerFadingOut = TRUE;
					Context->AnimateDividerFadingIn = FALSE;
					SetTimer(Context->hWnd, TNP_TIMER_ANIMATE_DIVIDER, TNP_ANIMATE_DIVIDER_INTERVAL, NULL);
				}
			}
		}

		if (Context->window.hTooltip)
		{
			ULONG index;
			ULONG id;

			if (!(hitTest.Flags & TN_HIT_DIVIDER))
			{
				index = hitTest.Node ? hitTest.Node->Index : -1;
				id = hitTest.Column ? hitTest.Column->Id : -1;
			}
			else
			{
				index = -1;
				id = -1;
			}

			// This pops unnecessarily - when the cell has no tooltip text, and the user is moving the
			// mouse over it. However these unnecessary calls seem to fix a certain tooltip bug (move
			// the mouse around very quickly over the last column and the blank space to the right, and
			// no more tooltips will appear).
			if (Context->TooltipIndex != index || Context->TooltipId != id)
			{
				PhTnpPopTooltip(Context);
			}
		}
	}
	VOID PhTnpPrepareTooltipPop(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		Context->TooltipIndex = -1;
		Context->TooltipId = -1;
		Context->TooltipColumnId = -1;
	}
	VOID PhTnpPopTooltip(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		if (Context->window.hTooltip)
		{
			SendMessage(Context->window.hTooltip, TTM_POP, 0, 0);
			PhTnpPrepareTooltipPop(Context);
		}
	}
	VOID PhTnpGetMessagePos(
		_In_ HWND hwnd,
		_Out_ PPOINT ClientPoint
	)
	{
		ULONG position;
		POINT point;

		position = GetMessagePos();
		point.x = GET_X_LPARAM(position);
		point.y = GET_Y_LPARAM(position);
		ScreenToClient(hwnd, &point);

		*ClientPoint = point;
	}

	VOID PhTnpSetRedraw(
		_In_ PWINDOW_CONTEXT Context,
		_In_ BOOLEAN Redraw
	)
	{
		if (Redraw)
			Context->nEnableRedraw++;
		else
			Context->nEnableRedraw--;

		if (Context->nEnableRedraw == 1)
		{
			if (Context->SuspendUpdateStructure)
			{
				PhTnpRestructureNodes(Context);
			}

			if (Context->SuspendUpdateLayout)
			{
				PhTnpLayout(Context);
			}

			if (Context->SuspendUpdateMoveMouse)
			{
				POINT point;

				PhTnpGetMessagePos(Context->hWnd, &point);
				PhTnpProcessMoveMouse(Context, point.x, point.y);
			}

			Context->SuspendUpdateStructure = FALSE;
			Context->SuspendUpdateLayout = FALSE;
			Context->SuspendUpdateMoveMouse = FALSE;

			if (Context->SuspendUpdateRegion)
			{
				InvalidateRgn(Context->hWnd, Context->SuspendUpdateRegion, FALSE);
				DeleteRgn(Context->SuspendUpdateRegion);
				Context->SuspendUpdateRegion = NULL;
			}
		}
	}
	FORCEINLINE ULONG PhMultiplyDivide(
		_In_ ULONG Number,
		_In_ ULONG Numerator,
		_In_ ULONG Denominator
	)
	{
		return (ULONG)(((ULONG64)Number * (ULONG64)Numerator + Denominator / 2) / (ULONG64)Denominator);
	}
	LONG PhTnpInsertColumnHeader(
		_In_ PWINDOW_CONTEXT	Context,
		_In_ PPH_TREENEW_COLUMN Column
	)
	{
		HDITEM item;

		if (Column->Fixed)
		{
			if (Column->Width < Context->FixedWidthMinimum)
				Column->Width = Context->FixedWidthMinimum;

			Context->FixedWidth = Column->Width;
			Context->NormalLeft = Context->FixedWidth + 1;
			Context->FixedColumnVisible = TRUE;

			if (!(Context->dwStyle & TN_STYLE_NO_DIVIDER))
				Context->FixedDividerVisible = TRUE;
		}

		memset(&item, 0, sizeof(HDITEM));
		item.mask = HDI_WIDTH | HDI_TEXT | HDI_FORMAT | HDI_LPARAM | HDI_ORDER;
		item.cxy = Column->Width;
		item.pszText = Column->Text;
		item.fmt = 0;
		item.lParam = (LPARAM)Column;

		if (Column->Fixed)
			item.cxy++;

		if (Column->Fixed)
			item.iOrder = 0;
		else
			item.iOrder = Column->DisplayIndex;

		if (Column->Alignment & PH_ALIGN_LEFT)
			item.fmt |= HDF_LEFT;
		else if (Column->Alignment & PH_ALIGN_RIGHT)
			item.fmt |= HDF_RIGHT;
		else
			item.fmt |= HDF_CENTER;

		if (Column->Id == Context->SortColumn)
		{
			if (Context->SortOrder == AscendingSortOrder)
				item.fmt |= HDF_SORTUP;
			else if (Context->SortOrder == DescendingSortOrder)
				item.fmt |= HDF_SORTDOWN;
		}

		Column->Visible = TRUE;

		if (Column->Fixed)
			return Header_InsertItem(Context->window.hFixedHeader, 0, &item);
		else
			return Header_InsertItem(Context->window.hHeader, MAXINT, &item);
	}
	VOID PhTnpUpdateColumnHeaders(
		_In_ PWINDOW_CONTEXT	Context
	)
	{
		ULONG count;
		ULONG i;
		HDITEM item;
		PPH_TREENEW_COLUMN column;

		item.mask = HDI_WIDTH | HDI_LPARAM | HDI_ORDER;

		// Fixed column

		if (Context->FixedColumnVisible && Header_GetItem(Context->window.hFixedHeader, 0, &item))
		{
			column = Context->FixedColumn.get();
			column->Width = item.cxy - 1;
		}

		// Normal columns

		count = Header_GetItemCount(Context->window.hHeader);

		if (count != -1)
		{
			for (i = 0; i < count; i++)
			{
				if (Header_GetItem(Context->window.hHeader, i, &item))
				{
					column = (PPH_TREENEW_COLUMN)item.lParam;
					column->s.ViewIndex = i;
					column->Width = item.cxy;
					column->DisplayIndex = item.iOrder;
				}
			}
		}
	}

	void	PrintColumn(PPH_TREENEW_COLUMN p) {
		Log("%-32s %d", "Visible",			p->Visible);
		Log("%-32s %d", "CustomDraw",		p->CustomDraw);
		Log("%-32s %d", "Fixed",			p->Fixed);
		Log("%-32s %d", "SortDescending",	p->SortDescending);
		Log("%-32s %d", "DpiScaleOnAdd",	p->DpiScaleOnAdd);
		Log("%-32s %d", "SpareFlags",		p->SpareFlags);

		Log("%-32s %d", "Id",		p->Id);
		Log("%-32s %p", "Context",	p->Context);
		Log("%-32s %ws", "Text", p->Text);
		Log("%-32s %d",	 "Width", p->Width);
		Log("%-32s %d", "Alignment", p->Alignment);
		Log("%-32s %d", "DisplayIndex", p->SpareFlags);
		Log("%-32s %d", "TextFlags", p->TextFlags);
		Log("%-32s %d", "s.ViewIndex", p->s.ViewIndex);
		Log("%-32s %d", "s.ViewX", p->s.ViewX);

	
	}
	BOOLEAN PhTnpAddColumn(
		_In_ PWINDOW_CONTEXT	Context,
		_In_ PPH_TREENEW_COLUMN Column
	)
	{
		ColumnPtr	ptr	= std::make_shared<PH_TREENEW_COLUMN>();
		CopyMemory(ptr.get(), Column, sizeof(PH_TREENEW_COLUMN));
		//PrintColumn(ptr.get());

		if( nullptr == ptr )	return false;

		// Check if a column with the same ID already exists.
		if (Column->Id < Context->Columns.size())
			return FALSE;

		if (Context->NextId < Column->Id + 1)
			Context->NextId = Column->Id + 1;

		if (ptr->DpiScaleOnAdd)
		{
			ptr->Width = PhMultiplyDivide(ptr->Width, m_nGlobalDpi, 96);
			ptr->DpiScaleOnAdd = FALSE;
		}
		PhTnpExpandAllocatedColumns(Context);
		Context->Columns[Column->Id] = ptr;
		//Context->Columns.push_back(ptr);
		//Log("%-32s Columns:%d/%d", __FUNCTION__, Context->Columns.size(), Context->Columns.capacity());

		if (ptr->Fixed)
		{
			if (Context->FixedColumn)
			{
				// We already have a fixed column, and we can't have two. Make this new column un-fixed.
				ptr->Fixed = FALSE;
			}
			else
			{
				Context->FixedColumn = ptr;
			}
			ptr->DisplayIndex = 0;
			ptr->s.ViewX = 0;
		}
		if (ptr->Visible)
		{
			BOOLEAN updateHeaders;

			updateHeaders = FALSE;

			Log("%-32s 1", __func__);
			if (!ptr->Fixed && ptr->DisplayIndex != Header_GetItemCount(Context->window.hHeader))
				updateHeaders = TRUE;
			Log("%-32s 2", __func__);
			ptr->s.ViewIndex = PhTnpInsertColumnHeader(Context, ptr.get());

			if (updateHeaders)
				PhTnpUpdateColumnHeaders(Context);
		}
		else
		{
			ptr->s.ViewIndex = -1;
		}
		PhTnpUpdateColumnMaps(Context);
		if (ptr->Visible)
			PhTnpLayout(Context);
		return TRUE;
	}
	VOID PhTnpUpdateColumnMaps(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		ULONG i;
		LONG x;

		if (Context->ColumnsByDisplay.size() < Context->Columns.size() )
		{
			Context->ColumnsByDisplay.clear();
		}

		for (i = 0; i < Context->Columns.size(); i++)
		{
			if (!Context->Columns[i])
				continue;

			if (Context->Columns[i]->Visible && !Context->Columns[i]->Fixed && Context->Columns[i]->DisplayIndex != -1)
			{
				if (Context->Columns[i]->DisplayIndex >= Context->Columns.size())
					RtlRaiseStatus(STATUS_INTERNAL_ERROR);

				Context->ColumnsByDisplay[Context->Columns[i]->DisplayIndex] = Context->Columns[i];
			}
		}

		x = 0;

		for (i = 0; i < Context->ColumnsByDisplay.size(); i++)
		{
			if (!Context->ColumnsByDisplay[i])
				break;

			Context->ColumnsByDisplay[i]->s.ViewX = x;
			x += Context->ColumnsByDisplay[i]->Width;
		}

		//Context->NumberOfColumnsByDisplay = i;
		Context->TotalViewX = x;

		if (Context->FixedColumnVisible)
			Context->FirstColumn = Context->FixedColumn;
		else if (Context->ColumnsByDisplay.size() != 0)
			Context->FirstColumn = Context->ColumnsByDisplay[0];
		else
			Context->FirstColumn = nullptr;

		if (Context->ColumnsByDisplay.size() != 0)
			Context->LastColumn = Context->ColumnsByDisplay[Context->ColumnsByDisplay.size() - 1];
		else if (Context->FixedColumnVisible)
			Context->LastColumn = Context->FixedColumn;
		else
			Context->LastColumn = NULL;
	}
	void	PhTnpExpandAllocatedColumns(
		PWINDOW_CONTEXT	Context ) {
		
		if( Context->Columns.capacity() ) {
			Context->AllocatedColumns	*= 2;
			if( Context->AllocatedColumns < Context->NextId )
				Context->AllocatedColumns	= Context->NextId;		
		}
		else {
			Context->AllocatedColumns	= 16;
			if( Context->AllocatedColumns < Context->NextId ) 
				Context->AllocatedColumns	= Context->NextId;
		}
		Context->Columns.reserve(Context->AllocatedColumns);
		
	}
	ULONG_PTR PhTnpOnUserMessage(
		_In_ HWND hWnd,
		_In_ PWND_CREATE_PARAMS Context,
		_In_ ULONG Message,
		_In_ ULONG_PTR WParam,
		_In_ ULONG_PTR LParam
	)
	{
		//__log_function__;

		WindowContextPtr	ptr	= GetWindowContext(hWnd);
		if( nullptr == ptr )	return 0;
		switch( Message ) {
			case TNM_ADDCOLUMN:
				Log("%-32s TNM_ADDCOLUMN", __func__);
				return PhTnpAddColumn(ptr.get(), (PPH_TREENEW_COLUMN)LParam);

			case TNM_SETREDRAW:
				PhTnpSetRedraw(ptr.get(), !!WParam);
				return (LRESULT)ptr->nEnableRedraw;

			case TNM_SETMAXID:
			{
				Log("%-32s TNM_SETMAXID", __func__);
				ULONG maxId = (ULONG)WParam;

				if (ptr->NextId < maxId + 1)
				{
					ptr->NextId = maxId + 1;
					if( ptr->AllocatedColumns < ptr->NextId )
						PhTnpExpandAllocatedColumns(ptr.get());
				}
			}
			return TRUE;
			case TNM_SETIMAGELIST:
			{
				Log("%-32s TNM_SETIMAGELIST", __func__);
				ptr->bImageList	= !!WParam;
				ptr->hImageList	= (HIMAGELIST)WParam;
			}
			return TRUE;

			case TNM_SETCALLBACK:
			{
				Log("%-32s TNM_SETCALLBACK", __func__);
				ptr->pCallback = (PPH_TREENEW_CALLBACK)LParam;
				ptr->pCallbackContext = (PVOID)WParam;

				if (!ptr->pCallback)
					ptr->pCallback = NullCallback;
			}
			return TRUE;
		return TRUE;
			case TNM_GETTOOLTIPS:
				Log("%-32s TNM_GETTOOLTIPS", __func__);
				return (LRESULT)ptr->window.hTooltip;

			case TNM_SETEXTENDEDFLAGS:
				Log("%-32s TNM_SETEXTENDEDFLAGS", __func__);
				ptr->dwExtendedFlags = (ptr->dwExtendedFlags & ~(ULONG)WParam) | ((ULONG)LParam & (ULONG)WParam);
				return TRUE;
			default:
				Log("%-32s %d", __func__, Message- WM_USER);
				break;
		}
		return 0;
	}
	bool	OnCreate(HWND hWnd, CREATESTRUCT * p) {
		__log_function__;
		WindowContextPtr	ptr	= AllocateWindowContext(hWnd, [&](PWINDOW_CONTEXT pContext) {
			Log("%p created.", pContext);
		});	
		if( !ptr )	return false;

		PWINDOW_CONTEXT	Context	= ptr.get();
		DWORD		dwHeaderStyle;

		PWND_CREATE_PARAMS	pp	= (PWND_CREATE_PARAMS)p->lpCreateParams;

		ptr->hWnd			= hWnd;				//	Handle
		ptr->hInstance		= p->hInstance;
		ptr->dwStyle		= p->style;
		ptr->dwExStyle		= p->dwExStyle;
		ptr->pClass			= pp->pClass;

		if( ptr->dwStyle & TN_STYLE_DOUBLE_BUFFERED )
			ptr->bDoubleBuffered	= true;
		if( (ptr->dwStyle & TN_STYLE_ANIMATE_DIVIDER) && ptr->bDoubleBuffered)
			ptr->bAnimateDivider = TRUE;

		dwHeaderStyle = HDS_HORZ | HDS_FULLDRAG;

		if (!(ptr->dwStyle & TN_STYLE_NO_COLUMN_SORT))
			dwHeaderStyle |= HDS_BUTTONS;
		if (!(ptr->dwStyle & TN_STYLE_NO_COLUMN_HEADER))
			dwHeaderStyle |= WS_VISIBLE;

		if (ptr->dwStyle & TN_STYLE_CUSTOM_COLORS)
		{
			ptr->color.text			= pp? pp->color.text		: RGB(0xff, 0xff, 0xff);
			ptr->color.focus		= pp? pp->color.focus		: RGB(0x0, 0x0, 0xff);
			ptr->color.selection	= pp? pp->color.selection	: RGB(0x0, 0x0, 0x80);
		}
		else
		{
			ptr->color.focus		= GetSysColor(COLOR_HOTLIGHT);
			ptr->color.selection	= GetSysColor(COLOR_HIGHLIGHT);
		}
		if (!(Context->window.hFixedHeader = CreateWindow(
			WC_HEADER,
			NULL,
			WS_CHILD | WS_CLIPSIBLINGS | dwHeaderStyle,
			0,
			0,
			0,
			0,
			hWnd,
			NULL,
			p->hInstance,
			NULL
		)))
		{
			return false;
		}
		if (!(Context->dwStyle & TN_STYLE_NO_COLUMN_REORDER))
			dwHeaderStyle |= HDS_DRAGDROP;

		if (!(Context->window.hHeader	= CreateWindow(
			WC_HEADER,
			NULL,
			WS_CHILD | WS_CLIPSIBLINGS | dwHeaderStyle,
			0,
			0,
			0,
			0,
			hWnd,
			NULL,
			p->hInstance,
			NULL
		)))
		{
			return FALSE;
		}

		Log("%-32s %-20s %p", __func__, "hHeader", Context->window.hHeader);
		Log("%-32s %-20s %p", __func__, "hFixedHeader", Context->window.hFixedHeader);

		if (!(Context->window.hVScroll = CreateWindow(
			WC_SCROLLBAR,
			NULL,
			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_VERT,
			0,
			0,
			0,
			0,
			hWnd,
			NULL,
			p->hInstance,
			NULL
		)))
		{
			return false;
		}

		if (!(Context->window.hHScroll = CreateWindow(
			WC_SCROLLBAR,
			NULL,
			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_HORZ,
			0,
			0,
			0,
			0,
			hWnd,
			NULL,
			p->hInstance,
			NULL
		)))
		{
			return false;
		}

		if (!(Context->window.hFillerBox = CreateWindow(
			WC_STATIC,
			NULL,
			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
			0,
			0,
			0,
			0,
			hWnd,
			NULL,
			p->hInstance,
			NULL
		)))
		{
			return false;
		}
		PhTnpSetFont(Context, NULL, FALSE); // use default font
		PhTnpUpdateSystemMetrics(Context);
		PhTnpInitializeTooltips(ptr);

		return TRUE;
	}

	VOID PhTnpInitializeTooltips(
		WindowContextPtr	Context
	)
	{
		Log("%-32s", __func__);
		TOOLINFO toolInfo;

		Context->window.hTooltip = CreateWindowEx(
			WS_EX_TRANSPARENT, // solves double-click problem
			TOOLTIPS_CLASS,
			NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
			0,
			0,
			0,
			0,
			NULL,
			NULL,
			Context->hInstance,
			NULL
		);

		if (!Context->window.hTooltip)
			return;

		// Item tooltips
		memset(&toolInfo, 0, sizeof(TOOLINFO));
		toolInfo.cbSize = sizeof(TOOLINFO);
		toolInfo.uFlags = TTF_TRANSPARENT;
		toolInfo.hwnd = Context->hWnd;
		toolInfo.uId = TNP_TOOLTIPS_ITEM;
		toolInfo.lpszText = LPSTR_TEXTCALLBACK;
		toolInfo.lParam = TNP_TOOLTIPS_ITEM;
		SendMessage(Context->window.hTooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

		// Fixed column tooltips
		toolInfo.uFlags = 0;
		toolInfo.hwnd = Context->window.hTooltip;
		toolInfo.uId = TNP_TOOLTIPS_FIXED_HEADER;
		toolInfo.lpszText = LPSTR_TEXTCALLBACK;
		toolInfo.lParam = TNP_TOOLTIPS_FIXED_HEADER;
		SendMessage(Context->window.hTooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

		// Normal column tooltips
		toolInfo.uFlags = 0;
		toolInfo.hwnd = Context->window.hTooltip;
		toolInfo.uId = TNP_TOOLTIPS_HEADER;
		toolInfo.lpszText = LPSTR_TEXTCALLBACK;
		toolInfo.lParam = TNP_TOOLTIPS_HEADER;
		SendMessage(Context->window.hTooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

		// Hook the header control window procedures so we can forward mouse messages to the tooltip control.
		Context->HeaderWindowProc = (WNDPROC)GetWindowLongPtr(Context->window.hHeader, GWLP_WNDPROC);
		Context->FixedHeaderWindowProc = (WNDPROC)GetWindowLongPtr(Context->window.hFixedHeader, GWLP_WNDPROC);

		Log("%-32s HeaderWindowProc %p", __func__, Context->HeaderWindowProc);
		Log("%-32s FixedHeaderWindowProc %p", __func__, Context->FixedHeaderWindowProc);

		if( 0 == SetWindowLongPtr(Context->window.hHeader, 0, (LONG_PTR)Context.get()) ) {
			CErrorMessage	err(GetLastError());
			Log("%-32s SetWindowLongPtr() failed. error=%s(%d)", __FUNCTION__, (PCSTR)err, (DWORD)err);
		}
		SetWindowLongPtr(Context->window.hFixedHeader, 0, (LONG_PTR)Context.get());

		//Log("%-32s %-20s %p", __func__, "hHeader", GetWindowLongPtr(Context->window.hHeader, 0));
		//Log("%-32s %-20s %p", __func__, "hHeader", GetWindowLongPtr(Context->window.hHeader, 1));
		//Log("%-32s %-20s %p", __func__, "hFixedHeader", GetWindowLongPtr(Context->window.hFixedHeader, 0));
		//Log("%-32s %-20s %p", __func__, "hFixedHeader", GetWindowLongPtr(Context->window.hFixedHeader, 1));

		if( false == AddWindowContext(Context->window.hHeader, Context) ) {
			Log("%-32s AddWindowContext() failed.", __FUNCTION__);
		}
		if( false == AddWindowContext(Context->window.hFixedHeader, Context) ) {
			Log("%-32s AddWindowContext() failed.", __func__);
		}
		SetWindowLongPtr(Context->window.hFixedHeader, GWLP_WNDPROC, (LONG_PTR)PhTnpHeaderHookWndProc);
		SetWindowLongPtr(Context->window.hHeader, GWLP_WNDPROC, (LONG_PTR)PhTnpHeaderHookWndProc);

		SendMessage(Context->window.hTooltip, TTM_SETMAXTIPWIDTH, 0, MAXSHORT); // no limit
		SetWindowFont(Context->window.hTooltip, Context->Font, FALSE);
		Context->TooltipFont = Context->Font;
	}

	PPH_TREENEW_COLUMN PhTnpHitTestHeader(
		_In_ PWINDOW_CONTEXT Context,
		_In_ BOOLEAN Fixed,
		_In_ PPOINT Point,
		_Out_opt_ PRECT ItemRect
	)
	{
		PPH_TREENEW_COLUMN column;
		RECT itemRect;

		if (Fixed)
		{
			if (!Context->FixedColumnVisible)
				return NULL;

			column = Context->FixedColumn.get();

			if (!Header_GetItemRect(Context->window.hFixedHeader, 0, &itemRect))
				return NULL;
		}
		else
		{
			HDHITTESTINFO hitTestInfo;

			hitTestInfo.pt = *Point;
			hitTestInfo.flags = 0;
			hitTestInfo.iItem = -1;

			if (SendMessage(Context->window.hHeader, HDM_HITTEST, 0, (LPARAM)&hitTestInfo) != -1 && hitTestInfo.iItem != -1)
			{
				HDITEM item;

				item.mask = HDI_LPARAM;

				if (!Header_GetItem(Context->window.hHeader, hitTestInfo.iItem, &item))
					return NULL;

				column = (PPH_TREENEW_COLUMN)item.lParam;

				if (!Header_GetItemRect(Context->window.hHeader, hitTestInfo.iItem, &itemRect))
					return NULL;
			}
			else
			{
				return NULL;
			}
		}

		if (ItemRect)
			*ItemRect = itemRect;

		return column;
	}
	static	LRESULT CALLBACK PhTnpHeaderHookWndProc(
		_In_ HWND hwnd,
		_In_ UINT uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
	)
	{
		WindowContextPtr	ptr;
		WNDPROC				oldWndProc;
		PWINDOW_CONTEXT		context		= (PWINDOW_CONTEXT)GetWindowLongPtr(hwnd, 0);
		if( NULL == context ) {
			return	0;
		}
		CTab				*pClass		= (CTab *)context->pClass;
		ptr				= pClass->GetWindowContext(hwnd);
		if( nullptr == ptr ) {
			return	0;
		}

		pClass->Log("%-32s %08x %p %p", __func__, uMsg, wParam, lParam);
		//context = ptr.get();
		if (hwnd == context->window.hFixedHeader)
			oldWndProc = context->FixedHeaderWindowProc;
		else
			oldWndProc = context->HeaderWindowProc;

		pClass->Log("%-32s %08x", __func__, uMsg);

		switch (uMsg)
		{
			case WM_DESTROY:
			{
				SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
				pClass->RemoveWindowContext(hwnd);
			}
			break;
			case WM_MOUSEMOVE:
			{
				POINT point;
				PPH_TREENEW_COLUMN column;
				ULONG id;

				point.x = GET_X_LPARAM(lParam);
				point.y = GET_Y_LPARAM(lParam);
				column = pClass->PhTnpHitTestHeader(context, hwnd == context->window.hFixedHeader, &point, NULL);

				if (column)
					id = column->Id;
				else
					id = -1;

				if (context->TooltipColumnId != id)
				{
					pClass->PhTnpPopTooltip(context);
				}
			}
			break;
			case WM_NOTIFY:
			{
				NMHDR *header = (NMHDR *)lParam;

				switch (header->code)
				{
				case TTN_GETDISPINFO:
				{
					if (header->hwndFrom == context->window.hTooltip)
					{
						NMTTDISPINFO *info = (NMTTDISPINFO *)header;
						POINT point;

						pClass->PhTnpGetMessagePos(hwnd, &point);
						pClass->PhTnpGetHeaderTooltipText(context, info->lParam == TNP_TOOLTIPS_FIXED_HEADER, &point, &info->lpszText);
					}
				}
				break;
				case TTN_SHOW:
				{
					if (header->hwndFrom == context->window.hTooltip)
					{
						return pClass->PhTnpPrepareTooltipShow(context);
					}
				}
				break;
				case TTN_POP:
				{
					if (header->hwndFrom == context->window.hTooltip)
					{
						pClass->PhTnpPrepareTooltipPop(context);
					}
				}
				break;
				}
			}
			break;
		}

		switch (uMsg)
		{
			case WM_MOUSEMOVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			{
				if (context->window.hTooltip)
				{
					MSG message;

					message.hwnd = hwnd;
					message.message = uMsg;
					message.wParam = wParam;
					message.lParam = lParam;
					SendMessage(context->window.hTooltip, TTM_RELAYEVENT, 0, (LPARAM)&message);
				}
			}
			break;
		}
		return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
	}
	BOOLEAN PhTnpPrepareTooltipShow(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		RECT rect;

		if (Context->TooltipFont != Context->NewTooltipFont)
		{
			Context->TooltipFont = Context->NewTooltipFont;
			SetWindowFont(Context->window.hTooltip, Context->TooltipFont, FALSE);
		}

		if (!Context->TooltipUnfolding)
		{
			SetWindowPos(
				Context->window.hTooltip,
				HWND_TOPMOST,
				0,
				0,
				0,
				0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_HIDEWINDOW
			);

			return FALSE;
		}

		rect = Context->TooltipRect;
		SendMessage(Context->window.hTooltip, TTM_ADJUSTRECT, TRUE, (LPARAM)&rect);
		MapWindowPoints(Context->hWnd, NULL, (POINT *)&rect, 2);
		SetWindowPos(
			Context->window.hTooltip,
			HWND_TOPMOST,
			rect.left,
			rect.top,
			0,
			0,
			SWP_NOSIZE | SWP_NOACTIVATE | SWP_HIDEWINDOW
		);

		return TRUE;
	}
	VOID PhTnpGetHeaderTooltipText(
		_In_ PWINDOW_CONTEXT Context,
		_In_ BOOLEAN Fixed,
		_In_ PPOINT Point,
		_Out_ PWSTR *Text
	)
	{
		LOGICAL result;
		PPH_TREENEW_COLUMN column;
		RECT itemRect;
		PWSTR text;
		SIZE_T textCount;
		HDC hdc;
		SIZE textSize;

		column = PhTnpHitTestHeader(Context, Fixed, Point, &itemRect);

		if (!column)
			return;

		if (Context->TooltipColumnId != column->Id)
		{
			// Determine if the tooltip needs to be shown.

			text = column->Text;
			textCount = PhCountStringZ(text);

			if (!(hdc = GetDC(Context->hWnd)))
				return;

			SelectFont(hdc, Context->Font);

			result = GetTextExtentPoint32(hdc, text, (ULONG)textCount, &textSize);
			ReleaseDC(Context->hWnd, hdc);

			if (!result)
				return;

			if (textSize.cx + 6 + 6 <= itemRect.right - itemRect.left) // HACK: Magic values (same as our cell margins?)
				return;

			Context->TooltipColumnId = column->Id;
			Context->TooltipText	= text;
		}
		*Text = (PWSTR)Context->TooltipText.c_str();

		// Always use the default parameters for column header tooltips.
		Context->NewTooltipFont = Context->Font;
		Context->TooltipUnfolding = FALSE;
		SendMessage(Context->window.hTooltip, TTM_SETMAXTIPWIDTH, 0, TNP_TOOLTIPS_DEFAULT_MAXIMUM_WIDTH);
	}

	SIZE_T PhCountStringZ(
		_In_ PWSTR String
	)
	{
		static BOOLEAN PhpVectorLevel = PH_VECTOR_LEVEL_NONE;

#ifndef _ARM64_
		if (PhpVectorLevel >= PH_VECTOR_LEVEL_SSE2)
		{
			PWSTR p;
			ULONG unaligned;
			__m128i b;
			__m128i z;
			ULONG mask;
			ULONG index;

			p = (PWSTR)((ULONG_PTR)String & ~0xe); // String should be 2 byte aligned
			unaligned = PtrToUlong(String) & 0xf;
			z = _mm_setzero_si128();

			if (unaligned != 0)
			{
				b = _mm_load_si128((__m128i *)p);
				b = _mm_cmpeq_epi16(b, z);
				mask = _mm_movemask_epi8(b) >> unaligned;

				if (_BitScanForward(&index, mask))
					return index / sizeof(WCHAR);

				p += 16 / sizeof(WCHAR);
			}

			while (TRUE)
			{
				b = _mm_load_si128((__m128i *)p);
				b = _mm_cmpeq_epi16(b, z);
				mask = _mm_movemask_epi8(b);

				if (_BitScanForward(&index, mask))
					return (SIZE_T)(p - String) + index / sizeof(WCHAR);

				p += 16 / sizeof(WCHAR);
			}
		}
		else
#endif
		{
			return wcslen(String);
		}
	}
	VOID PhTnpSetFont(
		_In_ PWINDOW_CONTEXT	Context,
		_In_opt_ HFONT Font,
		_In_ BOOLEAN Redraw
	)
	{
		if (Context->FontOwned)
		{
			DeleteFont(Context->Font);
			Context->FontOwned = FALSE;
		}

		Context->Font = Font;

		if (!Context->Font)
		{
			LOGFONT logFont;

			if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
			{
				Context->Font = CreateFontIndirect(&logFont);
				Context->FontOwned = TRUE;
			}
		}

		SetWindowFont(Context->window.hFixedHeader, Context->Font, Redraw);
		SetWindowFont(Context->window.hHeader, Context->Font, Redraw);

		if (Context->window.hTooltip)
		{
			SetWindowFont(Context->window.hTooltip, Context->Font, FALSE);
			Context->TooltipFont = Context->Font;
		}

		PhTnpUpdateTextMetrics(Context);
	}
	VOID PhTnpUpdateSystemMetrics(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		Context->VScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
		Context->HScrollHeight = GetSystemMetrics(SM_CYHSCROLL);
		Context->SystemBorderX = GetSystemMetrics(SM_CXBORDER);
		Context->SystemBorderY = GetSystemMetrics(SM_CYBORDER);
		Context->SystemEdgeX = GetSystemMetrics(SM_CXEDGE);
		Context->SystemEdgeY = GetSystemMetrics(SM_CYEDGE);
		Context->SystemDragX = GetSystemMetrics(SM_CXDRAG);
		Context->SystemDragY = GetSystemMetrics(SM_CYDRAG);

		if (Context->SystemDragX < 2)
			Context->SystemDragX = 2;
		if (Context->SystemDragY < 2)
			Context->SystemDragY = 2;
	}
	VOID PhTnpUpdateTextMetrics(
		_In_ PWINDOW_CONTEXT Context
	)
	{
		HDC hdc;

		if (hdc = GetDC(Context->hWnd))
		{
			SelectFont(hdc, Context->Font);
			GetTextMetrics(hdc, &Context->TextMetrics);

			if (!Context->CustomRowHeight)
			{
				// Below we try to match the row height as calculated by the list view, even if it
				// involves magic numbers. On Vista and above there seems to be extra padding.

				Context->RowHeight = Context->TextMetrics.tmHeight;

				if (Context->dwStyle & TN_STYLE_ICONS)
				{
					if (Context->RowHeight < m_nSmallIconHeight)
						Context->RowHeight = m_nSmallIconHeight;
				}
				else
				{
					if (!(Context->dwStyle & TN_STYLE_THIN_ROWS))
						Context->RowHeight += 1; // HACK
				}

				Context->RowHeight += 1; // HACK

				if (!(Context->dwStyle & TN_STYLE_THIN_ROWS))
					Context->RowHeight += 2; // HACK
			}

			ReleaseDC(Context->hWnd, hdc);
		}
	}
	void	OnDestroy(HWND hWnd) {
		FreeWindowContext(hWnd, [&](PWINDOW_CONTEXT pContext) {
			Log("%p destroyed.", pContext);
		});
	}
};
class CControl
	:
	public	CTab
{
public:
	CControl() {
		
	}
	~CControl() {
	
	}

	void	InitControls(HINSTANCE hInstance, HWND hWnd)
	{
		Log(__FUNCTION__);

		bool	bTreeListCustomColors	= CSettings::GetInteger(L"TreeListCustomColorsEnable")? true : false;
		Setting::Color	color;

		if (bTreeListCustomColors)
		{
			color.text		= (COLORREF)CSettings::GetInteger(L"TreeListCustomColorText");
			color.focus		= (COLORREF)CSettings::GetInteger(L"TreeListCustomColorFocus");
			color.selection	= (COLORREF)CSettings::GetInteger(L"TreeListCustomColorSelection");
		}
		CTab::CreateTab(hInstance, hWnd, true, color);
	}
	void	DestroyControls()
	{
	
	}
	
private:


};
