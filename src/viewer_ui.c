#include "app.h"

void ClearWindowIcons(HWND hwnd) {
  if (g_windowIconSmall != NULL) {
    DestroyIcon(g_windowIconSmall);
    g_windowIconSmall = NULL;
  }
  if (g_windowIconLarge != NULL) {
    DestroyIcon(g_windowIconLarge);
    g_windowIconLarge = NULL;
  }

  if (hwnd != NULL) {
    SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)NULL);
    SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)NULL);
  }
}

HICON CreateIconFromBitmap(HBITMAP sourceBitmap, int width, int height) {
  BITMAP sourceInfo;
  HDC screenDc;
  HDC srcDc;
  HDC dstDc;
  HBITMAP colorBitmap;
  HBITMAP maskBitmap;
  HGDIOBJ oldSrc;
  HGDIOBJ oldDst;
  ICONINFO iconInfo;
  BITMAPINFO bmi;
  void* dibPixels;
  HICON icon = NULL;

  if (sourceBitmap == NULL || width <= 0 || height <= 0) {
    return NULL;
  }

  if (GetObjectA(sourceBitmap, sizeof(sourceInfo), &sourceInfo) == 0 ||
      sourceInfo.bmWidth <= 0 || sourceInfo.bmHeight <= 0) {
    return NULL;
  }

  screenDc = GetDC(NULL);
  if (screenDc == NULL) {
    return NULL;
  }

  srcDc = CreateCompatibleDC(screenDc);
  dstDc = CreateCompatibleDC(screenDc);
  if (srcDc == NULL || dstDc == NULL) {
    if (srcDc != NULL) DeleteDC(srcDc);
    if (dstDc != NULL) DeleteDC(dstDc);
    ReleaseDC(NULL, screenDc);
    return NULL;
  }

  ZeroMemory(&bmi, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  colorBitmap = CreateDIBSection(screenDc, &bmi, DIB_RGB_COLORS, &dibPixels, NULL, 0);
  maskBitmap = CreateBitmap(width, height, 1, 1, NULL);

  if (colorBitmap != NULL && maskBitmap != NULL) {
    oldSrc = SelectObject(srcDc, sourceBitmap);
    oldDst = SelectObject(dstDc, colorBitmap);

    SetStretchBltMode(dstDc, HALFTONE);
    StretchBlt(dstDc, 0, 0, width, height, srcDc, 0, 0, sourceInfo.bmWidth, sourceInfo.bmHeight, SRCCOPY);

    SelectObject(srcDc, oldSrc);
    SelectObject(dstDc, oldDst);

    ZeroMemory(&iconInfo, sizeof(iconInfo));
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = colorBitmap;
    iconInfo.hbmMask = maskBitmap;
    icon = CreateIconIndirect(&iconInfo);
  }

  if (colorBitmap != NULL) DeleteObject(colorBitmap);
  if (maskBitmap != NULL) DeleteObject(maskBitmap);

  DeleteDC(srcDc);
  DeleteDC(dstDc);
  ReleaseDC(NULL, screenDc);
  return icon;
}

void UpdateWindowIcons(HWND hwnd) {
  int smallW;
  int smallH;
  int largeW;
  int largeH;
  HICON smallIcon;
  HICON largeIcon;

  if (hwnd == NULL) {
    return;
  }

  if (g_imageBitmap == NULL) {
    ClearWindowIcons(hwnd);
    return;
  }

  smallW = GetSystemMetrics(SM_CXSMICON);
  smallH = GetSystemMetrics(SM_CYSMICON);
  largeW = GetSystemMetrics(SM_CXICON);
  largeH = GetSystemMetrics(SM_CYICON);

  smallIcon = CreateIconFromBitmap(g_imageBitmap, smallW, smallH);
  largeIcon = CreateIconFromBitmap(g_imageBitmap, largeW, largeH);

  if (smallIcon == NULL && largeIcon == NULL) {
    return;
  }

  ClearWindowIcons(NULL);
  g_windowIconSmall = smallIcon;
  g_windowIconLarge = largeIcon;

  SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_windowIconSmall);
  SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_windowIconLarge);
}

void UpdateFullscreenMenuSelection(HWND hwnd) {
  HMENU menu = GetMenu(hwnd);
  if (menu != NULL) {
    CheckMenuItem(menu, IDM_VIEW_FULLSCREEN, MF_BYCOMMAND | (g_isFullscreen ? MF_CHECKED : MF_UNCHECKED));
  }
}

void ToggleFullscreen(HWND hwnd) {
  MONITORINFO monitorInfo;

  if (!g_isFullscreen) {
    monitorInfo.cbSize = sizeof(monitorInfo);
    g_windowedStyle = (DWORD)GetWindowLongPtrA(hwnd, GWL_STYLE);
    g_windowedExStyle = (DWORD)GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    GetWindowPlacement(hwnd, &g_windowPlacement);

    if (GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitorInfo)) {
      SetMenu(hwnd, NULL);
      SetWindowLongPtrA(hwnd, GWL_STYLE, g_windowedStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU));
      SetWindowLongPtrA(
          hwnd,
          GWL_EXSTYLE,
          g_windowedExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

      SetWindowPos(
          hwnd,
          HWND_TOPMOST,
          monitorInfo.rcMonitor.left,
          monitorInfo.rcMonitor.top,
          monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
          monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
          SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
      g_isFullscreen = TRUE;
    }
  } else {
    SetWindowLongPtrA(hwnd, GWL_STYLE, g_windowedStyle);
    SetWindowLongPtrA(hwnd, GWL_EXSTYLE, g_windowedExStyle);
    SetMenu(hwnd, g_menuBar);
    SetWindowPlacement(hwnd, &g_windowPlacement);
    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    g_isFullscreen = FALSE;
  }

  UpdateFullscreenMenuSelection(hwnd);
  InvalidateRect(hwnd, NULL, TRUE);
}

void UpdateZoomMenuSelection(HWND hwnd) {
  HMENU menu = GetMenu(hwnd);
  UINT checkedId = g_zoomMode == ZOOM_MODE_ORIGINAL ? IDM_ZOOM_ORIGINAL : IDM_ZOOM_FIT;
  if (menu != NULL) {
    CheckMenuRadioItem(menu, IDM_ZOOM_FIT, IDM_ZOOM_ORIGINAL, checkedId, MF_BYCOMMAND);
  }
}

void UpdateSlideshowMenuSelection(HWND hwnd) {
  HMENU menu = GetMenu(hwnd);
  UINT checkedId = IDM_SLIDESHOW_OFF;
  if (g_slideshowSeconds == 1) {
    checkedId = IDM_SLIDESHOW_1;
  } else if (g_slideshowSeconds == 3) {
    checkedId = IDM_SLIDESHOW_3;
  } else if (g_slideshowSeconds == 10) {
    checkedId = IDM_SLIDESHOW_10;
  } else if (g_slideshowSeconds == 30) {
    checkedId = IDM_SLIDESHOW_30;
  }

  if (menu != NULL) {
    CheckMenuRadioItem(menu, IDM_SLIDESHOW_OFF, IDM_SLIDESHOW_30, checkedId, MF_BYCOMMAND);
  }
}

void UpdateSlideshowOrderMenuSelection(HWND hwnd) {
  HMENU menu = GetMenu(hwnd);
  UINT checkedId = g_slideshowOrder == SLIDESHOW_ORDER_RANDOM
                       ? IDM_SLIDESHOW_ORDER_RANDOM
                       : IDM_SLIDESHOW_ORDER_ALPHABETICAL;

  if (menu != NULL) {
    CheckMenuRadioItem(
        menu,
        IDM_SLIDESHOW_ORDER_ALPHABETICAL,
        IDM_SLIDESHOW_ORDER_RANDOM,
        checkedId,
        MF_BYCOMMAND);
  }
}

void ApplySlideshowSetting(HWND hwnd, int seconds) {
  g_slideshowSeconds = seconds;
  KillTimer(hwnd, TIMER_ID_SLIDESHOW);
  if (g_slideshowSeconds > 0) {
    SetTimer(hwnd, TIMER_ID_SLIDESHOW, (UINT)g_slideshowSeconds * 1000U, NULL);
  }
  UpdateSlideshowMenuSelection(hwnd);
}

static int ClampToInt(double value) {
  if (value > 2147483647.0) {
    return 2147483647;
  }
  if (value < 0.0) {
    return 0;
  }
  return (int)value;
}

void PaintFallbackText(HDC hdc, RECT clientRect) {
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(230, 230, 230));
  DrawTextA(hdc, "Failed to load image", -1, &clientRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void PaintImage(HDC hdc, RECT clientRect) {
  BITMAP bmp;
  if (GetObjectA(g_imageBitmap, sizeof(bmp), &bmp) == 0 || bmp.bmWidth <= 0 || bmp.bmHeight <= 0) {
    PaintFallbackText(hdc, clientRect);
    return;
  }

  HDC memDc = CreateCompatibleDC(hdc);
  if (memDc == NULL) {
    PaintFallbackText(hdc, clientRect);
    return;
  }

  HGDIOBJ oldBitmap = SelectObject(memDc, g_imageBitmap);

  const int clientWidth = clientRect.right - clientRect.left;
  const int clientHeight = clientRect.bottom - clientRect.top;
  int drawWidth;
  int drawHeight;

  if (g_zoomMode == ZOOM_MODE_ORIGINAL) {
    drawWidth = bmp.bmWidth;
    drawHeight = bmp.bmHeight;
  } else {
    const double scaleX = (double)clientWidth / (double)bmp.bmWidth;
    const double scaleY = (double)clientHeight / (double)bmp.bmHeight;
    const double scale = scaleX < scaleY ? scaleX : scaleY;
    drawWidth = ClampToInt((double)bmp.bmWidth * scale);
    drawHeight = ClampToInt((double)bmp.bmHeight * scale);
  }

  const int drawX = (clientWidth - drawWidth) / 2;
  const int drawY = (clientHeight - drawHeight) / 2;

  SetStretchBltMode(hdc, HALFTONE);
  StretchBlt(hdc, drawX, drawY, drawWidth, drawHeight, memDc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

  SelectObject(memDc, oldBitmap);
  DeleteDC(memDc);
}

void OpenImageFromDialog(HWND hwnd) {
  WCHAR filePath[MAX_PATH] = L"";
  OPENFILENAMEW ofn;

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = filePath;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrFilter =
      L"Image Files (*.jpg;*.jpeg;*.bmp;*.png;*.webp;*.gif;*.tif;*.tiff)\0*.jpg;*.jpeg;*.bmp;*.png;*.webp;*.gif;*.tif;*.tiff\0"
      L"All Files (*.*)\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

  if (GetOpenFileNameW(&ofn)) {
    LoadSelectionFromPath(hwnd, filePath);
  }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  (void)lParam;

  switch (uMsg) {
    case WM_ERASEBKGND:
      return 1;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDM_FILE_OPEN) {
        OpenImageFromDialog(hwnd);
        return 0;
      }
      if (LOWORD(wParam) == IDM_FILE_EXIT) {
        DestroyWindow(hwnd);
        return 0;
      }
      if (LOWORD(wParam) == IDM_EDIT_COPY) {
        CopyCurrentImageToClipboard(hwnd);
        return 0;
      }
      if (LOWORD(wParam) == IDM_EDIT_COPY_PATH) {
        CopyCurrentPathToClipboard(hwnd);
        return 0;
      }
      if (LOWORD(wParam) == IDM_ZOOM_FIT) {
        g_zoomMode = ZOOM_MODE_FIT;
        UpdateZoomMenuSelection(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
      }
      if (LOWORD(wParam) == IDM_ZOOM_ORIGINAL) {
        g_zoomMode = ZOOM_MODE_ORIGINAL;
        UpdateZoomMenuSelection(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
      }
      if (LOWORD(wParam) == IDM_SLIDESHOW_OFF) {
        ApplySlideshowSetting(hwnd, 0);
        return 0;
      }
      if (LOWORD(wParam) == IDM_SLIDESHOW_1) {
        ApplySlideshowSetting(hwnd, 1);
        return 0;
      }
      if (LOWORD(wParam) == IDM_SLIDESHOW_3) {
        ApplySlideshowSetting(hwnd, 3);
        return 0;
      }
      if (LOWORD(wParam) == IDM_SLIDESHOW_10) {
        ApplySlideshowSetting(hwnd, 10);
        return 0;
      }
      if (LOWORD(wParam) == IDM_SLIDESHOW_30) {
        ApplySlideshowSetting(hwnd, 30);
        return 0;
      }
      if (LOWORD(wParam) == IDM_SLIDESHOW_ORDER_ALPHABETICAL) {
        g_slideshowOrder = SLIDESHOW_ORDER_ALPHABETICAL;
        UpdateSlideshowOrderMenuSelection(hwnd);
        return 0;
      }
      if (LOWORD(wParam) == IDM_SLIDESHOW_ORDER_RANDOM) {
        g_slideshowOrder = SLIDESHOW_ORDER_RANDOM;
        UpdateSlideshowOrderMenuSelection(hwnd);
        return 0;
      }
      if (LOWORD(wParam) == IDM_VIEW_FULLSCREEN) {
        ToggleFullscreen(hwnd);
        return 0;
      }
      break;
    case WM_TIMER:
      if (wParam == TIMER_ID_SLIDESHOW) {
        NavigateSlideshowNext(hwnd);
        return 0;
      }
      break;
    case WM_KEYDOWN:
      if (wParam == VK_LEFT) {
        NavigateImage(hwnd, -1);
        return 0;
      }
      if (wParam == VK_RIGHT) {
        NavigateImage(hwnd, 1);
        return 0;
      }
      if (wParam == 'Z') {
        g_zoomMode = (g_zoomMode == ZOOM_MODE_FIT) ? ZOOM_MODE_ORIGINAL : ZOOM_MODE_FIT;
        UpdateZoomMenuSelection(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
      }
      if (wParam == 'F') {
        ToggleFullscreen(hwnd);
        return 0;
      }
      break;
    case WM_SIZE:
      InvalidateRect(hwnd, NULL, TRUE);
      return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      RECT clientRect;
      HDC backDc;
      HBITMAP backBitmap;
      HGDIOBJ oldBackBitmap;
      int width;
      int height;
      GetClientRect(hwnd, &clientRect);

      width = clientRect.right - clientRect.left;
      height = clientRect.bottom - clientRect.top;
      backDc = CreateCompatibleDC(hdc);
      backBitmap = CreateCompatibleBitmap(hdc, width > 0 ? width : 1, height > 0 ? height : 1);

      if (backDc != NULL && backBitmap != NULL) {
        oldBackBitmap = SelectObject(backDc, backBitmap);

        FillRect(backDc, &clientRect, g_backgroundBrush);
        if (g_imageBitmap != NULL) {
          PaintImage(backDc, clientRect);
        } else if (g_currentImagePath[0] != L'\0') {
          PaintFallbackText(backDc, clientRect);
        }

        BitBlt(hdc, 0, 0, width, height, backDc, 0, 0, SRCCOPY);

        SelectObject(backDc, oldBackBitmap);
      } else {
        FillRect(hdc, &clientRect, g_backgroundBrush);
        if (g_imageBitmap != NULL) {
          PaintImage(hdc, clientRect);
        } else {
          PaintFallbackText(hdc, clientRect);
        }
      }

      if (backBitmap != NULL) {
        DeleteObject(backBitmap);
      }
      if (backDc != NULL) {
        DeleteDC(backDc);
      }

      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  }

  return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
