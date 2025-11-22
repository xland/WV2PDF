#include <napi.h>
#include <thread>
#include <chrono>

class Worker : public Napi::AsyncWorker
{
public:
    Worker(Napi::Promise::Deferred deferred) : Napi::AsyncWorker(deferred.Env()), deferred(deferred)
    {
    }
    void Execute() override
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    void OnOK() override
    {
        Napi::Object obj = Napi::Object::New(Env());
        obj.Set("ok", true);
        deferred.Resolve(obj);
    }
    void OnError(const Napi::Error &error) override
    {
        deferred.Reject(error.Value());
    }

private:
    Napi::Promise::Deferred deferred;
};