#include <Windows.h>
#include <wrl.h>
#include <WebView2.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include <vector>

using namespace Microsoft::WRL;
ComPtr<ICoreWebView2Environment6> env6;
ComPtr<ICoreWebView2PrintSettings> printSettings;
std::vector<ComPtr<ICoreWebView2Controller>> ctrls;
std::vector<ComPtr<ICoreWebView2>> webviews;
RECT bounds{ 0,0,1,1 };

HWND hwnd;



void loadUrl() {
    env6->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [](HRESULT result, ICoreWebView2Controller* controller) {
            ctrls.push_back(controller);
            ComPtr<ICoreWebView2> webview;
            controller->get_CoreWebView2(&webview);
            controller->put_Bounds(bounds);
            webviews.push_back(webview);
            webview->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>([](ICoreWebView2* wv, ICoreWebView2NavigationCompletedEventArgs* arg) {                
                LPWSTR rawUri = nullptr;
                wv->get_Source(&rawUri);
                std::wstring url(rawUri);
                CoTaskMemFree(rawUri);
                if (url == L"about:blank") {
                    return S_OK;
                }
                ComPtr<ICoreWebView2_7> webview7;
                wv->QueryInterface(IID_PPV_ARGS(&webview7));
                webview7->PrintToPdf(
                    L"D:\\1.pdf", printSettings.Get(),
                    Callback<ICoreWebView2PrintToPdfCompletedHandler>(
                        [&webview7](HRESULT errorCode, BOOL isSuccessful) {
                            if (SUCCEEDED(errorCode) && isSuccessful) {

                            }
                            else
                            {

                            }
                            webview7->Navigate(L"about:blank");
                            return S_OK;
                        })
                    .Get());
                return S_OK;

                }).Get(), NULL);
            webview->Navigate(L"https://www.baidu.com");
            return S_OK;
        }).Get());
}


void createWebView2() {
	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[](HRESULT result, ICoreWebView2Environment* env){
                env->QueryInterface(IID_PPV_ARGS(&env6));
				env6->CreatePrintSettings(&printSettings);
                printSettings->put_Orientation(COREWEBVIEW2_PRINT_ORIENTATION_LANDSCAPE);
				return S_OK;
			}).Get());
}

LRESULT CALLBACK windowMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void createWindow(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = &windowMsg;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_WINLOGO);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"WV2PDFWINDOW";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_WINLOGO);
    RegisterClassExW(&wcex);
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, wcex.lpszClassName, wcex.lpszClassName, WS_POPUP,
        -999999, -999999, 1, 1, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	createWindow(hInstance);
	createWebView2();
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}