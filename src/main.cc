#include <napi.h>
#include <Windows.h>
#include "Worker.h"
#include <libipc/ipc.h>

std::unique_ptr<ipc::route> ipcIns;

Napi::Value Init(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    HANDLE hJob = CreateJobObject(nullptr, nullptr);
    if (!hJob)
    {
        Napi::Error::New(env, "Failed to create job object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE; // 当 Job 关闭时，终止所有子进程
    if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo)))
    {
        CloseHandle(hJob);
        Napi::Error::New(env, "Failed to set job object info").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    WCHAR cmd[] = L"D:\\project\\WV2PDF\\exe\\x64\\Debug\\SimpleDemo.exe";
    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    BOOL success = CreateProcess(nullptr, cmd, nullptr, nullptr,
                                 FALSE, CREATE_SUSPENDED, // 暂停启动
                                 nullptr, nullptr, &si, &pi);
    if (!success)
    {
        CloseHandle(hJob);
        Napi::Error::New(env, "Failed to create process").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!AssignProcessToJobObject(hJob, pi.hProcess)) // 将子进程加入 Job
    {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(hJob);
        Napi::Error::New(env, "Failed to assign process to job").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    ipcIns = std::make_unique<ipc::route>("WV2PDF", ipc::sender);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess); //不要 CloseHandle(hJob) —— 让它随父进程退出自动销毁
    Napi::Object result = Napi::Object::New(env);
    result.Set("ok", true);
    return result;
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