#include <napi.h>
#include <Windows.h>
#include "Worker.h"

HWND hwnd;
Napi::Value Init(const Napi::CallbackInfo &info)
{

    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    auto flag = CreateProcess(nullptr, const_cast<LPWSTR>(L"wv.exe"),
                              nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
    if (!flag)
    {
        Napi::Env env = info.Env();
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("ok", false);
        obj.Set("err", "can not create process");
        return obj;
    }
    while (true)
    {
        hwnd = FindWindow(L"WV2PDFWINDOW", nullptr);
        if (hwnd != nullptr)
        {
            break;
        }
        Sleep(200);
    }
    Napi::Env env = info.Env();
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("ok", true);
    return obj;
}

Napi::Value HTML2PDF(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    (new Worker(deferred))->Queue();
    return deferred.Promise();
}

Napi::Value URL2PDF(const Napi::CallbackInfo &info)
{

    Napi::Env env = info.Env();
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("ok", true);
    return obj;
}

Napi::Value Dispose(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("ok", true);
    return obj;
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set("init", Napi::Function::New<Init>(env));
    exports.Set("html2pdf", Napi::Function::New<HTML2PDF>(env));
    exports.Set("url2pdf", Napi::Function::New<URL2PDF>(env));
    exports.Set("dispose", Napi::Function::New<Dispose>(env));
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)