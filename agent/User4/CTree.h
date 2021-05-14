#pragma once

#include <WinUser.h>
#include <windowsx.h>
#include <vssym32.h>

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)    // ntsubauth
#endif

class CTree
{
public:
	CTree() {
	    m_nSmallIconWidth   = 0;
        m_nSmallIconHeight  = 0;
	}
	~CTree() {
	
	}
	bool		Init(HINSTANCE hInstance)
	{
        WNDCLASSEX c = { sizeof(c) };

        c.style         = CS_DBLCLKS | CS_GLOBALCLASS;
        c.lpfnWndProc   = PhTnpWndProc;
        c.cbClsExtra    = 0;
        c.cbWndExtra    = sizeof(PVOID);
        c.hInstance     = hInstance;
        c.hIcon         = NULL;
        c.hCursor       = LoadCursor(NULL, IDC_ARROW);
        c.hbrBackground = NULL;
        c.lpszMenuName  = NULL;
        c.lpszClassName = PH_TREENEW_CLASSNAME;
        c.hIconSm = NULL;

        if (!RegisterClassEx(&c))
            return false;

        m_nSmallIconWidth   = GetSystemMetrics(SM_CXSMICON);
        m_nSmallIconHeight  = GetSystemMetrics(SM_CYSMICON);

        return TRUE;
	}
private:
    LONG    m_nSmallIconWidth;
    LONG    m_nSmallIconHeight;    

    static  VOID TnpCreateTreeNewContext(
        _Out_ PPH_TREENEW_CONTEXT *Context
    )
    {
        PPH_TREENEW_CONTEXT context;

        context = (PPH_TREENEW_CONTEXT)PhAllocateZero(sizeof(PH_TREENEW_CONTEXT));
        context->FixedWidthMinimum  = 20;
        context->RowHeight          = 1; // must never be 0
        context->HotNodeIndex       = -1;
        context->Callback           = NULL;
        context->FlatList           = PhCreateList(64);
        context->TooltipIndex       = -1;
        context->TooltipId          = -1;
        context->TooltipColumnId    = -1;
        context->EnableRedraw       = 1;
        context->DefaultBackColor   = GetSysColor(COLOR_WINDOW); // RGB(0xff, 0xff, 0xff)
        context->DefaultForeColor   = GetSysColor(COLOR_WINDOWTEXT); // RGB(0x00, 0x00, 0x00)

        *Context = context;
    }
    static  VOID PhTnpDestroyTreeNewContext(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        if (Context->Columns)
        {
            for (ULONG i = 0; i < Context->NextId; i++)
            {
                if (Context->Columns[i])
                    PhFree(Context->Columns[i]);
            }
            PhFree(Context->Columns);
        }
        if (Context->ColumnsByDisplay)
            PhFree(Context->ColumnsByDisplay);

        if (Context->FontOwned)
            DeleteFont(Context->Font);

        if (Context->ThemeData)
            CloseThemeData(Context->ThemeData);

        if (Context->SearchString)
            PhFree(Context->SearchString);

        //if (Context->TooltipText)
        //    PhDereferenceObject(Context->TooltipText);

        //if (Context->BufferedContext)
        //    PhTnpDestroyBufferedContext(Context);

        if (Context->SuspendUpdateRegion)
            DeleteRgn(Context->SuspendUpdateRegion);

        PhFree(Context);
    }
    static  VOID PhTnpSetFixedWidth(
        _In_ PPH_TREENEW_CONTEXT Context,
        _In_ ULONG FixedWidth
    )
    {
        HDITEM item;

        if (Context->FixedColumnVisible)
        {
            Context->FixedWidth = FixedWidth;

            if (Context->FixedWidth < Context->FixedWidthMinimum)
                Context->FixedWidth = Context->FixedWidthMinimum;

            Context->NormalLeft = Context->FixedWidth + 1;

            item.mask = HDI_WIDTH;
            item.cxy = Context->FixedWidth + 1;
            Header_SetItem(Context->FixedHeaderHandle, 0, &item);
        }
        else
        {
            Context->FixedWidth = 0;
            Context->NormalLeft = 0;
        }
    }
    static  VOID PhTnpCancelTrack(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        PhTnpSetFixedWidth(Context, Context->TrackOldFixedWidth);
        ReleaseCapture();
    }
    static  LRESULT CALLBACK PhTnpWndProc(
        _In_ HWND hwnd,
        _In_ UINT uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
    )
    {
        PPH_TREENEW_CONTEXT context;

        GWL_EXSTYLE;
        context = (PPH_TREENEW_CONTEXT)GetWindowLongPtr(hwnd, 0);

        if (uMsg == WM_CREATE)
        {
            TnpCreateTreeNewContext(&context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)context);
        }

        if (!context)
            return DefWindowProc(hwnd, uMsg, wParam, lParam);

        if (context->Tracking && (GetAsyncKeyState(VK_ESCAPE) & 0x1))
        {
            PhTnpCancelTrack(context);
        }

        // Note: if we have suspended restructuring, we *cannot* access any nodes, because all node
        // pointers are now invalid. Below, we disable all input.

        switch (uMsg)
        {
        case WM_CREATE:
        {
            if (!PhTnpOnCreate(hwnd, context, (CREATESTRUCT *)lParam))
                return -1;
        }
        return 0;
        case WM_NCDESTROY:
        {
            context->Callback(hwnd, TreeNewDestroying, NULL, NULL, context->CallbackContext);
            PhTnpDestroyTreeNewContext(context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
        }
        return 0;
        case WM_SIZE:
        {
            PhTnpOnSize(hwnd, context);
        }
        break;
        case WM_ERASEBKGND:
            return TRUE;
        case WM_PAINT:
        {
            PhTnpOnPaint(hwnd, context);
        }
        return 0;
        case WM_PRINTCLIENT:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnPrintClient(hwnd, context, (HDC)wParam, (ULONG)lParam);
        }
        return 0;
        case WM_NCPAINT:
        {
            if (PhTnpOnNcPaint(hwnd, context, (HRGN)wParam))
                return 0;
        }
        break;
        case WM_GETFONT:
            return (LRESULT)context->Font;
        case WM_SETFONT:
        {
            PhTnpOnSetFont(hwnd, context, (HFONT)wParam, LOWORD(lParam));
        }
        break;
        case WM_STYLECHANGED:
        {
            PhTnpOnStyleChanged(hwnd, context, (LONG)wParam, (STYLESTRUCT *)lParam);
        }
        break;
        case WM_SETTINGCHANGE:
        {
            PhTnpOnSettingChange(hwnd, context);
        }
        break;
        case WM_THEMECHANGED:
        {
            PhTnpOnThemeChanged(hwnd, context);
        }
        break;
        case WM_GETDLGCODE:
            return PhTnpOnGetDlgCode(hwnd, context, (ULONG)wParam, (PMSG)lParam);
        case WM_SETFOCUS:
        {
            context->HasFocus = TRUE;
            InvalidateRect(context->Handle, NULL, FALSE);
        }
        return 0;
        case WM_KILLFOCUS:
        {
            //if (!context->ContextMenuActive && !(context->Style & TN_STYLE_ALWAYS_SHOW_SELECTION))
            //    PhTnpSelectRange(context, -1, -1, TN_SELECT_RESET, NULL, NULL);

            context->HasFocus = FALSE;

            InvalidateRect(context->Handle, NULL, FALSE);
        }
        return 0;
        case WM_SETCURSOR:
        {
            if (PhTnpOnSetCursor(hwnd, context, (HWND)wParam))
                return TRUE;
        }
        break;
        case WM_TIMER:
        {
            PhTnpOnTimer(hwnd, context, (ULONG)wParam);
        }
        return 0;
        case WM_MOUSEMOVE:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnMouseMove(hwnd, context, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            else
                context->SuspendUpdateMoveMouse = TRUE;
        }
        break;
        case WM_MOUSELEAVE:
        {
            //if (!context->ContextMenuActive && !(context->Style & TN_STYLE_ALWAYS_SHOW_SELECTION))
            //{
            //    ULONG changedStart;
            //    ULONG changedEnd;
            //    RECT rect;
            //
            //    PhTnpSelectRange(context, -1, -1, TN_SELECT_RESET, &changedStart, &changedEnd);
            //
            //    if (PhTnpGetRowRects(context, changedStart, changedEnd, TRUE, &rect))
            //    {
            //        InvalidateRect(context->Handle, &rect, FALSE);
            //    }
            //}

            if (!context->SuspendUpdateStructure)
                PhTnpOnMouseLeave(hwnd, context);
        }
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
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnXxxButtonXxx(hwnd, context, uMsg, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
        case WM_CAPTURECHANGED:
        {
            PhTnpOnCaptureChanged(hwnd, context);
        }
        break;
        case WM_KEYDOWN:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnKeyDown(hwnd, context, (ULONG)wParam, (ULONG)lParam);
        }
        break;
        case WM_CHAR:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnChar(hwnd, context, (ULONG)wParam, (ULONG)lParam);
        }
        return 0;
        case WM_MOUSEWHEEL:
        {
            PhTnpOnMouseWheel(hwnd, context, (SHORT)HIWORD(wParam), LOWORD(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
        case WM_MOUSEHWHEEL:
        {
            PhTnpOnMouseHWheel(hwnd, context, (SHORT)HIWORD(wParam), LOWORD(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
        case WM_CONTEXTMENU:
        {
            if (!context->SuspendUpdateStructure)
                PhTnpOnContextMenu(hwnd, context, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        return 0;
        case WM_VSCROLL:
        {
            PhTnpOnVScroll(hwnd, context, LOWORD(wParam), HIWORD(wParam));
        }
        return 0;
        case WM_HSCROLL:
        {
            PhTnpOnHScroll(hwnd, context, LOWORD(wParam), HIWORD(wParam));
        }
        return 0;
        case WM_NOTIFY:
        {
            LRESULT result;

            if (PhTnpOnNotify(hwnd, context, (NMHDR *)lParam, &result))
                return result;
        }
        break;
        case WM_MEASUREITEM:
            if (PhThemeWindowMeasureItem(hwnd, (LPMEASUREITEMSTRUCT)lParam))
                return TRUE;
            break;
        case WM_DRAWITEM:
            if (PhThemeWindowDrawItem((LPDRAWITEMSTRUCT)lParam))
                return TRUE;
            break;
        }

        if (uMsg >= TNM_FIRST && uMsg <= TNM_LAST)
        {
            return PhTnpOnUserMessage(hwnd, context, uMsg, wParam, lParam);
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
            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = hwnd;
                message.message = uMsg;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }
        }
        break;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    static  VOID PhTnpDrawThemedBorder(
        _In_ PPH_TREENEW_CONTEXT Context,
        _In_ HDC hdc
    )
    {
        RECT windowRect;
        RECT clientRect;
        INT  sizingBorderWidth;
        LONG borderX;
        LONG borderY;

        GetWindowRect(Context->Handle, &windowRect);
        windowRect.right -= windowRect.left;
        windowRect.bottom -= windowRect.top;
        windowRect.left = 0;
        windowRect.top = 0;

        clientRect.left = windowRect.left + Context->SystemEdgeX;
        clientRect.top = windowRect.top + Context->SystemEdgeY;
        clientRect.right = windowRect.right - Context->SystemEdgeX;
        clientRect.bottom = windowRect.bottom - Context->SystemEdgeY;

        // Make sure we don't paint in the client area.
        ExcludeClipRect(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

        // Draw the themed border.
        DrawThemeBackground(Context->ThemeData, hdc, 0, 0, &windowRect, NULL);

        // Calculate the size of the border we just drew, and fill in the rest of the space if we didn't
        // fully paint the region.

        if (SUCCEEDED(GetThemeInt(Context->ThemeData, 0, 0, TMT_SIZINGBORDERWIDTH, 
            &sizingBorderWidth)))
        {
            borderX = sizingBorderWidth;
            borderY = sizingBorderWidth;
        }
        else
        {
            borderX = Context->SystemBorderX;
            borderY = Context->SystemBorderY;
        }

        if (borderX < Context->SystemEdgeX || borderY < Context->SystemEdgeY)
        {
            windowRect.left += Context->SystemEdgeX - borderX;
            windowRect.top += Context->SystemEdgeY - borderY;
            windowRect.right -= Context->SystemEdgeX - borderX;
            windowRect.bottom -= Context->SystemEdgeY - borderY;
            FillRect(hdc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
        }
    }
    static  BOOLEAN PhTnpOnNcPaint(
        _In_ HWND hwnd,
        _In_ PPH_TREENEW_CONTEXT Context,
        _In_opt_ HRGN UpdateRegion
    )
    {
        PhTnpInitializeThemeData(Context);

        // Themed border
        if ((Context->ExtendedStyle & WS_EX_CLIENTEDGE) && Context->ThemeData)
        {
            HDC hdc;
            ULONG flags;

            if (UpdateRegion == HRGN_FULL)
                UpdateRegion = NULL;

            // Note the use of undocumented flags below. GetDCEx doesn't work without these.

            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000;

            if (UpdateRegion)
                flags |= DCX_INTERSECTRGN | 0x40000;

            if (hdc = GetDCEx(hwnd, UpdateRegion, flags))
            {
                PhTnpDrawThemedBorder(Context, hdc);
                ReleaseDC(hwnd, hdc);
                return TRUE;
            }
        }

        return FALSE;
    }
    static  VOID PhTnpOnPrintClient(
        _In_ HWND hwnd,
        _In_ PPH_TREENEW_CONTEXT Context,
        _In_ HDC hdc,
        _In_ ULONG Flags
    )
    {
        PhTnpPaint(hwnd, Context, hdc, &Context->ClientRect);
    }
    static  VOID PhTnpUpdateThemeData(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        Context->DefaultBackColor = GetSysColor(COLOR_WINDOW);
        Context->DefaultForeColor = GetSysColor(COLOR_WINDOWTEXT);
        Context->ThemeActive = !!IsThemeActive();

        if (Context->ThemeData)
        {
            CloseThemeData(Context->ThemeData);
            Context->ThemeData = NULL;
        }

        Context->ThemeData = OpenThemeData(Context->Handle, VSCLASS_TREEVIEW);

        if (Context->ThemeData)
        {
            Context->ThemeHasItemBackground = !!IsThemePartDefined(Context->ThemeData, TVP_TREEITEM, 0);
            Context->ThemeHasGlyph = !!IsThemePartDefined(Context->ThemeData, TVP_GLYPH, 0);
            Context->ThemeHasHotGlyph = !!IsThemePartDefined(Context->ThemeData, TVP_HOTGLYPH, 0);
        }
        else
        {
            Context->ThemeHasItemBackground = FALSE;
            Context->ThemeHasGlyph = FALSE;
            Context->ThemeHasHotGlyph = FALSE;
        }
    }
    static  VOID PhTnpInitializeThemeData(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        if (!Context->ThemeInitialized)
        {
            PhTnpUpdateThemeData(Context);
            Context->ThemeInitialized = TRUE;
        }
    }
    static  VOID PhTnpPaint(
        _In_ HWND hwnd,
        _In_ PPH_TREENEW_CONTEXT Context,
        _In_ HDC hdc,
        _In_ PRECT PaintRect
    )
    {
        RECT viewRect;
        LONG vScrollPosition;
        LONG hScrollPosition;
        LONG firstRowToUpdate;
        LONG lastRowToUpdate;
        LONG i;
        LONG j;
        PPH_TREENEW_NODE node;
        PPH_TREENEW_COLUMN column;
        RECT rowRect;
        LONG x;
        BOOLEAN fixedUpdate;
        LONG normalUpdateLeftX;
        LONG normalUpdateRightX;
        LONG normalUpdateLeftIndex;
        LONG normalUpdateRightIndex;
        LONG normalTotalX;
        RECT cellRect;
        HRGN oldClipRegion;

        PhTnpInitializeThemeData(Context);

        viewRect = Context->ClientRect;

        if (Context->VScrollVisible)
            viewRect.right -= Context->VScrollWidth;

        vScrollPosition = Context->VScrollPosition;
        hScrollPosition = Context->HScrollPosition;

        // Calculate the indicies of the first and last rows that need painting. These indicies are
        // relative to the top of the view area.

        firstRowToUpdate = (PaintRect->top - Context->HeaderHeight) / Context->RowHeight;
        lastRowToUpdate = (PaintRect->bottom - 1 - Context->HeaderHeight) / Context->RowHeight; // minus one since bottom is exclusive

        if (firstRowToUpdate < 0)
            firstRowToUpdate = 0;

        rowRect.left = 0;
        rowRect.top = Context->HeaderHeight + firstRowToUpdate * Context->RowHeight;
        rowRect.right = Context->NormalLeft + Context->TotalViewX - Context->HScrollPosition;
        rowRect.bottom = rowRect.top + Context->RowHeight;

        // Change the indicies to absolute row indicies.

        firstRowToUpdate += vScrollPosition;
        lastRowToUpdate += vScrollPosition;

        if (lastRowToUpdate >= (LONG)Context->FlatList->Count)
            lastRowToUpdate = Context->FlatList->Count - 1; // becomes -1 when there are no items, handled correctly by loop below

                                                            // Determine whether the fixed column needs painting, and which normal columns need painting.

        fixedUpdate = FALSE;

        if (Context->FixedColumnVisible && PaintRect->left < Context->FixedWidth)
            fixedUpdate = TRUE;

        x = Context->NormalLeft - hScrollPosition;
        normalUpdateLeftX = viewRect.right;
        normalUpdateLeftIndex = 0;
        normalUpdateRightX = 0;
        normalUpdateRightIndex = -1;

        for (j = 0; j < (LONG)Context->NumberOfColumnsByDisplay; j++)
        {
            column = Context->ColumnsByDisplay[j];

            if (x + column->Width >= Context->NormalLeft && x + column->Width > PaintRect->left && x < PaintRect->right)
            {
                if (normalUpdateLeftX > x)
                {
                    normalUpdateLeftX = x;
                    normalUpdateLeftIndex = j;
                }

                if (normalUpdateRightX < x + column->Width)
                {
                    normalUpdateRightX = x + column->Width;
                    normalUpdateRightIndex = j;
                }
            }

            x += column->Width;
        }

        normalTotalX = x;

        if (normalUpdateRightIndex >= (LONG)Context->NumberOfColumnsByDisplay)
            normalUpdateRightIndex = Context->NumberOfColumnsByDisplay - 1;

        // Paint the rows.

        SelectFont(hdc, Context->Font);
        SetBkMode(hdc, TRANSPARENT);

        for (i = firstRowToUpdate; i <= lastRowToUpdate; i++)
        {
            node = Context->FlatList->Items[i];

            // Prepare the row for drawing.

            PhTnpPrepareRowForDraw(Context, hdc, node);

            if (Context->ThemeSupport)
            {
                HDC tempDc;
                BITMAPINFOHEADER header;
                HBITMAP bitmap;
                HBITMAP oldBitmap;
                PVOID bits;
                RECT tempRect;
                BLENDFUNCTION blendFunction;

                SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                SetDCBrushColor(hdc, RGB(30, 30, 30));
                FillRect(hdc, &rowRect, GetStockBrush(DC_BRUSH));

                if (tempDc = CreateCompatibleDC(hdc))
                {
                    memset(&header, 0, sizeof(BITMAPINFOHEADER));
                    header.biSize = sizeof(BITMAPINFOHEADER);
                    header.biWidth = 1;
                    header.biHeight = 1;
                    header.biPlanes = 1;
                    header.biBitCount = 24;
                    bitmap = CreateDIBSection(tempDc, (BITMAPINFO *)&header, DIB_RGB_COLORS, &bits, NULL, 0);

                    if (bitmap)
                    {
                        // Draw the outline of the selection rectangle.
                        //FrameRect(hdc, &rowRect, GetSysColorBrush(COLOR_HIGHLIGHT));

                        // Fill in the selection rectangle.
                        oldBitmap = SelectBitmap(tempDc, bitmap);
                        tempRect.left = 0;
                        tempRect.top = 0;
                        tempRect.right = 1;
                        tempRect.bottom = 1;

                        SetTextColor(tempDc, node->s.DrawForeColor);

                        if (node->s.DrawBackColor != 16777215)
                        {
                            SetDCBrushColor(tempDc, node->s.DrawBackColor);
                            FillRect(tempDc, &tempRect, GetStockBrush(DC_BRUSH));
                        }
                        else
                        {
                            SetDCBrushColor(tempDc, RGB(30, 30, 30));
                            FillRect(tempDc, &tempRect, GetStockBrush(DC_BRUSH));
                        }

                        blendFunction.BlendOp = AC_SRC_OVER;
                        blendFunction.BlendFlags = 0;
                        blendFunction.SourceConstantAlpha = 96;
                        blendFunction.AlphaFormat = 0;

                        GdiAlphaBlend(
                            hdc,
                            rowRect.left,
                            rowRect.top,
                            rowRect.right - rowRect.left,
                            rowRect.bottom - rowRect.top,
                            tempDc,
                            0,
                            0,
                            1,
                            1,
                            blendFunction
                        );

                        SelectBitmap(tempDc, oldBitmap);
                        DeleteBitmap(bitmap);
                    }

                    DeleteDC(tempDc);
                }
            }
            else
            {
                if (node->Selected && (Context->CustomColors || !Context->ThemeHasItemBackground))
                {
                    // Non-themed background

                    if (Context->HasFocus)
                    {
                        if (Context->CustomColors)
                        {
                            SetTextColor(hdc, Context->CustomTextColor);
                            SetDCBrushColor(hdc, Context->CustomFocusColor);
                            FillRect(hdc, &rowRect, GetStockBrush(DC_BRUSH));
                        }
                        else
                        {
                            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                            FillRect(hdc, &rowRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                        }
                    }
                    else
                    {
                        if (Context->CustomColors)
                        {
                            SetTextColor(hdc, Context->CustomTextColor);
                            SetDCBrushColor(hdc, Context->CustomSelectedColor);
                            FillRect(hdc, &rowRect, GetStockBrush(DC_BRUSH));
                        }
                        else
                        {
                            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
                            FillRect(hdc, &rowRect, GetSysColorBrush(COLOR_BTNFACE));
                        }
                    }
                }
                else
                {
                    SetTextColor(hdc, node->s.DrawForeColor);
                    SetDCBrushColor(hdc, node->s.DrawBackColor);
                    FillRect(hdc, &rowRect, GetStockBrush(DC_BRUSH));
                }
            }

            if (!Context->CustomColors && Context->ThemeHasItemBackground)
            {
                INT stateId;

                // Themed background

                if (node->Selected)
                {
                    if (i == Context->HotNodeIndex)
                        stateId = TREIS_HOTSELECTED;
                    else if (!Context->HasFocus)
                        stateId = TREIS_SELECTEDNOTFOCUS;
                    else
                        stateId = TREIS_SELECTED;
                }
                else
                {
                    if (i == Context->HotNodeIndex)
                        stateId = TREIS_HOT;
                    else
                        stateId = INT_MAX;
                }

                // Themed background

                if (stateId != INT_MAX)
                {
                    if (!Context->FixedColumnVisible)
                    {
                        rowRect.left = Context->NormalLeft - hScrollPosition;
                    }

                    DrawThemeBackground(
                        Context->ThemeData,
                        hdc,
                        TVP_TREEITEM,
                        stateId,
                        &rowRect,
                        PaintRect
                    );
                }
            }

            // Paint the fixed column.

            cellRect.top = rowRect.top;
            cellRect.bottom = rowRect.bottom;

            if (fixedUpdate)
            {
                cellRect.left = 0;
                cellRect.right = Context->FixedWidth;
                PhTnpDrawCell(Context, hdc, &cellRect, node, Context->FixedColumn, i, -1);
            }

            // Paint the normal columns.

            if (normalUpdateLeftX < normalUpdateRightX)
            {
                cellRect.left = normalUpdateLeftX;
                cellRect.right = cellRect.left;

                oldClipRegion = CreateRectRgn(0, 0, 0, 0);

                if (GetClipRgn(hdc, oldClipRegion) != 1)
                {
                    DeleteRgn(oldClipRegion);
                    oldClipRegion = NULL;
                }

                IntersectClipRect(hdc, Context->NormalLeft, cellRect.top, viewRect.right, cellRect.bottom);

                for (j = normalUpdateLeftIndex; j <= normalUpdateRightIndex; j++)
                {
                    column = Context->ColumnsByDisplay[j];

                    cellRect.left = cellRect.right;
                    cellRect.right = cellRect.left + column->Width;
                    PhTnpDrawCell(Context, hdc, &cellRect, node, column, i, j);
                }

                SelectClipRgn(hdc, oldClipRegion);

                if (oldClipRegion)
                {
                    DeleteRgn(oldClipRegion);
                }
            }

            rowRect.top += Context->RowHeight;
            rowRect.bottom += Context->RowHeight;
        }

        if (lastRowToUpdate == Context->FlatList->Count - 1) // works even if there are no items
        {
            // Fill the rest of the space on the bottom with the window color.
            rowRect.bottom = viewRect.bottom;

            if (Context->ThemeSupport)
            {
                SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                SetDCBrushColor(hdc, RGB(30, 30, 30));
                FillRect(hdc, &rowRect, GetStockBrush(DC_BRUSH));
            }
            else
            {
                FillRect(hdc, &rowRect, GetSysColorBrush(COLOR_WINDOW));
            }
        }

        if (normalTotalX < viewRect.right && viewRect.right > PaintRect->left && normalTotalX < PaintRect->right)
        {
            // Fill the rest of the space on the right with the window color.
            rowRect.left = normalTotalX;
            rowRect.top = Context->HeaderHeight;
            rowRect.right = viewRect.right;
            rowRect.bottom = viewRect.bottom;

            if (Context->ThemeSupport)
            {
                SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                SetDCBrushColor(hdc, RGB(30, 30, 30));
                FillRect(hdc, &rowRect, GetStockBrush(DC_BRUSH));
            }
            else
            {
                FillRect(hdc, &rowRect, GetSysColorBrush(COLOR_WINDOW));
            }
        }

        if (Context->FlatList->Count == 0 && Context->EmptyText.Length != 0)
        {
            RECT textRect;

            textRect.left = 20;
            textRect.top = Context->HeaderHeight + 10;
            textRect.right = viewRect.right - 20;
            textRect.bottom = viewRect.bottom - 5;

            if (PhGetIntegerSetting(L"EnableThemeSupport"))
            {
                SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
            }
            else
            {
                SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
            }

            DrawText(
                hdc,
                Context->EmptyText.Buffer,
                (ULONG)Context->EmptyText.Length / 2,
                &textRect,
                DT_NOPREFIX | DT_CENTER | DT_END_ELLIPSIS
            );
        }

        if (Context->FixedDividerVisible && Context->FixedWidth >= PaintRect->left && Context->FixedWidth < PaintRect->right)
        {
            PhTnpDrawDivider(Context, hdc);
        }

        if (Context->DragSelectionActive)
        {
            PhTnpDrawSelectionRectangle(Context, hdc, &Context->DragRect);
        }
    }
    static  VOID PhTnpOnPaint(
        _In_ HWND hwnd,
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        RECT updateRect;
        HDC hdc;
        PAINTSTRUCT paintStruct;

        if (GetUpdateRect(hwnd, &updateRect, FALSE) && (updateRect.left | updateRect.right | updateRect.top | updateRect.bottom))
        {
            if (Context->EnableRedraw <= 0)
            {
                HRGN updateRegion;

                updateRegion = CreateRectRgn(0, 0, 0, 0);
                GetUpdateRgn(hwnd, updateRegion, FALSE);

                if (!Context->SuspendUpdateRegion)
                {
                    Context->SuspendUpdateRegion = updateRegion;
                }
                else
                {
                    CombineRgn(Context->SuspendUpdateRegion, Context->SuspendUpdateRegion, updateRegion, RGN_OR);
                    DeleteRgn(updateRegion);
                }

                // Pretend we painted something; this ensures the update region is validated properly.
                if (BeginPaint(hwnd, &paintStruct))
                    EndPaint(hwnd, &paintStruct);

                return;
            }

            if (Context->DoubleBuffered)
            {
                if (!Context->BufferedContext)
                {
                    PhTnpCreateBufferedContext(Context);
                }
            }

            if (hdc = BeginPaint(hwnd, &paintStruct))
            {
                updateRect = paintStruct.rcPaint;

                if (Context->BufferedContext)
                {
                    PhTnpPaint(hwnd, Context, Context->BufferedContext, &updateRect);
                    BitBlt(
                        hdc,
                        updateRect.left,
                        updateRect.top,
                        updateRect.right - updateRect.left,
                        updateRect.bottom - updateRect.top,
                        Context->BufferedContext,
                        updateRect.left,
                        updateRect.top,
                        SRCCOPY
                    );
                }
                else
                {
                    PhTnpPaint(hwnd, Context, hdc, &updateRect);
                }

                EndPaint(hwnd, &paintStruct);
            }
        }
    }
    static  VOID PhTnpDestroyBufferedContext(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        // The original bitmap must be selected back into the context, otherwise the bitmap can't be
        // deleted.
        SelectBitmap(Context->BufferedContext, Context->BufferedOldBitmap);
        DeleteBitmap(Context->BufferedBitmap);
        DeleteDC(Context->BufferedContext);

        Context->BufferedContext = NULL;
        Context->BufferedBitmap = NULL;
    }
    static  VOID PhTnpLayoutHeader(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        RECT rect;
        HDLAYOUT hdl;
        WINDOWPOS windowPos;

        hdl.prc = &rect;
        hdl.pwpos = &windowPos;

        if (!(Context->Style & TN_STYLE_NO_COLUMN_HEADER))
        {
            // Fixed portion header control
            rect.left = 0;
            rect.top = 0;
            rect.right = Context->NormalLeft;
            rect.bottom = Context->ClientRect.bottom;
            Header_Layout(Context->FixedHeaderHandle, &hdl);
            SetWindowPos(Context->FixedHeaderHandle, NULL, windowPos.x, windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);
            Context->HeaderHeight = windowPos.cy;

            // Normal portion header control
            rect.left = Context->NormalLeft - Context->HScrollPosition;
            rect.top = 0;
            rect.right = Context->ClientRect.right - (Context->VScrollVisible ? Context->VScrollWidth : 0);
            rect.bottom = Context->ClientRect.bottom;
            Header_Layout(Context->HeaderHandle, &hdl);
            SetWindowPos(Context->HeaderHandle, NULL, windowPos.x, windowPos.y, windowPos.cx, windowPos.cy, windowPos.flags);
        }
        else
        {
            Context->HeaderHeight = 0;
        }

        if (Context->TooltipsHandle)
        {
            TOOLINFO toolInfo;

            memset(&toolInfo, 0, sizeof(TOOLINFO));
            toolInfo.cbSize = sizeof(TOOLINFO);
            toolInfo.hwnd = Context->FixedHeaderHandle;
            toolInfo.uId = TNP_TOOLTIPS_FIXED_HEADER;
            GetClientRect(Context->FixedHeaderHandle, &toolInfo.rect);
            SendMessage(Context->TooltipsHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);

            toolInfo.hwnd = Context->HeaderHandle;
            toolInfo.uId = TNP_TOOLTIPS_HEADER;
            GetClientRect(Context->HeaderHandle, &toolInfo.rect);
            SendMessage(Context->TooltipsHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
        }
    }
    static  VOID PhTnpProcessScroll(
        _In_ PPH_TREENEW_CONTEXT Context,
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
                Context->Handle,
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
            if (Context->FlatList->Count != 0)
            {
                deltaY = DeltaRows * Context->RowHeight;

                // If we're scrolling vertically as well, we need to scroll the fixed part and the
                // normal part separately.

                if (DeltaRows != 0)
                {
                    rect.left = 0;
                    rect.right = Context->NormalLeft;
                    ScrollWindowEx(
                        Context->Handle,
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
                    Context->Handle,
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
    static  VOID PhTnpUpdateScrollBars(
        _In_ PPH_TREENEW_CONTEXT Context
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
        //contentHeight = (LONG)Context->FlatList->Count * Context->RowHeight;

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
        GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
        oldPosition = scrollInfo.nPos;

        scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
        scrollInfo.nMin = 0;
        //scrollInfo.nMax = Context->FlatList->Count != 0 ? Context->FlatList->Count - 1 : 0;
        scrollInfo.nPage = height / Context->RowHeight;
        SetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo, TRUE);

        // The scroll position may have changed due to the modified scroll range.
        scrollInfo.fMask = SIF_POS;
        GetScrollInfo(Context->VScrollHandle, SB_CTL, &scrollInfo);
        deltaRows = scrollInfo.nPos - oldPosition;
        Context->VScrollPosition = scrollInfo.nPos;

        if (contentHeight > height && contentHeight != 0)
        {
            ShowWindow(Context->VScrollHandle, SW_SHOW);
            Context->VScrollVisible = TRUE;
        }
        else
        {
            ShowWindow(Context->VScrollHandle, SW_HIDE);
            Context->VScrollVisible = FALSE;
        }

        // Horizontal scroll bar

        scrollInfo.cbSize = sizeof(SCROLLINFO);
        scrollInfo.fMask = SIF_POS;
        GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);
        oldPosition = scrollInfo.nPos;

        scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
        scrollInfo.nMin = 0;
        scrollInfo.nMax = contentWidth != 0 ? contentWidth - 1 : 0;
        scrollInfo.nPage = width;
        SetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo, TRUE);

        scrollInfo.fMask = SIF_POS;
        GetScrollInfo(Context->HScrollHandle, SB_CTL, &scrollInfo);
        deltaX = scrollInfo.nPos - oldPosition;
        Context->HScrollPosition = scrollInfo.nPos;

        oldHScrollVisible = Context->HScrollVisible;

        if (contentWidth > width && contentWidth != 0)
        {
            ShowWindow(Context->HScrollHandle, SW_SHOW);
            Context->HScrollVisible = TRUE;
        }
        else
        {
            ShowWindow(Context->HScrollHandle, SW_HIDE);
            Context->HScrollVisible = FALSE;
        }

        if ((Context->HScrollVisible != oldHScrollVisible) && Context->FixedDividerVisible && Context->AnimateDivider)
        {
            rect.left = Context->FixedWidth;
            rect.top = Context->HeaderHeight;
            rect.right = Context->FixedWidth + 1;
            rect.bottom = Context->ClientRect.bottom;
            InvalidateRect(Context->Handle, &rect, FALSE);
        }

        if (deltaRows != 0 || deltaX != 0)
            PhTnpProcessScroll(Context, deltaRows, deltaX);

        ShowWindow(Context->FillerBoxHandle, (Context->VScrollVisible && Context->HScrollVisible) ? SW_SHOW : SW_HIDE);
    }
    static  VOID PhTnpLayout(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        RECT clientRect;

        if (Context->EnableRedraw <= 0)
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
                Context->VScrollHandle,
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
                Context->HScrollHandle,
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
                Context->FillerBoxHandle,
                clientRect.right - Context->VScrollWidth,
                clientRect.bottom - Context->HScrollHeight,
                Context->VScrollWidth,
                Context->HScrollHeight,
                TRUE
            );
        }

        PhTnpLayoutHeader(Context);

        // Redraw the entire window if we are displaying empty text.
        if (Context->FlatList->Count == 0 && Context->EmptyText.Length != 0)
            InvalidateRect(Context->Handle, NULL, FALSE);
    }
    static  VOID PhTnpOnSize(
        _In_ HWND hwnd,
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        GetClientRect(hwnd, &Context->ClientRect);

        if (Context->BufferedContext && (
            Context->BufferedContextRect.right < Context->ClientRect.right ||
            Context->BufferedContextRect.bottom < Context->ClientRect.bottom))
        {
            // Invalidate the buffered context because the client size has increased.
            PhTnpDestroyBufferedContext(Context);
        }

        PhTnpLayout(Context);

        if (Context->TooltipsHandle)
        {
            TOOLINFO toolInfo;

            memset(&toolInfo, 0, sizeof(TOOLINFO));
            toolInfo.cbSize = sizeof(TOOLINFO);
            toolInfo.hwnd = hwnd;
            toolInfo.uId = TNP_TOOLTIPS_ITEM;
            toolInfo.rect = Context->ClientRect;
            SendMessage(Context->TooltipsHandle, TTM_NEWTOOLRECT, 0, (LPARAM)&toolInfo);
        }
    }

    static  VOID PhTnpOnSetFont(
        _In_ HWND hwnd,
        _In_ PPH_TREENEW_CONTEXT Context,
        _In_opt_ HFONT Font,
        _In_ LOGICAL Redraw
    )
    {
        PhTnpSetFont(Context, Font, !!Redraw);
        PhTnpLayout(Context);
    }
    static  BOOLEAN PhTnpOnCreate(
        _In_ HWND hwnd,
        _In_ PPH_TREENEW_CONTEXT Context,
        _In_ CREATESTRUCT *CreateStruct
    )
    {
        ULONG               headerStyle;
        Setting::Color      *pColor;

        Context->Handle = hwnd;
        Context->InstanceHandle = CreateStruct->hInstance;
        Context->Style = CreateStruct->style;
        Context->ExtendedStyle = CreateStruct->dwExStyle;

        pColor = (Setting::Color *)CreateStruct->lpCreateParams;

        if (Context->Style & TN_STYLE_DOUBLE_BUFFERED)
            Context->DoubleBuffered = TRUE;
        if ((Context->Style & TN_STYLE_ANIMATE_DIVIDER) && Context->DoubleBuffered)
            Context->AnimateDivider = TRUE;

        headerStyle = HDS_HORZ | HDS_FULLDRAG;

        if (!(Context->Style & TN_STYLE_NO_COLUMN_SORT))
            headerStyle |= HDS_BUTTONS;
        if (!(Context->Style & TN_STYLE_NO_COLUMN_HEADER))
            headerStyle |= WS_VISIBLE;

        if (Context->Style & TN_STYLE_CUSTOM_COLORS)
        {
            Context->CustomTextColor        = pColor->text  ? pColor->text : RGB(0xff, 0xff, 0xff);
            Context->CustomFocusColor       = pColor->focus ? pColor->focus : RGB(0x0, 0x0, 0xff);
            Context->CustomSelectedColor    = pColor->selection ? pColor->selection : RGB(0x0, 0x0, 0x80);
            Context->CustomColors           = TRUE;
        }
        else
        {
            Context->CustomFocusColor = GetSysColor(COLOR_HOTLIGHT);
            Context->CustomSelectedColor = GetSysColor(COLOR_HIGHLIGHT);
        }

        if (!(Context->FixedHeaderHandle = CreateWindow(
            WC_HEADER,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | headerStyle,
            0,
            0,
            0,
            0,
            hwnd,
            NULL,
            CreateStruct->hInstance,
            NULL
        )))
        {
            return FALSE;
        }

        if (!(Context->Style & TN_STYLE_NO_COLUMN_REORDER))
            headerStyle |= HDS_DRAGDROP;

        if (!(Context->HeaderHandle = CreateWindow(
            WC_HEADER,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | headerStyle,
            0,
            0,
            0,
            0,
            hwnd,
            NULL,
            CreateStruct->hInstance,
            NULL
        )))
        {
            return FALSE;
        }

        if (!(Context->VScrollHandle = CreateWindow(
            WC_SCROLLBAR,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_VERT,
            0,
            0,
            0,
            0,
            hwnd,
            NULL,
            CreateStruct->hInstance,
            NULL
        )))
        {
            return FALSE;
        }

        if (!(Context->HScrollHandle = CreateWindow(
            WC_SCROLLBAR,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_HORZ,
            0,
            0,
            0,
            0,
            hwnd,
            NULL,
            CreateStruct->hInstance,
            NULL
        )))
        {
            return FALSE;
        }

        if (!(Context->FillerBoxHandle = CreateWindow(
            WC_STATIC,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
            0,
            0,
            0,
            0,
            hwnd,
            NULL,
            CreateStruct->hInstance,
            NULL
        )))
        {
            return FALSE;
        }

        TnpSetFont(Context, NULL, FALSE); // use default font
        TnpUpdateSystemMetrics(Context);
        TnpInitializeTooltips(Context);

        return TRUE;
    }
    VOID TnpSetFont(
        _In_ PPH_TREENEW_CONTEXT Context,
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

        SetWindowFont(Context->FixedHeaderHandle, Context->Font, Redraw);
        SetWindowFont(Context->HeaderHandle, Context->Font, Redraw);

        if (Context->TooltipsHandle)
        {
            SetWindowFont(Context->TooltipsHandle, Context->Font, FALSE);
            Context->TooltipFont = Context->Font;
        }
        TnpUpdateTextMetrics(Context);
    }

    VOID TnpUpdateSystemMetrics(
        _In_ PPH_TREENEW_CONTEXT Context
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
    VOID TnpUpdateTextMetrics(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        HDC hdc;

        if (hdc = GetDC(Context->Handle))
        {
            SelectFont(hdc, Context->Font);
            GetTextMetrics(hdc, &Context->TextMetrics);

            if (!Context->CustomRowHeight)
            {
                // Below we try to match the row height as calculated by the list view, even if it
                // involves magic numbers. On Vista and above there seems to be extra padding.

                Context->RowHeight = Context->TextMetrics.tmHeight;

                if (Context->Style & TN_STYLE_ICONS)
                {
                    if (Context->RowHeight < m_nSmallIconHeight)
                        Context->RowHeight = m_nSmallIconHeight;
                }
                else
                {
                    if (!(Context->Style & TN_STYLE_THIN_ROWS))
                        Context->RowHeight += 1; // HACK
                }

                Context->RowHeight += 1; // HACK

                if (!(Context->Style & TN_STYLE_THIN_ROWS))
                    Context->RowHeight += 2; // HACK
            }
            ReleaseDC(Context->Handle, hdc);
        }
    }
    VOID TnpInitializeTooltips(
        _In_ PPH_TREENEW_CONTEXT Context
    )
    {
        TOOLINFO toolInfo;

        Context->TooltipsHandle = CreateWindowEx(
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
            (HINSTANCE)Context->InstanceHandle,
            NULL
        );

        if (!Context->TooltipsHandle)
            return;

        // Item tooltips
        memset(&toolInfo, 0, sizeof(TOOLINFO));
        toolInfo.cbSize = sizeof(TOOLINFO);
        toolInfo.uFlags = TTF_TRANSPARENT;
        toolInfo.hwnd = Context->Handle;
        toolInfo.uId = TNP_TOOLTIPS_ITEM;
        toolInfo.lpszText = LPSTR_TEXTCALLBACK;
        toolInfo.lParam = TNP_TOOLTIPS_ITEM;
        SendMessage(Context->TooltipsHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

        // Fixed column tooltips
        toolInfo.uFlags = 0;
        toolInfo.hwnd = Context->FixedHeaderHandle;
        toolInfo.uId = TNP_TOOLTIPS_FIXED_HEADER;
        toolInfo.lpszText = LPSTR_TEXTCALLBACK;
        toolInfo.lParam = TNP_TOOLTIPS_FIXED_HEADER;
        SendMessage(Context->TooltipsHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

        // Normal column tooltips
        toolInfo.uFlags = 0;
        toolInfo.hwnd = Context->HeaderHandle;
        toolInfo.uId = TNP_TOOLTIPS_HEADER;
        toolInfo.lpszText = LPSTR_TEXTCALLBACK;
        toolInfo.lParam = TNP_TOOLTIPS_HEADER;
        SendMessage(Context->TooltipsHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

        // Hook the header control window procedures so we can forward mouse messages to the tooltip control.
        Context->HeaderWindowProc = (WNDPROC)GetWindowLongPtr(Context->HeaderHandle, GWLP_WNDPROC);
        Context->FixedHeaderWindowProc = (WNDPROC)GetWindowLongPtr(Context->FixedHeaderHandle, GWLP_WNDPROC);

        PhSetWindowContext(Context->HeaderHandle, 0xF, Context);
        PhSetWindowContext(Context->FixedHeaderHandle, 0xF, Context);

        SetWindowLongPtr(Context->FixedHeaderHandle, GWLP_WNDPROC, (LONG_PTR)TnpHeaderHookWndProc);
        SetWindowLongPtr(Context->HeaderHandle, GWLP_WNDPROC, (LONG_PTR)TnpHeaderHookWndProc);

        SendMessage(Context->TooltipsHandle, TTM_SETMAXTIPWIDTH, 0, MAXSHORT); // no limit
        SetWindowFont(Context->TooltipsHandle, Context->Font, FALSE);
        Context->TooltipFont = Context->Font;
    }
    static  LRESULT CALLBACK TnpHeaderHookWndProc(
        _In_ HWND hwnd,
        _In_ UINT uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
    )
    {
        PPH_TREENEW_CONTEXT context;
        WNDPROC oldWndProc;

        context = GetWindowContext(hwnd, 0xF);

        if (hwnd == context->FixedHeaderHandle)
            oldWndProc = context->FixedHeaderWindowProc;
        else
            oldWndProc = context->HeaderWindowProc;

        switch (uMsg)
        {
        case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwnd, 0xF);
        }
        break;
        case WM_MOUSEMOVE:
        {
            POINT point;
            PPH_TREENEW_COLUMN column;
            ULONG id;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);
            column = PhTnpHitTestHeader(context, hwnd == context->FixedHeaderHandle, &point, NULL);

            if (column)
                id = column->Id;
            else
                id = -1;

            if (context->TooltipColumnId != id)
            {
                PhTnpPopTooltip(context);
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
                if (header->hwndFrom == context->TooltipsHandle)
                {
                    NMTTDISPINFO *info = (NMTTDISPINFO *)header;
                    POINT point;

                    PhTnpGetMessagePos(hwnd, &point);
                    PhTnpGetHeaderTooltipText(context, info->lParam == TNP_TOOLTIPS_FIXED_HEADER, &point, &info->lpszText);
                }
            }
            break;
            case TTN_SHOW:
            {
                if (header->hwndFrom == context->TooltipsHandle)
                {
                    return PhTnpPrepareTooltipShow(context);
                }
            }
            break;
            case TTN_POP:
            {
                if (header->hwndFrom == context->TooltipsHandle)
                {
                    PhTnpPrepareTooltipPop(context);
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
            if (context->TooltipsHandle)
            {
                MSG message;

                message.hwnd = hwnd;
                message.message = uMsg;
                message.wParam = wParam;
                message.lParam = lParam;
                SendMessage(context->TooltipsHandle, TTM_RELAYEVENT, 0, (LPARAM)&message);
            }
        }
        break;
        }

        return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
    }
    PVOID GetWindowContext(
        _In_ HWND WindowHandle,
        _In_ ULONG PropertyHash
    )
    {
        /*
        PH_WINDOW_PROPERTY_CONTEXT lookupEntry;
        PPH_WINDOW_PROPERTY_CONTEXT entry;

        lookupEntry.WindowHandle = WindowHandle;
        lookupEntry.PropertyHash = PropertyHash;

        PhAcquireQueuedLockShared(&WindowContextListLock);
        entry = PhFindEntryHashtable(WindowContextHashTable, &lookupEntry);
        PhReleaseQueuedLockShared(&WindowContextListLock);

        if (entry)
            return entry->Context;
        else
            return NULL;
        */
        return NULL;
    }
    VOID PhSetWindowContext(
        _In_ HWND WindowHandle,
        _In_ ULONG PropertyHash,
        _In_ PVOID Context
    )
    {
        /*
        PH_WINDOW_PROPERTY_CONTEXT entry;

        entry.WindowHandle = WindowHandle;
        entry.PropertyHash = PropertyHash;
        entry.Context = Context;

        PhAcquireQueuedLockExclusive(&WindowContextListLock);
        PhAddEntryHashtable(WindowContextHashTable, &entry);
        PhReleaseQueuedLockExclusive(&WindowContextListLock);
        */
    }

    VOID PhRemoveWindowContext(
        _In_ HWND WindowHandle,
        _In_ ULONG PropertyHash
    )
    {
        /*
        PH_WINDOW_PROPERTY_CONTEXT lookupEntry;

        lookupEntry.WindowHandle = WindowHandle;
        lookupEntry.PropertyHash = PropertyHash;

        PhAcquireQueuedLockExclusive(&WindowContextListLock);
        PhRemoveEntryHashtable(WindowContextHashTable, &lookupEntry);
        PhReleaseQueuedLockExclusive(&WindowContextListLock);
        */
    }
};
