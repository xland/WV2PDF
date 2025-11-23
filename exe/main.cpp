#include <Windows.h>
#include <filesystem>
#include <shlobj.h>
#include <wrl.h>
#include <WebView2.h>
#include <thread>
#include <DispatcherQueue.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include <functional>
#include <winrt/Windows.System.h>
#include <vector>
#include <ixwebsocket/IXWebSocketServer.h>
#include <iostream>

using namespace Microsoft::WRL;
using namespace winrt::Windows::Data::Json;
ComPtr<ICoreWebView2Environment6> env6;
ComPtr<ICoreWebView2PrintSettings> printSettings;
std::vector<ComPtr<ICoreWebView2Controller>> ctrls;
std::vector<ComPtr<ICoreWebView2>> webviews;
RECT bounds{ 0,0,1,1 };
HWND hwnd;
ix::WebSocketServer* server;

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
        case WM_APP + 100:
        {
            std::string* p = reinterpret_cast<std::string*>(lParam);
            std::string url = *p;
            delete p;
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

void procMsg(const std::string& msg) {
    winrt::hstring str = winrt::to_hstring(msg);
    JsonObject param = JsonObject::Parse(str);
    auto action = param.GetNamedString(L"action");
    if (action == L"url2pdf") {
        auto url = param.GetNamedString(L"url");
        for (auto& client : server->getClients())
        {
            client->send("msg format ok");
        }
        //wsClient->send("消息格式正确");
    }
}

void waitMsg()
{
    ix::initNetSystem();
    server = new ix::WebSocketServer(8080, "0.0.0.0",  ix::SocketServer::kDefaultTcpBacklog,  ix::SocketServer::kDefaultMaxConnections,
        ix::WebSocketServer::kDefaultHandShakeTimeoutSecs, ix::SocketServer::kDefaultAddressFamily, 36);
    server->disablePerMessageDeflate();       // 关闭压缩
    server->disablePong();  // 有些客户端不发 pong 也会被踢
    server->setOnClientMessageCallback([](std::shared_ptr<ix::ConnectionState> connectionState,
        ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message)
        {
            for (auto& client : server->getClients())
            {
                client->send("receive msg");
            }
            procMsg(msg->str);
        }
    });
    auto res = server->listen();
    if (!res.first)
    {
        return;
    }
    server->start();
    server->wait();
    // Block until server.stop() is called.
    //ix::uninitNetSystem();
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
    //waitMsg();
    std::thread wsThread(waitMsg);
    wsThread.detach();
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