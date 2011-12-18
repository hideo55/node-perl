#ifndef EMBED_PERL_H_
#define EMBED_PERL_H_

#include <string>
extern "C" {
#define PERLIO_NOT_STDIO 0
#define USE_PERLIO
#include <EXTERN.h>
#include <perl.h>
#include "ppport.h"
}

#ifdef New
#undef New
#endif

EXTERN_C void xs_init(pTHX);

class EmbedPerl {
public:

	EmbedPerl() {
		perl = perl_alloc();
		perl_construct(perl);
	}

	~EmbedPerl() {
		PL_perl_destruct_level = 2;
		perl_destruct(perl);
		perl_free(perl);
	}

	int run(int argc, const std::string *argv, std::string& out,
			std::string& err) {

		int exitstatus = 0;

		PERL_SYS_INIT3(&argc, (char ***) &argv, (char ***) NULL);

		PL_perl_destruct_level = 2;
		exitstatus = perl_parse(perl, xs_init, argc, (char **) argv,
				(char **) NULL);
		if (exitstatus != 0) {
			return exitstatus;
		}

		ENTER;SAVETMPS;

		SV *outsv = sv_newmortal();
		SV *errsv = sv_newmortal();

		this->override_stdhandle(aTHX_ outsv, "STDOUT");
		this->override_stdhandle(aTHX_ errsv, "STDERR");

		perl_run(perl);

		this->restore_stdhandle(aTHX_ "STDOUT");
		this->restore_stdhandle(aTHX_ "STDERR");

		STRLEN outlen = SvCUR(outsv);
		char *tmpout = SvPV_nolen(outsv);
		out = std::string(tmpout, outlen);

		STRLEN errlen = SvCUR(errsv);
		char * tmperr = SvPV_nolen(errsv);
		err = std::string(tmperr, errlen);

		FREETMPS;LEAVE;

		PERL_SYS_TERM();

		return 0;
	}

private:

	PerlInterpreter *perl;

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
