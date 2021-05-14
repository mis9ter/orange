#include "framework.h"

#define PhpAddIntegerSetting(a,b)       p->Add(Setting::INTEGER, a, b)
#define PhpAddStringSetting(a,b)        p->Add(Setting::STRING, a, b)
#define PhpAddIntegerPairSetting(a,b)   p->Add(Setting::INTEGER_PAIR, a, b)
#define PhpAddScalableIntegerPairSetting(a,b)   p->Add(Setting::SCALABLE_INTEGER_PAIR, a, b)

void    SetDefaultSettings(CSettings * p)
{
    PhpAddIntegerSetting(L"AllowOnlyOneInstance", L"1");
    PhpAddIntegerSetting(L"CloseOnEscape", L"0");
    PhpAddStringSetting(L"DbgHelpSearchPath", L"SRV*C:\\Symbols*https://msdl.microsoft.com/download/symbols");
    PhpAddIntegerSetting(L"DbgHelpUndecorate", L"1");
    PhpAddStringSetting(L"DisabledPlugins", L"");
    PhpAddIntegerSetting(L"ElevationLevel", L"1"); // PromptElevateAction
    PhpAddIntegerSetting(L"EnableCycleCpuUsage", L"1");
    PhpAddIntegerSetting(L"EnableInstantTooltips", L"0");
    PhpAddIntegerSetting(L"EnableKph", L"1");
    PhpAddIntegerSetting(L"EnableKphWarnings", L"1");
    PhpAddIntegerSetting(L"EnableHandleSnapshot", L"1");
    PhpAddIntegerSetting(L"EnableNetworkResolve", L"1");
    PhpAddIntegerSetting(L"EnableNetworkResolveDoH", L"0");
    PhpAddIntegerSetting(L"EnablePlugins", L"1");
    PhpAddIntegerSetting(L"EnableServiceNonPoll", L"0");
    PhpAddIntegerSetting(L"EnableStage2", L"1");
    PhpAddIntegerSetting(L"EnableServiceStage2", L"0");
    PhpAddIntegerSetting(L"EnableWarnings", L"1");
    PhpAddIntegerSetting(L"EnableWindowText", L"1");
    PhpAddIntegerSetting(L"EnableThemeSupport", L"0");
    PhpAddIntegerSetting(L"EnableTooltipSupport", L"1");
    PhpAddIntegerSetting(L"EnableSecurityAdvancedDialog", L"1");
    PhpAddIntegerSetting(L"EnableLinuxSubsystemSupport", L"0");
    PhpAddStringSetting(L"EnvironmentTreeListColumns", L"");
    PhpAddStringSetting(L"EnvironmentTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddIntegerSetting(L"EnvironmentTreeListFlags", L"0");
    PhpAddIntegerSetting(L"FindObjRegex", L"0");
    PhpAddStringSetting(L"FindObjTreeListColumns", L"");
    PhpAddIntegerPairSetting(L"FindObjWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"FindObjWindowSize", L"@96|550,420");
    PhpAddStringSetting(L"FileBrowseExecutable", L"%SystemRoot%\\explorer.exe /select,\"%s\"");
    PhpAddIntegerSetting(L"FirstRun", L"1");
    PhpAddStringSetting(L"Font", L""); // null
    PhpAddIntegerSetting(L"ForceNoParent", L"1");
    PhpAddIntegerSetting(L"KphBuildNumber", L"0");
    PhpAddStringSetting(L"HandleTreeListColumns", L"");
    PhpAddStringSetting(L"HandleTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(L"HandleTreeListFlags", L"3");
    PhpAddIntegerPairSetting(L"HandlePropertiesWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"HandlePropertiesWindowSize", L"@96|260,260");
    PhpAddIntegerSetting(L"HiddenProcessesMenuEnabled", L"0");
    PhpAddStringSetting(L"HiddenProcessesListViewColumns", L"");
    PhpAddIntegerPairSetting(L"HiddenProcessesWindowPosition", L"400,400");
    PhpAddScalableIntegerPairSetting(L"HiddenProcessesWindowSize", L"@96|520,400");
    PhpAddIntegerSetting(L"HideDriverServices", L"0");
    PhpAddIntegerSetting(L"HideFreeRegions", L"1");
    PhpAddIntegerSetting(L"HideOnClose", L"0");
    PhpAddIntegerSetting(L"HideOnMinimize", L"0");
    PhpAddIntegerSetting(L"HideOtherUserProcesses", L"0");
    PhpAddIntegerSetting(L"HideSignedProcesses", L"0");
    PhpAddIntegerSetting(L"HideWaitingConnections", L"0");
    PhpAddIntegerSetting(L"HighlightingDuration", L"3e8"); // 1000ms
    PhpAddStringSetting(L"IconGuids", L"");
    PhpAddStringSetting(L"IconSettings", L"1|1");
    PhpAddIntegerSetting(L"IconNotifyMask", L"c"); // PH_NOTIFY_SERVICE_CREATE | PH_NOTIFY_SERVICE_DELETE
    PhpAddIntegerSetting(L"IconProcesses", L"f"); // 15
    PhpAddIntegerSetting(L"IconSingleClick", L"0");
    PhpAddIntegerSetting(L"IconTogglesVisibility", L"1");
    PhpAddStringSetting(L"JobListViewColumns", L"");
    //PhpAddIntegerSetting(L"KphUnloadOnShutdown", L"0");
    PhpAddIntegerSetting(L"LogEntries", L"200"); // 512
    PhpAddStringSetting(L"LogListViewColumns", L"");
    PhpAddIntegerPairSetting(L"LogWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"LogWindowSize", L"@96|450,500");
    PhpAddIntegerSetting(L"MainWindowAlwaysOnTop", L"0");
    PhpAddStringSetting(L"MainWindowClassName", L"MainWindowClassName");
    PhpAddIntegerSetting(L"MainWindowOpacity", L"0"); // means 100%
    PhpAddIntegerPairSetting(L"MainWindowPosition", L"100,100");
    PhpAddScalableIntegerPairSetting(L"MainWindowSize", L"@96|800,600");
    PhpAddIntegerSetting(L"MainWindowState", L"1");
    PhpAddIntegerSetting(L"MainWindowTabRestoreEnabled", L"0");
    PhpAddIntegerSetting(L"MainWindowTabRestoreIndex", L"0");
    PhpAddIntegerSetting(L"MaxSizeUnit", L"6");
    PhpAddIntegerSetting(L"MemEditBytesPerRow", L"10"); // 16
    PhpAddStringSetting(L"MemEditGotoChoices", L"");
    PhpAddIntegerPairSetting(L"MemEditPosition", L"450,450");
    PhpAddScalableIntegerPairSetting(L"MemEditSize", L"@96|600,500");
    PhpAddStringSetting(L"MemFilterChoices", L"");
    PhpAddStringSetting(L"MemResultsListViewColumns", L"");
    PhpAddIntegerPairSetting(L"MemResultsPosition", L"300,300");
    PhpAddScalableIntegerPairSetting(L"MemResultsSize", L"@96|500,520");
    PhpAddIntegerSetting(L"MemoryListFlags", L"3");
    PhpAddStringSetting(L"MemoryTreeListColumns", L"");
    PhpAddStringSetting(L"MemoryTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddIntegerPairSetting(L"MemoryListsWindowPosition", L"400,400");
    PhpAddStringSetting(L"MemoryReadWriteAddressChoices", L"");
    PhpAddStringSetting(L"MiniInfoWindowClassName", L"MiniInfoWindowClassName");
    PhpAddIntegerSetting(L"MiniInfoWindowEnabled", L"1");
    PhpAddIntegerSetting(L"MiniInfoWindowOpacity", L"0"); // means 100%
    PhpAddIntegerSetting(L"MiniInfoWindowPinned", L"0");
    PhpAddIntegerPairSetting(L"MiniInfoWindowPosition", L"200,200");
    PhpAddIntegerSetting(L"MiniInfoWindowRefreshAutomatically", L"1");
    PhpAddScalableIntegerPairSetting(L"MiniInfoWindowSize", L"@96|10,200");
    PhpAddIntegerSetting(L"ModuleTreeListFlags", L"1");
    PhpAddStringSetting(L"ModuleTreeListColumns", L"");
    PhpAddStringSetting(L"ModuleTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddStringSetting(L"NetworkTreeListColumns", L"");
    PhpAddStringSetting(L"NetworkTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(L"NoPurgeProcessRecords", L"0");
    PhpAddIntegerPairSetting(L"OptionsWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"OptionsWindowSize", L"@96|900,590");
    PhpAddIntegerPairSetting(L"PageFileWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"PageFileWindowSize", L"@96|500,300");
    PhpAddStringSetting(L"PageFileListViewColumns", L"");
    PhpAddIntegerPairSetting(L"PluginManagerWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"PluginManagerWindowSize", L"@96|900,590");
    PhpAddStringSetting(L"PluginManagerTreeListColumns", L"");
    PhpAddStringSetting(L"PluginsDirectory", L"plugins");
    PhpAddStringSetting(L"ProcessServiceListViewColumns", L"");
    PhpAddStringSetting(L"ProcessTreeColumnSetConfig", L"");
    PhpAddStringSetting(L"ProcessTreeListColumns", L"");
    PhpAddStringSetting(L"ProcessTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddStringSetting(L"ProcPropPage", L"General");
    PhpAddIntegerPairSetting(L"ProcPropPosition", L"200,200");
    PhpAddScalableIntegerPairSetting(L"ProcPropSize", L"@96|460,580");
    PhpAddStringSetting(L"ProcStatPropPageGroupStates", L"");
    PhpAddStringSetting(L"ProgramInspectExecutables", L"peview.exe \"%s\"");
    PhpAddIntegerSetting(L"PropagateCpuUsage", L"0");
    PhpAddStringSetting(L"RunAsProgram", L"");
    PhpAddStringSetting(L"RunAsUserName", L"");
    PhpAddIntegerSetting(L"RunFileDlgState", L"0");
    PhpAddIntegerSetting(L"SampleCount", L"200"); // 512
    PhpAddIntegerSetting(L"SampleCountAutomatic", L"1");
    PhpAddIntegerSetting(L"ScrollToNewProcesses", L"0");
    PhpAddStringSetting(L"SearchEngine", L"https://www.google.com/search?q=\"%s\"");
    PhpAddIntegerPairSetting(L"ServiceWindowPosition", L"0,0");
    PhpAddStringSetting(L"ServiceListViewColumns", L"");
    PhpAddStringSetting(L"ServiceTreeListColumns", L"");
    PhpAddStringSetting(L"ServiceTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerPairSetting(L"SessionShadowHotkey", L"106,2"); // VK_MULTIPLY,KBDCTRL
    PhpAddIntegerSetting(L"ShowPluginLoadErrors", L"0");
    PhpAddIntegerSetting(L"ShowCommitInSummary", L"1");
    PhpAddIntegerSetting(L"ShowCpuBelow001", L"0");
    PhpAddIntegerSetting(L"ShowHexId", L"0");
    PhpAddIntegerSetting(L"StartHidden", L"0");
    PhpAddIntegerSetting(L"SysInfoWindowAlwaysOnTop", L"0");
    PhpAddIntegerSetting(L"SysInfoWindowOneGraphPerCpu", L"0");
    PhpAddIntegerPairSetting(L"SysInfoWindowPosition", L"200,200");
    PhpAddStringSetting(L"SysInfoWindowSection", L"");
    PhpAddScalableIntegerPairSetting(L"SysInfoWindowSize", L"@96|620,590");
    PhpAddIntegerSetting(L"SysInfoWindowState", L"1");
    PhpAddIntegerSetting(L"ThinRows", L"0");
    PhpAddStringSetting(L"ThreadTreeListColumns", L"");
    PhpAddStringSetting(L"ThreadTreeListSort", L"1,2"); // 1, DescendingSortOrder
    PhpAddIntegerSetting(L"ThreadTreeListFlags", L"0");
    PhpAddStringSetting(L"ThreadStackTreeListColumns", L"");
    PhpAddScalableIntegerPairSetting(L"ThreadStackWindowSize", L"@96|420,400");
    PhpAddStringSetting(L"TokenGroupsListViewColumns", L"");
    PhpAddStringSetting(L"TokenGroupsListViewStates", L"");
    PhpAddStringSetting(L"TokenGroupsListViewSort", L"1,2");
    PhpAddIntegerSetting(L"TokenSplitterEnable", L"0");
    PhpAddIntegerSetting(L"TokenSplitterPosition", L"150");
    PhpAddStringSetting(L"TokenPrivilegesListViewColumns", L"");
    PhpAddIntegerSetting(L"TreeListBorderEnable", L"0");
    PhpAddIntegerSetting(L"TreeListCustomColorsEnable", L"0");
    PhpAddIntegerSetting(L"TreeListCustomColorText", L"0");
    PhpAddIntegerSetting(L"TreeListCustomColorFocus", L"0");
    PhpAddIntegerSetting(L"TreeListCustomColorSelection", L"0");
    PhpAddIntegerSetting(L"UpdateInterval", L"3e8"); // 1000ms
    PhpAddIntegerSetting(L"WmiProviderEnableHiddenMenu", L"0");
    PhpAddStringSetting(L"WmiProviderListViewColumns", L"");

    // Colors are specified with R in the lowest byte, then G, then B. So: bbggrr.
    PhpAddIntegerSetting(L"ColorNew", L"00ff7f"); // Chartreuse
    PhpAddIntegerSetting(L"ColorRemoved", L"283cff");
    PhpAddIntegerSetting(L"UseColorOwnProcesses", L"1");
    PhpAddIntegerSetting(L"ColorOwnProcesses", L"aaffff");
    PhpAddIntegerSetting(L"UseColorSystemProcesses", L"1");
    PhpAddIntegerSetting(L"ColorSystemProcesses", L"ffccaa");
    PhpAddIntegerSetting(L"UseColorServiceProcesses", L"1");
    PhpAddIntegerSetting(L"ColorServiceProcesses", L"ffffcc");
    PhpAddIntegerSetting(L"UseColorJobProcesses", L"0");
    PhpAddIntegerSetting(L"ColorJobProcesses", L"3f85cd"); // Peru
    PhpAddIntegerSetting(L"UseColorWow64Processes", L"0");
    PhpAddIntegerSetting(L"ColorWow64Processes", L"8f8fbc"); // Rosy Brown
    PhpAddIntegerSetting(L"UseColorDebuggedProcesses", L"1");
    PhpAddIntegerSetting(L"ColorDebuggedProcesses", L"ffbbcc");
    PhpAddIntegerSetting(L"UseColorElevatedProcesses", L"1");
    PhpAddIntegerSetting(L"ColorElevatedProcesses", L"00aaff");
    PhpAddIntegerSetting(L"UseColorHandleFiltered", L"1");
    PhpAddIntegerSetting(L"ColorHandleFiltered", L"000000"); // Black
    PhpAddIntegerSetting(L"UseColorImmersiveProcesses", L"1");
    PhpAddIntegerSetting(L"ColorImmersiveProcesses", L"cbc0ff"); // Pink
    PhpAddIntegerSetting(L"UseColorPicoProcesses", L"1");
    PhpAddIntegerSetting(L"ColorPicoProcesses", L"ff8000"); // Blue
    PhpAddIntegerSetting(L"UseColorSuspended", L"1");
    PhpAddIntegerSetting(L"ColorSuspended", L"777777");
    PhpAddIntegerSetting(L"UseColorDotNet", L"1");
    PhpAddIntegerSetting(L"ColorDotNet", L"00ffde");
    PhpAddIntegerSetting(L"UseColorPacked", L"1");
    PhpAddIntegerSetting(L"ColorPacked", L"9314ff"); // Deep Pink
    PhpAddIntegerSetting(L"UseColorGuiThreads", L"1");
    PhpAddIntegerSetting(L"ColorGuiThreads", L"77ffff");
    PhpAddIntegerSetting(L"UseColorRelocatedModules", L"1");
    PhpAddIntegerSetting(L"ColorRelocatedModules", L"80c0ff");
    PhpAddIntegerSetting(L"UseColorProtectedHandles", L"1");
    PhpAddIntegerSetting(L"ColorProtectedHandles", L"777777");
    PhpAddIntegerSetting(L"UseColorInheritHandles", L"1");
    PhpAddIntegerSetting(L"ColorInheritHandles", L"ffff77");
    PhpAddIntegerSetting(L"UseColorServiceDisabled", L"1");
    PhpAddIntegerSetting(L"ColorServiceDisabled", L"6d6d6d"); // Dark grey
    PhpAddIntegerSetting(L"UseColorServiceStop", L"1");
    PhpAddIntegerSetting(L"ColorServiceStop", L"6d6d6d"); // Dark grey
    PhpAddIntegerSetting(L"UseColorUnknown", L"1");
    PhpAddIntegerSetting(L"ColorUnknown", L"8080ff"); // Light Red

    PhpAddIntegerSetting(L"UseColorSystemThreadStack", L"0");
    PhpAddIntegerSetting(L"ColorSystemThreadStack", L"ffccaa");
    PhpAddIntegerSetting(L"UseColorUserThreadStack", L"0");
    PhpAddIntegerSetting(L"ColorUserThreadStack", L"aaffff");

    PhpAddIntegerSetting(L"GraphShowText", L"1");
    PhpAddIntegerSetting(L"GraphColorMode", L"0");
    PhpAddIntegerSetting(L"ColorCpuKernel", L"00ff00");
    PhpAddIntegerSetting(L"ColorCpuUser", L"0000ff");
    PhpAddIntegerSetting(L"ColorIoReadOther", L"00ffff");
    PhpAddIntegerSetting(L"ColorIoWrite", L"ff0077");
    PhpAddIntegerSetting(L"ColorPrivate", L"0077ff");
    PhpAddIntegerSetting(L"ColorPhysical", L"ff8000"); // Blue
}