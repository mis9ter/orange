#include "yfilter.h"
#include "File.h"

static	CFileTable		g_file;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(NONPAGE,	FileTable)
#pragma alloc_text(PAGE,	CreateFileTable)
#pragma alloc_text(PAGE,	DestroyFileTable)
#endif

CFileTable *		FileTable()
{
	return &g_file;
}
void				CreateFileTable()
{
	g_file.Initialize(false);
}
void				DestroyFileTable()
{
	g_file.Destroy();
}