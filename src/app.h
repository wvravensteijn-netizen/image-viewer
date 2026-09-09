#ifndef APP_H
#define APP_H

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define IDM_FILE_EXIT 1001
#define IDM_FILE_OPEN 1002
#define IDM_EDIT_COPY 1051
#define IDM_EDIT_COPY_PATH 1052
#define IDM_ZOOM_FIT 1101
#define IDM_ZOOM_ORIGINAL 1102
#define IDM_SLIDESHOW_OFF 1200
#define IDM_SLIDESHOW_1 1201
#define IDM_SLIDESHOW_3 1203
#define IDM_SLIDESHOW_10 1210
#define IDM_SLIDESHOW_30 1230
#define IDM_SLIDESHOW_ORDER_ALPHABETICAL 1241
#define IDM_SLIDESHOW_ORDER_RANDOM 1242
#define IDM_VIEW_FULLSCREEN 1300

#define TIMER_ID_SLIDESHOW 1

#define ZOOM_MODE_FIT 0
#define ZOOM_MODE_ORIGINAL 1
#define SLIDESHOW_ORDER_ALPHABETICAL 0
#define SLIDESHOW_ORDER_RANDOM 1

extern HBRUSH g_backgroundBrush;
extern HBITMAP g_imageBitmap;
extern char g_windowTitle[MAX_PATH];
extern WCHAR g_currentImagePath[MAX_PATH];
extern HICON g_windowIconSmall;
extern HICON g_windowIconLarge;
extern LPWSTR* g_imagePaths;
extern int g_imageCount;
extern int g_currentImageIndex;
extern int g_zoomMode;
extern int g_slideshowSeconds;
extern int g_slideshowOrder;
extern BOOL g_isFullscreen;
extern HMENU g_menuBar;
extern WINDOWPLACEMENT g_windowPlacement;
extern DWORD g_windowedStyle;
extern DWORD g_windowedExStyle;

void FreeImageList(void);
void ClearWindowIcons(HWND hwnd);
HICON CreateIconFromBitmap(HBITMAP sourceBitmap, int width, int height);
void UpdateWindowIcons(HWND hwnd);
void SetCurrentImagePath(LPCWSTR path);
HGLOBAL CreateClipboardDibFromBitmap(HBITMAP bitmap);
BOOL CopyCurrentImageToClipboard(HWND hwnd);
BOOL CopyCurrentPathToClipboard(HWND hwnd);
void UpdateFullscreenMenuSelection(HWND hwnd);
void ToggleFullscreen(HWND hwnd);
void UpdateZoomMenuSelection(HWND hwnd);
void UpdateSlideshowMenuSelection(HWND hwnd);
void UpdateSlideshowOrderMenuSelection(HWND hwnd);
void ApplySlideshowSetting(HWND hwnd, int seconds);
void PaintFallbackText(HDC hdc, RECT clientRect);
void PaintImage(HDC hdc, RECT clientRect);
BOOL SetCurrentImageByIndex(HWND hwnd, int index);
void NavigateImage(HWND hwnd, int direction);
void NavigateSlideshowNext(HWND hwnd);
void OpenImageFromDialog(HWND hwnd);
BOOL LoadSelectionFromPath(HWND hwnd, LPCWSTR selectedPath);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
