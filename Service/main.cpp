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
std::unique_ptr<ix::WebSocketServer> server;
std::wstring curPath;

class Msg
{
public:
    ix::WebSocket* client{nullptr};
    JsonObject data;
    void send(const std::string& str) {
        if (client->getReadyState() != ix::ReadyState::Open) return;
        client->sendText(str);
    }
};


void navigateEnd(ICoreWebView2* wv, Msg* msg)
{
    LPWSTR uri = nullptr;
    wv->get_Source(&uri);
    std::wstring url(uri);
    CoTaskMemFree(uri);
    if (url == L"about:blank") {
        return;
    }
    ComPtr<ICoreWebView2_7> webview7;
    wv->QueryInterface(IID_PPV_ARGS(&webview7));
    static int pathName{ 0 };
    pathName += 1;
    auto filePath = std::format(L"{}\\{}.pdf", curPath, pathName);
    auto fileName = std::format("{}.pdf", pathName);
    webview7->PrintToPdf(filePath.data(), printSettings.Get(),
        Callback<ICoreWebView2PrintToPdfCompletedHandler>(
            [webview7,msg, fileName](HRESULT errorCode, BOOL isSuccessful) {
                if (SUCCEEDED(errorCode) && isSuccessful) {
                    auto msgStr = "{\"isFinish\":true, \"msg\" :\""+ fileName +"\", \"ok\":true}";
                    msg->send(msgStr);
                }
                else
                {
                    msg->send(R"({"isFinish":true, "msg":"err", "ok":false})");
                }
                webview7->Navigate(L"about:blank");
                return S_OK;
            }).Get());
}

void loadUrl(Msg* msg) {
    winrt::hstring url = msg->data.GetNamedString(L"url");
    for (auto& wv: webviews)
    {
        LPWSTR uri = nullptr;
        wv->get_Source(&uri);
        std::wstring curUrl(uri);
        CoTaskMemFree(uri);
        if (curUrl == L"about:blank") {
            wv->Navigate(url.data());
            return;
        }
    }
    if (webviews.size() > 18) {
        auto msgStr = R"({"isFinish":false,
"msg":"18 resource slots are currently in use. Please try again later.",
"ok":false})";
        msg->send(msgStr);
    }
    env6->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [url,msg](HRESULT result, ICoreWebView2Controller* controller) {
            ctrls.push_back(controller);
            ComPtr<ICoreWebView2> webview;
            controller->get_CoreWebView2(&webview);
            controller->put_Bounds(bounds);
            webviews.push_back(webview);
            auto navigateEndCB = Callback<ICoreWebView2NavigationCompletedEventHandler>([msg](auto wv, auto args) {
                navigateEnd(wv,msg);
                return S_OK;
            });
            webview->add_NavigationCompleted(navigateEndCB.Get(), NULL);
            webview->Navigate(url.data());
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
            loadUrl((Msg*)lParam);
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
    RegisterClassEx(&wcex);
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, wcex.lpszClassName, wcex.lpszClassName, WS_POPUP,
        -999999, -999999, 1, 1, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);
}

void waitMsg()
{
    ix::initNetSystem();
    server = std::make_unique<ix::WebSocketServer>(8080, "0.0.0.0",  ix::SocketServer::kDefaultTcpBacklog,  ix::SocketServer::kDefaultMaxConnections,
        ix::WebSocketServer::kDefaultHandShakeTimeoutSecs, ix::SocketServer::kDefaultAddressFamily, 36);
    server->setOnClientMessageCallback([](std::shared_ptr<ix::ConnectionState> connectionState,
        ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message)
        {
            webSocket.send(R"({"isFinish":false,"msg":"receive msg","ok":true})");
            winrt::hstring str = winrt::to_hstring(msg->str);
            auto msg = new Msg();
            msg->data = JsonObject::Parse(str);
            msg->client = &webSocket;
            PostMessage(hwnd, WM_APP + 100, NULL, (LPARAM)msg);
        }
    });
    auto res = server->listen();
    if (!res.first)
    {
        return;
    }
    server->start();
    server->wait();
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
    curPath = std::filesystem::path(
        std::filesystem::canonical(std::filesystem::current_path())
    ).wstring();
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
    server->stop();
    ix::uninitNetSystem();
    return 0;
}