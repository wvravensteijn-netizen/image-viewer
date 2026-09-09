#include "app.h"

static wchar_t* DuplicateWideString(LPCWSTR src) {
  size_t len = wcslen(src) + 1;
  wchar_t* dest = (wchar_t*)malloc(len * sizeof(wchar_t));
  if (dest == NULL) {
    return NULL;
  }
  wcscpy_s(dest, len, src);
  return dest;
}

void FreeImageList(void) {
  int i;
  for (i = 0; i < g_imageCount; ++i) {
    free(g_imagePaths[i]);
  }
  free(g_imagePaths);
  g_imagePaths = NULL;
  g_imageCount = 0;
  g_currentImageIndex = -1;
}

static BOOL IsSupportedImageExtension(LPCWSTR fileName) {
  LPCWSTR ext = PathFindExtensionW(fileName);
  if (ext == NULL || *ext == L'\0') {
    return FALSE;
  }

  return lstrcmpiW(ext, L".jpg") == 0 ||
         lstrcmpiW(ext, L".jpeg") == 0 ||
         lstrcmpiW(ext, L".bmp") == 0 ||
         lstrcmpiW(ext, L".png") == 0 ||
         lstrcmpiW(ext, L".webp") == 0 ||
         lstrcmpiW(ext, L".gif") == 0 ||
         lstrcmpiW(ext, L".tif") == 0 ||
         lstrcmpiW(ext, L".tiff") == 0;
}

static int __cdecl CompareImagePaths(const void* left, const void* right) {
  const LPCWSTR leftPath = *(const LPCWSTR*)left;
  const LPCWSTR rightPath = *(const LPCWSTR*)right;
  return StrCmpLogicalW(PathFindFileNameW(leftPath), PathFindFileNameW(rightPath));
}

static BOOL AddImagePath(LPCWSTR fullPath) {
  LPWSTR* next = (LPWSTR*)realloc(g_imagePaths, (g_imageCount + 1) * sizeof(LPWSTR));
  if (next == NULL) {
    return FALSE;
  }
  g_imagePaths = next;
  g_imagePaths[g_imageCount] = DuplicateWideString(fullPath);
  if (g_imagePaths[g_imageCount] == NULL) {
    return FALSE;
  }
  g_imageCount++;
  return TRUE;
}

static void SetTitleFromPath(LPCWSTR path) {
  LPCWSTR fileName = PathFindFileNameW(path);
  if (fileName == NULL || *fileName == L'\0') {
    fileName = path;
  }

  if (WideCharToMultiByte(CP_ACP, 0, fileName, -1, g_windowTitle, MAX_PATH, NULL, NULL) == 0) {
    strcpy_s(g_windowTitle, MAX_PATH, "Image Viewer");
  }
}

static BOOL BuildImageListFromSelectedPath(LPCWSTR selectedPath) {
  WIN32_FIND_DATAW findData;
  HANDLE findHandle = INVALID_HANDLE_VALUE;
  WCHAR selectedFull[MAX_PATH];
  WCHAR directory[MAX_PATH];
  WCHAR pattern[MAX_PATH];
  DWORD fullLen;

  FreeImageList();

  fullLen = GetFullPathNameW(selectedPath, MAX_PATH, selectedFull, NULL);
  if (fullLen == 0 || fullLen >= MAX_PATH) {
    return FALSE;
  }

  wcscpy_s(directory, MAX_PATH, selectedFull);
  if (!PathRemoveFileSpecW(directory)) {
    return FALSE;
  }

  swprintf_s(pattern, MAX_PATH, L"%s\\*", directory);
  findHandle = FindFirstFileW(pattern, &findData);
  if (findHandle == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  do {
    WCHAR candidate[MAX_PATH];
    WCHAR candidateFull[MAX_PATH];
    DWORD candidateLen;

    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      continue;
    }
    if (!IsSupportedImageExtension(findData.cFileName)) {
      continue;
    }

    swprintf_s(candidate, MAX_PATH, L"%s\\%s", directory, findData.cFileName);
    candidateLen = GetFullPathNameW(candidate, MAX_PATH, candidateFull, NULL);
    if (candidateLen == 0 || candidateLen >= MAX_PATH) {
      continue;
    }

    if (!AddImagePath(candidateFull)) {
      FindClose(findHandle);
      return FALSE;
    }
  } while (FindNextFileW(findHandle, &findData) != 0);

  FindClose(findHandle);

  if (g_imageCount == 0) {
    return FALSE;
  }

  qsort(g_imagePaths, g_imageCount, sizeof(LPWSTR), CompareImagePaths);

  {
    int i;
    for (i = 0; i < g_imageCount; ++i) {
      if (lstrcmpiW(g_imagePaths[i], selectedFull) == 0) {
        g_currentImageIndex = i;
        break;
      }
    }
  }

  if (g_currentImageIndex < 0) {
    g_currentImageIndex = 0;
  }

  return TRUE;
}

HBITMAP LoadImageBitmapWic(LPCWSTR filePath) {
  IWICImagingFactory* factory = NULL;
  IWICBitmapDecoder* decoder = NULL;
  IWICBitmapFrameDecode* frame = NULL;
  IWICFormatConverter* converter = NULL;
  BYTE* pixelBuffer = NULL;
  HBITMAP bitmap = NULL;

  UINT width = 0;
  UINT height = 0;
  UINT stride = 0;
  UINT bufferSize = 0;
  void* dibPixels = NULL;
  HRESULT hr = S_OK;

  hr = CoCreateInstance(
      &CLSID_WICImagingFactory,
      NULL,
      CLSCTX_INPROC_SERVER,
      &IID_IWICImagingFactory,
      (void**)&factory);
  if (FAILED(hr)) {
    goto Cleanup;
  }

  hr = factory->lpVtbl->CreateDecoderFromFilename(
      factory,
      filePath,
      NULL,
      GENERIC_READ,
      WICDecodeMetadataCacheOnLoad,
      &decoder);
  if (FAILED(hr)) {
    goto Cleanup;
  }

  hr = decoder->lpVtbl->GetFrame(decoder, 0, &frame);
  if (FAILED(hr)) {
    goto Cleanup;
  }

  hr = factory->lpVtbl->CreateFormatConverter(factory, &converter);
  if (FAILED(hr)) {
    goto Cleanup;
  }

  hr = converter->lpVtbl->Initialize(
      converter,
      (IWICBitmapSource*)frame,
      &GUID_WICPixelFormat32bppPBGRA,
      WICBitmapDitherTypeNone,
      NULL,
      0.0f,
      WICBitmapPaletteTypeCustom);
  if (FAILED(hr)) {
    goto Cleanup;
  }

  hr = ((IWICBitmapSource*)converter)->lpVtbl->GetSize((IWICBitmapSource*)converter, &width, &height);
  if (FAILED(hr) || width == 0 || height == 0) {
    goto Cleanup;
  }

  stride = width * 4;
  bufferSize = stride * height;
  pixelBuffer = (BYTE*)malloc(bufferSize);
  if (pixelBuffer == NULL) {
    goto Cleanup;
  }

  hr = ((IWICBitmapSource*)converter)->lpVtbl->CopyPixels(
      (IWICBitmapSource*)converter,
      NULL,
      stride,
      bufferSize,
      pixelBuffer);
  if (FAILED(hr)) {
    goto Cleanup;
  }

  BITMAPINFO bmi;
  ZeroMemory(&bmi, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = (LONG)width;
  bmi.bmiHeader.biHeight = -(LONG)height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  HDC screenDc = GetDC(NULL);
  bitmap = CreateDIBSection(screenDc, &bmi, DIB_RGB_COLORS, &dibPixels, NULL, 0);
  ReleaseDC(NULL, screenDc);
  if (bitmap == NULL || dibPixels == NULL) {
    bitmap = NULL;
    goto Cleanup;
  }

  memcpy(dibPixels, pixelBuffer, bufferSize);

Cleanup:
  if (pixelBuffer != NULL) {
    free(pixelBuffer);
  }
  if (converter != NULL) {
    converter->lpVtbl->Release(converter);
  }
  if (frame != NULL) {
    frame->lpVtbl->Release(frame);
  }
  if (decoder != NULL) {
    decoder->lpVtbl->Release(decoder);
  }
  if (factory != NULL) {
    factory->lpVtbl->Release(factory);
  }

  return bitmap;
}

void SetCurrentImagePath(LPCWSTR path) {
  if (path == NULL || *path == L'\0') {
    g_currentImagePath[0] = L'\0';
    return;
  }
  wcsncpy_s(g_currentImagePath, MAX_PATH, path, _TRUNCATE);
}

BOOL LoadSelectionFromPath(HWND hwnd, LPCWSTR selectedPath) {
  HBITMAP newBitmap;

  SetTitleFromPath(selectedPath);

  if (BuildImageListFromSelectedPath(selectedPath)) {
    return SetCurrentImageByIndex(hwnd, g_currentImageIndex);
  }

  newBitmap = LoadImageBitmapWic(selectedPath);
  if (g_imageBitmap != NULL) {
    DeleteObject(g_imageBitmap);
    g_imageBitmap = NULL;
  }
  g_imageBitmap = newBitmap;
  g_currentImageIndex = -1;
  SetCurrentImagePath(selectedPath);

  if (hwnd != NULL) {
    SetWindowTextA(hwnd, g_windowTitle);
    UpdateWindowIcons(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
  }

  return g_imageBitmap != NULL;
}

BOOL SetCurrentImageByIndex(HWND hwnd, int index) {
  HBITMAP newBitmap;

  if (index < 0 || index >= g_imageCount) {
    return FALSE;
  }

  newBitmap = LoadImageBitmapWic(g_imagePaths[index]);
  if (g_imageBitmap != NULL) {
    DeleteObject(g_imageBitmap);
    g_imageBitmap = NULL;
  }
  g_imageBitmap = newBitmap;
  g_currentImageIndex = index;
  SetCurrentImagePath(g_imagePaths[index]);
  SetTitleFromPath(g_imagePaths[index]);

  if (hwnd != NULL) {
    SetWindowTextA(hwnd, g_windowTitle);
    UpdateWindowIcons(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
  }

  return g_imageBitmap != NULL;
}

void NavigateImage(HWND hwnd, int direction) {
  int nextIndex;

  if (g_imageCount <= 1 || g_currentImageIndex < 0) {
    return;
  }

  nextIndex = (g_currentImageIndex + direction + g_imageCount) % g_imageCount;
  SetCurrentImageByIndex(hwnd, nextIndex);
}

void NavigateSlideshowNext(HWND hwnd) {
  int nextIndex;

  if (g_imageCount <= 1 || g_currentImageIndex < 0) {
    return;
  }

  if (g_slideshowOrder == SLIDESHOW_ORDER_RANDOM) {
    nextIndex = g_currentImageIndex;
    while (nextIndex == g_currentImageIndex) {
      nextIndex = rand() % g_imageCount;
    }
    SetCurrentImageByIndex(hwnd, nextIndex);
    return;
  }

  NavigateImage(hwnd, 1);
}

HGLOBAL CreateClipboardDibFromBitmap(HBITMAP bitmap) {
  BITMAP bmp;
  BITMAPINFOHEADER* header;
  BYTE* pixelData;
  HGLOBAL hDib;
  HDC screenDc;
  LONG stride;
  DWORD imageSize;

  if (GetObjectA(bitmap, sizeof(bmp), &bmp) == 0 || bmp.bmWidth <= 0 || bmp.bmHeight <= 0) {
    return NULL;
  }

  stride = ((bmp.bmWidth * 32 + 31) / 32) * 4;
  imageSize = (DWORD)(stride * bmp.bmHeight);

  hDib = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + imageSize);
  if (hDib == NULL) {
    return NULL;
  }

  header = (BITMAPINFOHEADER*)GlobalLock(hDib);
  if (header == NULL) {
    GlobalFree(hDib);
    return NULL;
  }

  ZeroMemory(header, sizeof(BITMAPINFOHEADER));
  header->biSize = sizeof(BITMAPINFOHEADER);
  header->biWidth = bmp.bmWidth;
  header->biHeight = bmp.bmHeight;
  header->biPlanes = 1;
  header->biBitCount = 32;
  header->biCompression = BI_RGB;
  header->biSizeImage = imageSize;

  pixelData = (BYTE*)(header + 1);
  screenDc = GetDC(NULL);
  if (screenDc == NULL) {
    GlobalUnlock(hDib);
    GlobalFree(hDib);
    return NULL;
  }

  if (GetDIBits(screenDc, bitmap, 0, (UINT)bmp.bmHeight, pixelData, (BITMAPINFO*)header, DIB_RGB_COLORS) == 0) {
    ReleaseDC(NULL, screenDc);
    GlobalUnlock(hDib);
    GlobalFree(hDib);
    return NULL;
  }

  ReleaseDC(NULL, screenDc);
  GlobalUnlock(hDib);
  return hDib;
}

BOOL CopyCurrentImageToClipboard(HWND hwnd) {
  HBITMAP clipboardBitmap;
  HGLOBAL clipboardDib;
  BOOL copied = FALSE;

  if (g_imageBitmap == NULL) {
    return FALSE;
  }

  clipboardBitmap = (HBITMAP)CopyImage(g_imageBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  clipboardDib = CreateClipboardDibFromBitmap(g_imageBitmap);
  if (clipboardBitmap == NULL && clipboardDib == NULL) {
    return FALSE;
  }

  if (!OpenClipboard(hwnd)) {
    if (clipboardBitmap != NULL) {
      DeleteObject(clipboardBitmap);
    }
    if (clipboardDib != NULL) {
      GlobalFree(clipboardDib);
    }
    return FALSE;
  }

  EmptyClipboard();

  if (clipboardDib != NULL) {
    if (SetClipboardData(CF_DIB, clipboardDib) != NULL) {
      copied = TRUE;
      clipboardDib = NULL;
    }
  }

  if (clipboardBitmap != NULL) {
    if (SetClipboardData(CF_BITMAP, clipboardBitmap) != NULL) {
      copied = TRUE;
      clipboardBitmap = NULL;
    }
  }

  if (clipboardBitmap != NULL) {
    DeleteObject(clipboardBitmap);
  }
  if (clipboardDib != NULL) {
    GlobalFree(clipboardDib);
  }

  CloseClipboard();
  return copied;
}

BOOL CopyCurrentPathToClipboard(HWND hwnd) {
  SIZE_T byteCount;
  HGLOBAL clipboardBuffer;
  WCHAR* bufferData;

  if (g_currentImagePath[0] == L'\0') {
    return FALSE;
  }

  byteCount = (wcslen(g_currentImagePath) + 1) * sizeof(WCHAR);
  clipboardBuffer = GlobalAlloc(GMEM_MOVEABLE, byteCount);
  if (clipboardBuffer == NULL) {
    return FALSE;
  }

  bufferData = (WCHAR*)GlobalLock(clipboardBuffer);
  if (bufferData == NULL) {
    GlobalFree(clipboardBuffer);
    return FALSE;
  }
  memcpy(bufferData, g_currentImagePath, byteCount);
  GlobalUnlock(clipboardBuffer);

  if (!OpenClipboard(hwnd)) {
    GlobalFree(clipboardBuffer);
    return FALSE;
  }

  EmptyClipboard();
  if (SetClipboardData(CF_UNICODETEXT, clipboardBuffer) == NULL) {
    GlobalFree(clipboardBuffer);
    CloseClipboard();
    return FALSE;
  }

  CloseClipboard();
  return TRUE;
}
