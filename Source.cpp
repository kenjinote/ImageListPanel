#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "gdiplus")

#include <windows.h>
#include <gdiplus.h>
#include <list>

TCHAR szClassName[] = TEXT("Window");
using namespace Gdiplus;

class ImageListPanel
{
	BOOL m_bDrag;
	int m_nDragIndex;
	int m_nSplitPrevIndex;
	int m_nSplitPrevPosY;
	int m_nMargin;
	int m_nImageMaxCount;
	std::list<Bitmap*> m_listBitmap;
	WNDPROC fnWndProc;
public:
	HWND m_hWnd;
	ImageListPanel(int nImageMaxCount, DWORD dwStyle, int x, int y, int width, int height, HWND hParent)
		: m_nImageMaxCount(nImageMaxCount)
		, m_hWnd(0)
		, fnWndProc(0)
		, m_nMargin(4)
		, m_bDrag(0)
		, m_nSplitPrevIndex(-1)
		, m_nSplitPrevPosY(0)
	{
		WNDCLASSW wndclass = { 0,WndProc,0,0,GetModuleHandle(0),0,LoadCursor(0,IDC_ARROW),(HBRUSH)(COLOR_WINDOW + 1),0,__FUNCTIONW__ };
		RegisterClassW(&wndclass);
		m_hWnd = CreateWindowW(__FUNCTIONW__, 0, dwStyle, x, y, width, height, hParent, 0, GetModuleHandle(0), this);
	}
	~ImageListPanel()
	{
		for (auto &bitmap : m_listBitmap) {
			delete bitmap;
			bitmap = 0;
		}
	}
	BOOL MoveImage(int nIndexFrom, int nIndexTo)
	{
		if (nIndexFrom < 0) nIndexFrom = 0;
		if (nIndexTo < 0) nIndexTo = 0;
		if (nIndexFrom == nIndexTo) return FALSE;
		std::list<Bitmap*>::iterator itFrom = m_listBitmap.begin();
		std::list<Bitmap*>::iterator itTo = m_listBitmap.begin();
		std::advance(itFrom, nIndexFrom);
		std::advance(itTo, nIndexTo);
		m_listBitmap.splice(itTo, m_listBitmap, itFrom);
		return TRUE;
	}
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (msg == WM_NCCREATE) {
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
			return TRUE;
		}
		ImageListPanel* _this = (ImageListPanel*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (_this) {
			switch (msg) {
				case WM_CREATE:
					DragAcceptFiles(hWnd, TRUE);
					return 0;
				case WM_DROPFILES:
					{
						HDROP hDrop = (HDROP)wParam;
						TCHAR szFileName[MAX_PATH];
						UINT iFile, nFiles;
						nFiles = DragQueryFile((HDROP)hDrop, 0xFFFFFFFF, NULL, 0);
						BOOL bUpdate = FALSE;
						for (iFile = 0; iFile<nFiles; ++iFile) {
							DragQueryFile(hDrop, iFile, szFileName, sizeof(szFileName));
							Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(szFileName);
							if (pBitmap && pBitmap->GetLastStatus() == Ok) {
								if (_this->m_listBitmap.size() < _this->m_nImageMaxCount) {
									_this->m_listBitmap.push_back(pBitmap);
									bUpdate = TRUE;
								}								
							}
						}
						DragFinish(hDrop);
						if (bUpdate)
							InvalidateRect(hWnd, 0, 1);
					}
					return 0;
				case WM_PAINT:
					{
						PAINTSTRUCT ps;
						HDC hdc = BeginPaint(hWnd, &ps);
						{
							RECT rect;
							GetClientRect(hWnd, &rect);
							INT nTop = _this->m_nMargin;
							Graphics g(hdc);
							int nWidth = rect.right - 2 * _this->m_nMargin;
							Gdiplus::StringFormat f;
							f.SetAlignment(Gdiplus::StringAlignmentCenter);
							f.SetLineAlignment(Gdiplus::StringAlignmentCenter);
							if (_this->m_listBitmap.size() == 0) {
								Gdiplus::Font font(hdc, (HFONT)GetStockObject(DEFAULT_GUI_FONT));
								RectF rectf((REAL)0, (REAL)0, (REAL)rect.right, (REAL)16);
								g.DrawString(L"画像をドロップ", -1, &font, rectf, &f, &SolidBrush(Color::MakeARGB(128, 0, 0, 0)));
							} else {
								Gdiplus::Font font(&FontFamily(L"Marlett"), 11, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
								for (auto bitmap : _this->m_listBitmap) {
									int nHeight = bitmap->GetHeight() * nWidth / bitmap->GetWidth();
									g.DrawImage(bitmap, _this->m_nMargin, nTop, nWidth, nHeight);
									RectF rectf((REAL)(nWidth + _this->m_nMargin - 16), (REAL)nTop, (REAL)16, (REAL)16);
									g.FillRectangle(&SolidBrush(Color::MakeARGB(192, 255, 255, 255)), rectf);
									g.DrawString(L"r", 1, &font, rectf, &f, &SolidBrush(Color::MakeARGB(192, 0, 0, 0)));
									nTop += nHeight + _this->m_nMargin;
								}
							}
						}
						EndPaint(hWnd, &ps);
					}
					return 0;
				case WM_LBUTTONDOWN:
					{
						RECT rect;
						GetClientRect(hWnd, &rect);
						POINT point = { LOWORD(lParam), HIWORD(lParam) };
						INT nTop = _this->m_nMargin;
						int nWidth = rect.right - 2 * _this->m_nMargin;
						for (auto it = _this->m_listBitmap.begin(); it != _this->m_listBitmap.end(); ++it) {
							int nHeight = (*it)->GetHeight() * nWidth / (*it)->GetWidth();
							RECT rectCloseButton = { nWidth  + _this->m_nMargin - 16, nTop, nWidth + _this->m_nMargin, nTop + 16 };
							if (PtInRect(&rectCloseButton, point)) {
								delete *it;
								*it = 0;
								_this->m_listBitmap.erase(it);
								InvalidateRect(hWnd, 0, 1);
								return 0;
							}
							nTop += nHeight + _this->m_nMargin;
						}
						nTop = _this->m_nMargin;
						int nIndex = 0;
						for (auto it = _this->m_listBitmap.begin(); it != _this->m_listBitmap.end(); ++it) {
							int nHeight = (*it)->GetHeight() * nWidth / (*it)->GetWidth();
							RECT rectImage = { _this->m_nMargin, nTop, _this->m_nMargin + nWidth, nTop + nHeight };
							if (PtInRect(&rectImage, point)) {
								_this->m_bDrag = TRUE;
								SetCapture(hWnd);
								_this->m_nDragIndex = nIndex;
								return 0;
							}
							nTop += nHeight + _this->m_nMargin;
							++nIndex;
						}
					}
					return 0;
				case WM_MOUSEMOVE:
					if (_this->m_bDrag)
					{
						RECT rect;
						GetClientRect(hWnd, &rect);
						INT nCursorY = HIWORD(lParam);
						INT nTop = 0;
						int nWidth = rect.right - 2 * _this->m_nMargin;
						int nIndex = 0;
						for (auto it = _this->m_listBitmap.begin(); it != _this->m_listBitmap.end(); ++it) {
							int nHeight = (*it)->GetHeight() * nWidth / (*it)->GetWidth();
							RECT rectImage = { 0, nTop, rect.right, nTop + nHeight + _this->m_nMargin };
							if (nCursorY >= nTop && (nIndex + 1 == _this->m_listBitmap.size() || nCursorY < nTop + nHeight + _this->m_nMargin)) {
								int nCurrentIndex;
								int nCurrentPosY;
								if (nCursorY < nTop + nHeight / 2 + _this->m_nMargin) {
									nCurrentIndex = nIndex;
									nCurrentPosY = nTop;
								} else {
									nCurrentIndex = nIndex + 1;
									nCurrentPosY = nTop + nHeight + _this->m_nMargin;
								}
								if (nCurrentIndex != _this->m_nSplitPrevIndex) {
									HDC hdc = GetDC(hWnd);
									if(_this->m_nSplitPrevIndex != -1)
										PatBlt(hdc, 0, _this->m_nSplitPrevPosY, rect.right, _this->m_nMargin, PATINVERT);
									PatBlt(hdc, 0, nCurrentPosY, rect.right, _this->m_nMargin, PATINVERT);
									ReleaseDC(hWnd, hdc);
									_this->m_nSplitPrevIndex = nCurrentIndex;
									_this->m_nSplitPrevPosY = nCurrentPosY;
								}
								return 0;
							}
							nTop += nHeight + _this->m_nMargin;
							++nIndex;
						}
					}
					return 0;
				case WM_LBUTTONUP:
					if (_this->m_bDrag)
					{
						ReleaseCapture();
						_this->m_bDrag = FALSE;
						if (_this->m_nSplitPrevIndex != -1) {
							RECT rect;
							GetClientRect(hWnd, &rect);
							HDC hdc = GetDC(hWnd);
							PatBlt(hdc, 0, _this->m_nSplitPrevPosY, rect.right, _this->m_nMargin, PATINVERT);
							ReleaseDC(hWnd, hdc);
							if (_this->MoveImage(_this->m_nDragIndex, _this->m_nSplitPrevIndex)) {
								InvalidateRect(hWnd, 0, 1);
							}
							_this->m_nSplitPrevIndex = -1;
						}
					}
					return 0;
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static ImageListPanel *pImagePanel1;
	static ImageListPanel *pImagePanel2;
	switch (msg)
	{
	case WM_CREATE:
		pImagePanel1 = new ImageListPanel(8, WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 100, 500, hWnd);
		pImagePanel2 = new ImageListPanel(8, WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 100, 500, hWnd);
		break;
	case WM_SIZE:
		MoveWindow(pImagePanel1->m_hWnd, 0, 0, 100, 500, TRUE);
		MoveWindow(pImagePanel2->m_hWnd, 110, 0, 100, 500, TRUE);
		break;
	case WM_DESTROY:
		delete pImagePanel1;
		delete pImagePanel2;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("複数の画像ファイルをドラッグドロップで追加できるコントロール"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}
