
#define TINT    long long
#define TUINT   unsigned long long
#define TFLOAT  double
#define TSIZE   unsigned long

enum asmop {
	ASSTB,
	ASSTH,
	ASSTW,
	ASSTL,
	ASSTS,
	ASSTD,

	ASLDB,
	ASLDH,
	ASLDW,
	ASLDL,
	ASLDS,
	ASLDD,

	ASADDW,
	ASSUBW,
	ASMULW,
	ASMODW,
	ASUMODW,
	ASDIVW,
	ASUDIVW,
	ASSHLW,
	ASSHRW,
	ASUSHRW,
	ASLTW,
	ASULTW,
	ASGTW,
	ASUGTW,
	ASLEW,
	ASULEW,
	ASGEW,
	ASUGEW,
	ASEQW,
	ASNEW,
	ASBANDW,
	ASBORW,
	ASBXORW,
	ASCPLW,

	ASADDL,
	ASSUBL,
	ASMULL,
	ASMODL,
	ASUMODL,
	ASDIVL,
	ASUDIVL,
	ASSHLL,
	ASSHRL,
	ASUSHRL,
	ASLTL,
	ASULTL,
	ASGTL,
	ASUGTL,
	ASLEL,
	ASULEL,
	ASGEL,
	ASUGEL,
	ASEQL,
	ASNEL,
	ASBANDL,
	ASBORL,
	ASBXORL,
	ASCPLL,

	ASADDS,
	ASSUBS,
	ASMULS,
	ASMODS,
	ASDIVS,
	ASSHLS,
	ASSHRS,
	ASLTS,
	ASGTS,
	ASLES,
	ASGES,
	ASEQS,
	ASNES,
	ASBANDS,
	ASBORS,
	ASBXORS,
	ASCPLS,

	ASADDD,
	ASSUBD,
	ASMULD,
	ASMODD,
	ASDIVD,
	ASSHLD,
	ASSHRD,
	ASLTD,
	ASGTD,
	ASLED,
	ASGED,
	ASEQD,
	ASNED,
	ASBANDD,
	ASBORD,
	ASBXORD,
	ASCPLD,

	ASEXTBW,
	ASUEXTBW,
	ASEXTBL,
	ASUEXTBL,
	ASEXTHW,
	ASUEXTHW,
	ASEXTHL,
	ASUEXTHL,
	ASEXTWL,
	ASUEXTWL,

	ASSTOL,
	ASSTOW,
	ASDTOL,
	ASDTOW,

	ASSWTOD,
	ASSWTOS,
	ASSLTOD,
	ASSLTOS,

	ASEXTS,
	ASTRUNCD
};
