#include <napi.h>
#include <thread>
#include <chrono>

class Worker : public Napi::AsyncWorker
{
public:
    Worker(Napi::Promise::Deferred deferred);
    void Execute() override;
    void OnOK() override;
    void OnError(const Napi::Error &error) override;

private:
    Napi::Promise::Deferred deferred;
};