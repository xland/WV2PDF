#include "Worker.h"

Worker::Worker(Napi::Promise::Deferred deferred) : Napi::AsyncWorker(deferred.Env()), deferred(deferred)
{
}
void Worker::Execute()
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
}
void Worker::OnOK()
{
    Napi::Object obj = Napi::Object::New(Env());
    obj.Set("ok", true);
    deferred.Resolve(obj);
}
void Worker::OnError(const Napi::Error &error)
{
    deferred.Reject(error.Value());
}