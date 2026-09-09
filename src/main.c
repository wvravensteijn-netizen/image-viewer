#include "app.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd) {
  (void)prevInstance;
  (void)cmdLine;

  // HRESULT comResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  // BOOL shouldUninitializeCom = SUCCEEDED(comResult);

  g_backgroundBrush = CreateSolidBrush(RGB(0x1d, 0x1d, 0x1d));

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv != NULL && argc > 1) {
    LoadSelectionFromPath(NULL, argv[1]);
  }
  if (argv != NULL) {
    LocalFree(argv);
  }

  WNDCLASSEXA wc;
  ZeroMemory(&wc, sizeof(wc));
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = instance;
  wc.lpszClassName = "ImageViewerWindowClass";
  wc.hbrBackground = g_backgroundBrush;
  wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

  if (RegisterClassExA(&wc) == 0) {
    if (g_imageBitmap != NULL) {
      DeleteObject(g_imageBitmap);
    }
    DeleteObject(g_backgroundBrush);
    return 1;
  }

  HMENU menuBar = CreateMenu();
  HMENU fileMenu = CreatePopupMenu();
  HMENU editMenu = CreatePopupMenu();
  HMENU viewMenu = CreatePopupMenu();
  HMENU zoomMenu = CreatePopupMenu();
  HMENU slideshowMenu = CreatePopupMenu();
  HACCEL accelTable = NULL;

  if (menuBar == NULL || fileMenu == NULL || editMenu == NULL || viewMenu == NULL || zoomMenu == NULL || slideshowMenu == NULL) {
    if (slideshowMenu != NULL) DestroyMenu(slideshowMenu);
    if (zoomMenu != NULL) DestroyMenu(zoomMenu);
    if (viewMenu != NULL) DestroyMenu(viewMenu);
    if (fileMenu != NULL) DestroyMenu(fileMenu);
    if (editMenu != NULL) DestroyMenu(editMenu);
    if (menuBar != NULL) DestroyMenu(menuBar);
    if (g_imageBitmap != NULL) DeleteObject(g_imageBitmap);
    FreeImageList();
    DeleteObject(g_backgroundBrush);
    return 1;
  }

  AppendMenuA(fileMenu, MF_STRING, IDM_FILE_OPEN, "&Open...\tCtrl+O");
  AppendMenuA(fileMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuA(fileMenu, MF_STRING, IDM_FILE_EXIT, "E&xit\tAlt+F4");
  AppendMenuA(editMenu, MF_STRING, IDM_EDIT_COPY, "&Copy\tCtrl+C");
  AppendMenuA(editMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuA(editMenu, MF_STRING, IDM_EDIT_COPY_PATH, "Copy &Path");
  AppendMenuA(zoomMenu, MF_STRING, IDM_ZOOM_FIT, "&Fit");
  AppendMenuA(zoomMenu, MF_STRING, IDM_ZOOM_ORIGINAL, "&Original");
  AppendMenuA(slideshowMenu, MF_STRING, IDM_SLIDESHOW_OFF, "&Off");
  AppendMenuA(slideshowMenu, MF_STRING, IDM_SLIDESHOW_1, "&1 sec.");
  AppendMenuA(slideshowMenu, MF_STRING, IDM_SLIDESHOW_3, "&3 sec.");
  AppendMenuA(slideshowMenu, MF_STRING, IDM_SLIDESHOW_10, "1&0 sec.");
  AppendMenuA(slideshowMenu, MF_STRING, IDM_SLIDESHOW_30, "3&0 sec.");
  AppendMenuA(slideshowMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuA(slideshowMenu, MF_STRING, IDM_SLIDESHOW_ORDER_ALPHABETICAL, "&Alphabetical");
  AppendMenuA(slideshowMenu, MF_STRING, IDM_SLIDESHOW_ORDER_RANDOM, "&Random");
  AppendMenuA(viewMenu, MF_POPUP, (UINT_PTR)zoomMenu, "&Zoom");
  AppendMenuA(viewMenu, MF_POPUP, (UINT_PTR)slideshowMenu, "&Slideshow");
  AppendMenuA(viewMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuA(viewMenu, MF_STRING, IDM_VIEW_FULLSCREEN, "&Fullscreen\tF");
  AppendMenuA(menuBar, MF_POPUP, (UINT_PTR)fileMenu, "&File");
  AppendMenuA(menuBar, MF_POPUP, (UINT_PTR)editMenu, "&Edit");
  AppendMenuA(menuBar, MF_POPUP, (UINT_PTR)viewMenu, "&View");

  g_menuBar = menuBar;

  {
    ACCEL accel[2];
    accel[0].fVirt = FVIRTKEY | FALT;
    accel[0].key = VK_F4;
    accel[0].cmd = IDM_FILE_EXIT;
    accel[1].fVirt = FVIRTKEY | FCONTROL;
    accel[1].key = 'C';
    accel[1].cmd = IDM_EDIT_COPY;
    accelTable = CreateAcceleratorTableA(accel, 2);
  }

  HWND hwnd = CreateWindowExA(
      0,
      wc.lpszClassName,
      g_windowTitle,
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      960,
      640,
      NULL,
      menuBar,
      instance,
      NULL);

  if (hwnd == NULL) {
    DestroyMenu(menuBar);
    if (g_imageBitmap != NULL) DeleteObject(g_imageBitmap);
    DeleteObject(g_backgroundBrush);
    return 1;
  }

  UpdateZoomMenuSelection(hwnd);
  UpdateSlideshowMenuSelection(hwnd);
  UpdateSlideshowOrderMenuSelection(hwnd);
  UpdateFullscreenMenuSelection(hwnd);
  UpdateWindowIcons(hwnd);

  ShowWindow(hwnd, showCmd);
  UpdateWindow(hwnd);

  srand((unsigned int)GetTickCount());

  MSG msg;
  while (GetMessageA(&msg, NULL, 0, 0) > 0) {
    if (accelTable == NULL || TranslateAcceleratorA(hwnd, accelTable, &msg) == 0) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
  }

  if (g_imageBitmap != NULL) DeleteObject(g_imageBitmap);
  ClearWindowIcons(NULL);
  FreeImageList();
  DeleteObject(g_backgroundBrush);
  if (accelTable != NULL) DestroyAcceleratorTable(accelTable);
  return (int)msg.wParam;
}

