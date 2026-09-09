#include "app.h"

HBRUSH g_backgroundBrush = NULL;
HBITMAP g_imageBitmap = NULL;
char g_windowTitle[MAX_PATH] = "Image Viewer";
WCHAR g_currentImagePath[MAX_PATH] = L"";
HICON g_windowIconSmall = NULL;
HICON g_windowIconLarge = NULL;
LPWSTR* g_imagePaths = NULL;
int g_imageCount = 0;
int g_currentImageIndex = -1;
int g_zoomMode = ZOOM_MODE_FIT;
int g_slideshowSeconds = 0;
int g_slideshowOrder = SLIDESHOW_ORDER_ALPHABETICAL;
BOOL g_isFullscreen = FALSE;
HMENU g_menuBar = NULL;
WINDOWPLACEMENT g_windowPlacement = {sizeof(WINDOWPLACEMENT)};
DWORD g_windowedStyle = 0;
DWORD g_windowedExStyle = 0;
