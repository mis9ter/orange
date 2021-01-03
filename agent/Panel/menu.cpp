#include "framework.h"
#include "emenu.h"

#define PH_MENU_ITEM_LOCATION_HACKER 0
#define PH_MENU_ITEM_LOCATION_VIEW 1
#define PH_MENU_ITEM_LOCATION_TOOLS 2
#define PH_MENU_ITEM_LOCATION_USERS 3
#define PH_MENU_ITEM_LOCATION_HELP 4

#ifdef ENABLE_RTL_NUMBER_OF_V2
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V2(A)
#else
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V1(A)
#endif

#define MAIN_MENU_COUNT     5

static HMENU SubMenuHandles[MAIN_MENU_COUNT];
static PPH_EMENU SubMenuObjects[MAIN_MENU_COUNT];

PPH_EMENU PhpCreateHackerMenu(
    _In_ PPH_EMENU HackerMenu
)
{
    PPH_EMENU_ITEM menuItem;

    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_RUN, (PWSTR)L"&Run...\bCtrl+R", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_RUNAS, (PWSTR)L"Run &as...\bCtrl+Shift+R", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_SHOWDETAILSFORALLPROCESSES, (PWSTR)L"Show &details for all processes", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_SAVE, (PWSTR)L"&Save...\bCtrl+S", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_FINDHANDLESORDLLS, (PWSTR)L"&Find handles or DLLs...\bCtrl+F", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_OPTIONS, (PWSTR)L"&Options...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, PhCreateEMenuSeparator(), ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, (PWSTR)L"&Computer", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_LOCK, (PWSTR)L"&Lock", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_LOGOFF, (PWSTR)L"Log o&ff", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SLEEP, (PWSTR)L"&Sleep", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_HIBERNATE, (PWSTR)L"&Hibernate", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTART, (PWSTR)L"R&estart", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_RESTARTBOOTOPTIONS, (PWSTR)L"Restart to boot options", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SHUTDOWN, (PWSTR)L"Shu&t down", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_COMPUTER_SHUTDOWNHYBRID, (PWSTR)L"H&ybrid shut down", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(HackerMenu, menuItem, ULONG_MAX);

    PhInsertEMenuItem(HackerMenu, PhCreateEMenuItem(0, ID_HACKER_EXIT, (PWSTR)L"E&xit", NULL, NULL), ULONG_MAX);

    return HackerMenu;
}
PPH_EMENU PhpCreateMainMenu(
    _In_ ULONG SubMenuIndex
)
{
    PPH_EMENU menu = PhCreateEMenu();
    PPH_EMENU_ITEM menuItem;

    switch (SubMenuIndex)
    {
        case PH_MENU_ITEM_LOCATION_HACKER:
            return PhpCreateHackerMenu(menu);
    }
    menu->Flags |= PH_EMENU_MAINMENU;

    menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, PH_MENU_ITEM_LOCATION_HACKER, (PWSTR)L"&Hacker", NULL, NULL);
    PhInsertEMenuItem(menu, PhpCreateHackerMenu(menuItem), ULONG_MAX);

    menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, 1, (PWSTR)L"&Hacker", NULL, NULL);
    PhInsertEMenuItem(menu, PhpCreateHackerMenu(menuItem), ULONG_MAX);

    menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, 2, (PWSTR)L"&Hacker", NULL, NULL);
    PhInsertEMenuItem(menu, PhpCreateHackerMenu(menuItem), ULONG_MAX);

    menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, 3, (PWSTR)L"&Hacker", NULL, NULL);
    PhInsertEMenuItem(menu, PhpCreateHackerMenu(menuItem), ULONG_MAX);

    menuItem = PhCreateEMenuItem(PH_EMENU_MAINMENU, 4, (PWSTR)L"&Hacker", NULL, NULL);
    PhInsertEMenuItem(menu, PhpCreateHackerMenu(menuItem), ULONG_MAX);



    return menu;
}
VOID PhMwpInitializeMainMenu(
    _In_ HMENU Menu
)
{
    MENUINFO menuInfo;
    ULONG i;

    memset(&menuInfo, 0, sizeof(MENUINFO));
    menuInfo.cbSize = sizeof(MENUINFO);
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_NOTIFYBYPOS; //| MNS_AUTODISMISS; Flag is unusable on Win10 - Github #547 (dmex).

    SetMenuInfo(Menu, &menuInfo);
    for (i = 0; i < _countof(SubMenuHandles); i++)
    {
        SubMenuHandles[i] = GetSubMenu(Menu, i);
    }
}

static const PH_FLAG_MAPPING EMenuStateMappings[] =
{
    { PH_EMENU_CHECKED, MFS_CHECKED },
    { PH_EMENU_DEFAULT, MFS_DEFAULT },
    { PH_EMENU_DISABLED, MFS_DISABLED },
    { PH_EMENU_HIGHLIGHT, MFS_HILITE }
};
static const PH_FLAG_MAPPING EMenuTypeMappings[] =
{
    { PH_EMENU_MENUBARBREAK, MFT_MENUBARBREAK },
    { PH_EMENU_MENUBREAK, MFT_MENUBREAK },
    { PH_EMENU_RADIOCHECK, MFT_RADIOCHECK }
};

static  _May_raise_ ULONG PhGetIntegerSetting(
    _In_ PCWSTR Name
)
{
    return 0;
}

static  VOID PhMapFlags1(
    _Inout_ PULONG Value2,
    _In_ ULONG Value1,
    _In_ const PH_FLAG_MAPPING *Mappings,
    _In_ ULONG NumberOfMappings
)
{
    ULONG i;
    ULONG value2;

    value2 = *Value2;

    if (value2 != 0)
    {
        // There are existing flags. Map the flags we know about by clearing/setting them. The flags
        // we don't know about won't be affected.

        for (i = 0; i < NumberOfMappings; i++)
        {
            if (Value1 & Mappings[i].Flag1)
                value2 |= Mappings[i].Flag2;
            else
                value2 &= ~Mappings[i].Flag2;
        }
    }
    else
    {
        // There are no existing flags, which means we can build the value from scratch, with no
        // clearing needed.

        for (i = 0; i < NumberOfMappings; i++)
        {
            if (Value1 & Mappings[i].Flag1)
                value2 |= Mappings[i].Flag2;
        }
    }

    *Value2 = value2;
}


VOID _PhEMenuToHMenu2(
    _In_ HMENU MenuHandle,
    _In_ PPH_EMENU_ITEM Menu,
    _In_ ULONG Flags,
    _Inout_opt_ PPH_EMENU_DATA Data
)
{
    ULONG i;
    PPH_EMENU_ITEM item;
    MENUITEMINFO menuItemInfo;

    for (i = 0; i < Menu->Items->Count; i++)
    {
        item = (PPH_EMENU_ITEM)Menu->Items->Items[i];

        memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
        menuItemInfo.cbSize = sizeof(MENUITEMINFO);

        // Type

        menuItemInfo.fMask = MIIM_FTYPE | MIIM_STATE;

        if (item->Flags & PH_EMENU_SEPARATOR)
        {
            menuItemInfo.fType = MFT_SEPARATOR;
        }
        else
        {
            menuItemInfo.fType = MFT_STRING;
            menuItemInfo.fMask |= MIIM_STRING;
            menuItemInfo.dwTypeData = item->Text;
        }

        PhMapFlags1(
            (PULONG)&menuItemInfo.fType,
            item->Flags,
            EMenuTypeMappings,
            sizeof(EMenuTypeMappings) / sizeof(PH_FLAG_MAPPING)
        );

        // Bitmap

        if (item->Bitmap)
        {
            menuItemInfo.hbmpItem = item->Bitmap;

            if (WindowsVersion < WINDOWS_10_19H2)
            {
                if (!PhGetIntegerSetting(L"EnableThemeSupport"))
                    menuItemInfo.fMask |= MIIM_BITMAP;
            }
            else
            {
                menuItemInfo.fMask |= MIIM_BITMAP;
            }
        }

        // Id

        if (Flags & PH_EMENU_CONVERT_ID)
        {
            ULONG id;

            if (Data)
            {
                PhAddItemList(Data->IdToItem, item);
                id = Data->IdToItem->Count;

                menuItemInfo.fMask |= MIIM_ID;
                menuItemInfo.wID = id;
            }
        }
        else
        {
            if (item->Id)
            {
                menuItemInfo.fMask |= MIIM_ID;
                menuItemInfo.wID = item->Id;
            }
        }

        // State

        PhMapFlags1(
            (PULONG)&menuItemInfo.fState,
            item->Flags,
            EMenuStateMappings,
            sizeof(EMenuStateMappings) / sizeof(PH_FLAG_MAPPING)
        );

        // Context

        menuItemInfo.fMask |= MIIM_DATA;
        menuItemInfo.dwItemData = (ULONG_PTR)item;

        // Submenu

        if (item->Items && item->Items->Count != 0)
        {
            menuItemInfo.fMask |= MIIM_SUBMENU;
            menuItemInfo.hSubMenu = PhEMenuToHMenu(item, Flags, Data);
        }

        // Themes
        if (WindowsVersion < WINDOWS_10_19H2)
        {
            if (PhGetIntegerSetting(L"EnableThemeSupport"))
            {
                menuItemInfo.fType |= MFT_OWNERDRAW;
            }
        }
        else
        {
            if (item->Flags & PH_EMENU_MAINMENU)
            {
                if (PhGetIntegerSetting(L"EnableThemeSupport"))
                {
                    menuItemInfo.fType |= MFT_OWNERDRAW;
                }
            }
        }

        InsertMenuItem(MenuHandle, MAXINT, TRUE, &menuItemInfo);
    }
}

BOOLEAN PhEnableThemeSupport;
VOID PhMwpInitializeSubMenu(
    _In_ PPH_EMENU Menu,
    _In_ ULONG Index
)

{

}
VOID OnInitMenuPopup(
    _In_ HWND WindowHandle,
    _In_ HMENU Menu,
    _In_ ULONG Index,
    _In_ BOOLEAN IsWindowMenu
)
{
    YAgent::Alert(L"%S", __FUNCTION__);
}

void	CreateMainWindowMenu(IN HWND hWnd) {
    HMENU   hMenu   = NULL;

    if( hMenu = CreateMenu() ) {

        

        SetMenu(hWnd, hMenu);


        PPH_EMENU   pMenu   = PhpCreateMainMenu(ULONG_MAX);
        //YAgent::Alert(L"pMenu=%p pMenu->Item->Count=%d", pMenu, pMenu? pMenu->Items->Count : 0);


        _PhEMenuToHMenu2(hMenu, pMenu, 0, NULL);
        PhMwpInitializeMainMenu(hMenu);
       // if (PhEnableThemeSupport) PhInitializeWindowThemeMainMenu(windowMenuHandle);
    }



}