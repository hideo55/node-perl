#include <node_events.h>
#include <node.h>
#include <string>
#include <vector>
#include "embed_perl.h"

#define INTERPRETER_NAME "node-perl"

using namespace v8;

class NodePerl: node::EventEmitter {
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
		if (!args[0]->IsObject()) {
			return ThrowException(Exception::Error(String::New(
					"Arguments must be JavaScript Array")));
		}

		std::vector<std::string> args_v;
		args_v.push_back(INTERPRETER_NAME);

		//process parameters
		Local < Object > arg = args[0]->ToObject();

		if (arg->Has(String::New("opts"))) {
			if (arg->Get(String::New("opts"))->IsArray()) {
				Local < Object > opts
						= (arg->Get(String::New("opts")))->ToObject();
				int len = (opts->GetPropertyNames())->Length();
				for (int i = 0; i < len; i++) {
					args_v.push_back(*String::Utf8Value(opts->Get(Integer::New(
							i))->ToString()));
				}
			}
		}

		if (arg->Has(String::New("script"))) {
			if (arg->Get(String::New("script"))->IsString()) {
				args_v.push_back(*String::Utf8Value(arg->Get(String::New(
						"script"))->ToString()));
			}
		}

		if (arg->Has(String::New("args"))) {
			if (arg->Get(String::New("args"))->IsArray()) {
				Local < Object > argv
						= arg->Get(String::New("args"))->ToObject();
				int len = (argv->GetPropertyNames())->Length();
				for (int i = 0; i < len; i++) {
					args_v.push_back(*String::Utf8Value(argv->Get(Integer::New(
							i))->ToString()));
				}
			}
		}

		std::string *perl_args = new std::string[args_v.size()];
		for (int i = 0; i < args_v.size(); i++) {
			perl_args[i] = args_v[i];
		}

		Local < Function > callback = Local<Function>::Cast(args[1]);
		PerlRunArgs * pargs = (PerlRunArgs *) malloc(sizeof(PerlRunArgs));
		pargs->argc = args_v.size();
		pargs->argv = perl_args;
		pargs->stdout = new std::string("");
		pargs->stderr = new std::string("");
		pargs->interp = Unwrap<NodePerl> (args.This());
		pargs->cb = Persistent<Function>::New(callback);

		eio_custom(EIO_Run, EIO_PRI_DEFAULT, EIO_AfterRun, pargs);

		pargs->interp->Ref();
		ev_ref( EV_DEFAULT_UC);
		return Undefined();
	}

private:
	EmbedPerl *p;

	typedef struct {
		unsigned int argc;
		std::string *argv;
		std::string *stdout;
		std::string *stderr;
		NodePerl *interp;
		Persistent<Function> cb;
	} PerlRunArgs;

	int Run(int argc, std::string *argv, std::string *out, std::string *err) {
		p->run(argc, argv, out, err);
		return 0;
	}

	/* EIO wrapper function for Run() */
	static int EIO_Run(eio_req *req) {
		PerlRunArgs *args = (PerlRunArgs *) req->data;
		int res;
		res = args->interp->Run(args->argc, args->argv, args->stdout,
				args->stderr);
		req->result = res;
		return res;
	}

	static int EIO_AfterRun(eio_req *req) {
		HandleScope scope;
		ev_unref( EV_DEFAULT_UC);
		PerlRunArgs *args = (PerlRunArgs *) req->data;
		Local < Value > argv[2];
		argv[0] = Local<Value>::New(String::New(args->stdout->c_str()));
		argv[1] = Local<Value>::New(String::New(args->stderr->c_str()));

		TryCatch try_catch;

		args->cb->Call(Context::GetCurrent()->Global(), 2, argv);

	    if (try_catch.HasCaught()) {
	        node::FatalException(try_catch);
	    }

		args->interp->Unref();
		args->cb.Dispose();
		delete [] args->argv;
		delete args->stdout;
		delete args->stderr;
		free(args);
		return 0;
	}

};

extern "C" void init(Handle<Object> target) {
	HandleScope scope;
	Local < FunctionTemplate > t = FunctionTemplate::New(NodePerl::New);
	NODE_SET_PROTOTYPE_METHOD(t, "Run", NodePerl::Run);
	t->Inherit(node::EventEmitter::constructor_template);
	t->InstanceTemplate()->SetInternalFieldCount(1);
	target->Set(String::New("Perl"), t->GetFunction());
}

