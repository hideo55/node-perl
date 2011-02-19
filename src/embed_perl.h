#ifndef EMBED_PERL_H_
#define EMBED_PERL_H_

#include <string>
extern "C" {
#define PERLIO_NOT_STDIO 0
#define USE_PERLIO
#include <EXTERN.h>
#include <perl.h>
}

#ifdef New
#undef New
#endif

EXTERN_C void xs_init(pTHX);

class EmbedPerl {
public:

	EmbedPerl() {
		/*
		interp = perl_alloc();
		PERL_SET_CONTEXT(interp);
		PL_perl_destruct_level = 1;
		perl_construct(interp);
		PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
		*/
	}

	~EmbedPerl() {
		/*
		PL_perl_destruct_level = 1;
		perl_destruct(interp);
		perl_free(interp);
		*/
	}

	int run(int argc, const std::string *argv, std::string *out,
			std::string *err) {

		int exitstatus = 0;

		PERL_SYS_INIT3(&argc, (char ***) &argv, (char ***) NULL);

		interp = perl_alloc();
		PERL_SET_CONTEXT(interp);
		PL_perl_destruct_level = 1;
		perl_construct(interp);
		PL_exit_flags |= PERL_EXIT_DESTRUCT_END;

		exitstatus = perl_parse(interp, xs_init, argc, (char **) argv,
				(char **) NULL);
		if (exitstatus != 0) {
			return exitstatus;
		}

		ENTER;SAVETMPS;

		SV *outsv = sv_newmortal();
		SV *errsv = sv_newmortal();

		this->override_stdhandle(outsv, "STDOUT");
		this->override_stdhandle(errsv, "STDERR");

		perl_run(interp);

		this->restore_stdhandle("STDOUT");
		this->restore_stdhandle("STDERR");

		STRLEN outlen = SvCUR(outsv);
		char *tmpout = SvPV_nolen(outsv);
		*out = tmpout;

		STRLEN errlen = SvCUR(errsv);
		char * tmperr = SvPV_nolen(errsv);
		*err = tmperr;

		FREETMPS;LEAVE;

		PL_perl_destruct_level = 1;
		perl_destruct(interp);
		perl_free(interp);

		PERL_SYS_TERM();

		return 0;
	}

private:

	PerlInterpreter *interp;

void override_stdhandle (pTHX_ SV *sv,const char *name ) {
	int status;
	GV *handle = gv_fetchpv(name,TRUE,SVt_PVIO);
	SV *svref = newRV_inc(sv);

	save_gp(handle, 1);

	status = Perl_do_open9(aTHX_ handle, ">:scalar", 8 , FALSE, O_WRONLY, 0, Nullfp, svref, 1);
	if(status == 0) {
		Perl_croak(aTHX_ "Failed to open %s: %" SVf,name, get_sv("!",TRUE));
	}
}

void restore_stdhandle (pTHX_ const char *name) {
	int status;
	GV *handle = gv_fetchpv(name,FALSE,SVt_PVIO);

	if( GvIOn(handle) && IoOFP(GvIOn(handle)) && (PerlIO_flush(IoOFP(GvIOn(handle))) == -1 ) ) {
		Perl_croak(aTHX_ "Failed to flush %s: " SVf,name,get_sv("!",TRUE) );
	}
}

};

#endif /* EMBED_PERL_H_ */
