#define BUILDING_NODE_EXTENSION

#include <node.h>
#include <string>
#include <vector>
#include "embed_perl.h"

#define INTERPRETER_NAME "node-perl"

using namespace v8;
using namespace node;

class NodePerl: ObjectWrap {
public:

    NodePerl() {
        p = new EmbedPerl();
    }

    static Handle<Value> New(const Arguments& args) {
        HandleScope scope;

        if (!args.IsConstructCall())
            return args.Callee()->NewInstance();
        try {
            (new NodePerl())->Wrap(args.Holder());
        } catch (const char *msg) {
            return ThrowException(Exception::Error(String::New(msg)));
        }
        return scope.Close(args.Holder());
    }

    static Handle<Value> Run(const Arguments& args) {
        HandleScope scope;
        if (!args[0]->IsObject()) {
            return ThrowException(Exception::Error(String::New("Arguments must be JavaScript Array")));
        }

        std::vector<std::string> args_v;
        args_v.push_back(INTERPRETER_NAME);

        //process parameters
        Local<Object> arg = args[0]->ToObject();

        if (arg->Has(String::New("opts"))) {
            if (arg->Get(String::New("opts"))->IsArray()) {
                Local<Object> opts = (arg->Get(String::New("opts")))->ToObject();
                int len = (opts->GetPropertyNames())->Length();
                for (int i = 0; i < len; i++) {
                    args_v.push_back(*String::Utf8Value(opts->Get(Integer::New(i))->ToString()));
                }
            }
        }

        if (arg->Has(String::New("script"))) {
            if (arg->Get(String::New("script"))->IsString()) {
                args_v.push_back(*String::Utf8Value(arg->Get(String::New("script"))->ToString()));
            }
        }

        if (arg->Has(String::New("args"))) {
            if (arg->Get(String::New("args"))->IsArray()) {
                Local<Object> argv = arg->Get(String::New("args"))->ToObject();
                int len = (argv->GetPropertyNames())->Length();
                for (int i = 0; i < len; i++) {
                    args_v.push_back(*String::Utf8Value(argv->Get(Integer::New(i))->ToString()));
                }
            }
        }

        std::string *perl_args = new std::string[args_v.size()];
        for (int i = 0; i < args_v.size(); i++) {
            perl_args[i] = args_v[i];
        }

        Persistent<Function> callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));

        std::string out, err;
        Unwrap<NodePerl> (args.This())->Run(args_v.size(), perl_args, out, err);

        Handle<Value> argv[2];
        argv[0] = String::New(out.c_str(), out.size());
        argv[1] = String::New("");

        callback->Call(Context::GetCalling()->Global(), 2, argv);

        return scope.Close(Undefined());
    }

private:
    EmbedPerl *p;

    int Run(int argc, std::string *argv, std::string& out, std::string& err) {
        p->run(argc, argv, out, err);
        return 0;
    }
};

extern "C" void init(Handle<Object> target) {
    HandleScope scope;
    Local<FunctionTemplate> t = FunctionTemplate::New(NodePerl::New);
    NODE_SET_PROTOTYPE_METHOD(t, "Run", NodePerl::Run);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    target->Set(String::New("Perl"), t->GetFunction());
}

