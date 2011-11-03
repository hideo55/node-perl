#include <node.h>
#include <node_version.h>
#include <string>
#include <vector>
#include "embed_perl.h"

#define INTERPRETER_NAME "node-perl"
#define REQ_FUN_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a function")));  \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

using namespace v8;

class NodePerl: node::ObjectWrap {
public:

    NodePerl() {
        p = new EmbedPerl();
    }

    static Handle<Value> New(const Arguments& args) {
        HandleScope scope;

        if (!args.IsConstructCall())
            return args.Callee()->NewInstance();
        try {
            (new NodePerl())->Wrap(args.This());
        } catch (const char *msg) {
            return ThrowException(Exception::Error(String::New(msg)));
        }
        return args.This();
    }

    static Handle<Value> Run(const Arguments& args) {
        HandleScope scope;
        int res;
        int argc;
        std::string *perl_args;

        Unwrap<NodePerl> (args.This())->process_run_args(args, &argc,
                &perl_args);

        std::string out, err;

        REQ_FUN_ARG(1,callback)
        res = Unwrap<NodePerl> (args.This())->Run(argc, perl_args, &out, &err);

        Local<Value> argv[3];
        argv[0] = res < 0 ? Local<Value>::New(String::New("Error")) : Local<
                Value>::New(Undefined());
        argv[1] = Local<Value>::New(String::New(out.c_str()));
        argv[2] = Local<Value>::New(String::New(err.c_str()));

        callback->Call(Context::GetCurrent()->Global(), 3, argv);

        return Undefined();
    }

    static Handle<Value> RunAsync(const Arguments& args) {
        HandleScope scope;

        int argc;
        std::string *argv;

        Unwrap<NodePerl> (args.This())->process_run_args(args, &argc, &argv);
        REQ_FUN_ARG(1,callback);

        PerlRunArgs *pargs = new PerlRunArgs;
        pargs->node_perl = Unwrap<NodePerl> (args.This());
        pargs->argc = argc;
        pargs->argv = argv;
        pargs->stdout = new std::string();
        pargs->stderr = new std::string();
        pargs->callback = Persistent<Function>::New(callback);

        eio_custom(EIO_Run, EIO_PRI_DEFAULT, EIO_AfterRun, pargs);

        ev_ref( EV_DEFAULT_UC);
        pargs->node_perl->Ref();

        return Undefined();
    }

private:
    EmbedPerl *p;

    typedef struct {
        NodePerl *node_perl;
        int argc;
        std::string *argv;
        std::string *stdout;
        std::string *stderr;
        Persistent<Function> callback;

    } PerlRunArgs;

    Handle<Value> process_run_args(const Arguments& args, int *argc,
            std::string **argv) {
        HandleScope scope;

        if (!args[0]->IsObject()) {
            return ThrowException(
                    Exception::Error(
                            String::New("Arguments must be JavaScript Array")));
        }

        std::vector<std::string> args_v;
        args_v.push_back(INTERPRETER_NAME);

        //process parameters
        Local<Object> arg = args[0]->ToObject();

        if (arg->Has(String::New("opts"))) {
            if (arg->Get(String::New("opts"))->IsArray()) {
                Local<Object> opts =
                        (arg->Get(String::New("opts")))->ToObject();
                int len = (opts->GetPropertyNames())->Length();
                for (int i = 0; i < len; i++) {
                    args_v.push_back(
                            *String::Utf8Value(
                                    opts->Get(Integer::New(i))->ToString()));
                }
            }
        }

        if (arg->Has(String::New("script"))) {
            if (arg->Get(String::New("script"))->IsString()) {
                args_v.push_back(
                        *String::Utf8Value(
                                arg->Get(String::New("script"))->ToString()));
            }
        }

        if (arg->Has(String::New("args"))) {
            if (arg->Get(String::New("args"))->IsArray()) {
                Local<Object> argv = arg->Get(String::New("args"))->ToObject();
                int len = (argv->GetPropertyNames())->Length();
                for (int i = 0; i < len; i++) {
                    args_v.push_back(
                            *String::Utf8Value(
                                    argv->Get(Integer::New(i))->ToString()));
                }
            }
        }

        *argc = args_v.size();
        std::string *tmp_argv = new std::string[*argc];

        for (int i = 0; i < *argc; i++) {
            tmp_argv[i] = args_v[i];
        }
        *argv = tmp_argv;
        return Undefined();
    }

    int Run(int argc, std::string *argv, std::string *out, std::string *err) {
        p->run(argc, argv, out, err);
        return 0;
    }

#if NODE_VERSION_AT_LEAST(0,5,0)
    static void EIO_Run(eio_req *req) {
#else
    static int EIO_Run(eio_req *req) {
#endif
        PerlRunArgs *data = reinterpret_cast<PerlRunArgs *>(req->data);
        req->result = data->node_perl->Run(data->argc, data->argv,
                data->stdout, data->stderr);

#if !NODE_VERSION_AT_LEAST(0,5,0)
        return 0;
#endif
    }

    static int EIO_AfterRun(eio_req *req) {
        HandleScope scope;

        ev_unref( EV_DEFAULT_UC);

        PerlRunArgs *data = reinterpret_cast<PerlRunArgs *>(req->data);

        Handle<Value> argv[3];
        argv[0] = req->result < 0 ? Local<Value>::New(String::New("Error"))
                : Local<Value>::New(Undefined());
        argv[1] = Local<Value>::New(String::New(data->stdout->c_str()));
        argv[2] = Local<Value>::New(String::New(data->stderr->c_str()));

        data->callback->Call(Context::GetCurrent()->Global(), 3, argv);

        data->node_perl->Unref();
        data->callback.Dispose();

        delete[] data->argv;
        delete data->stdout;
        delete data->stderr;
        delete data;

        return 0;
    }

};

extern "C" void init(Handle<Object> target) {
    HandleScope scope;
    Local<FunctionTemplate> t = FunctionTemplate::New(NodePerl::New);
    NODE_SET_PROTOTYPE_METHOD(t, "Run", NodePerl::Run);
    NODE_SET_PROTOTYPE_METHOD(t, "RunAsync", NodePerl::RunAsync);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    target->Set(String::New("Perl"), t->GetFunction());
    target->Set(String::New("version"), String::New("0.0.1"));
}
