#include <Windows.h>
#include <filesystem>
#include <shlobj.h>
#include <wrl.h>
#include <WebView2.h>
#include <DispatcherQueue.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include <functional>
#include <winrt/Windows.System.h>
#include <vector>
#include <libipc/ipc.h>

using namespace Microsoft::WRL;
ComPtr<ICoreWebView2Environment6> env6;
ComPtr<ICoreWebView2PrintSettings> printSettings;
std::vector<ComPtr<ICoreWebView2Controller>> ctrls;
std::vector<ComPtr<ICoreWebView2>> webviews;
RECT bounds{ 0,0,1,1 };
std::unique_ptr<ipc::route> ipcIns;
winrt::Windows::System::DispatcherQueue dq{ winrt::Windows::System::DispatcherQueue::GetForCurrentThread() };
winrt::Windows::System::DispatcherQueueController controller{ nullptr };
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
    PWSTR pathTmp;
    auto hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &pathTmp);
    if (FAILED(hr))
    {
        CoTaskMemFree(pathTmp);
        MessageBox(NULL, L"无法得到应用数据目录", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    std::filesystem::path appDir{ pathTmp };
    CoTaskMemFree(pathTmp);
    appDir /= "WV2PDFWINDOW";
    auto dataPath = appDir.wstring();
	CreateCoreWebView2EnvironmentWithOptions(nullptr, dataPath.data(), nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[](HRESULT result, ICoreWebView2Environment* env){
                env->QueryInterface(IID_PPV_ARGS(&env6));
				env6->CreatePrintSettings(&printSettings);
                printSettings->put_Orientation(COREWEBVIEW2_PRINT_ORIENTATION_LANDSCAPE);
                ipcIns->send("");
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

winrt::fire_and_forget waitMsg()
{
    ipcIns = std::make_unique<ipc::route>("my-ipc-route", ipc::receiver);
    co_await winrt::resume_background();
    while (true) {
        auto buf = ipcIns->recv();
        auto str = static_cast<char*>(buf.data());
        if (str == nullptr || str[0] == '\0') break;
        auto msg = std::string{ str };
        co_await winrt::resume_foreground(dq);
        if (msg == "loadUrl") {
            //env6.Navigate(L"https://example.com");
        }
        else if (msg == "loadHtml") {
        }
        co_await winrt::resume_background();
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
    DispatcherQueueOptions options{
        sizeof(DispatcherQueueOptions),
        DQTYPE_THREAD_CURRENT,
        DQTAT_COM_NONE
    };
    CreateDispatcherQueueController(options,
        reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(controller)));
    waitMsg();
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