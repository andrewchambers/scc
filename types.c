
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"
#include "machine.h"

#define NR_TYPE_HASH 16

Type
	*voidtype = &(Type) {
		.op = VOID
	},
	*uchartype = &(Type) {
		.op = INT,
		.size = CHARSIZE,
		.sign = 1
	},
	*chartype = &(Type) {
		.op = INT,
		.size = CHARSIZE,
	},
	*uinttype = &(Type) {
		.op = INT,
		.size = INTSIZE,
		.sign = 1
	},
	*inttype = &(Type) {
		.op = INT,
		.size = INTSIZE,
		.sign = 1
	},
	*ushortype = &(Type) {
		.op = INT,
		.size = SHORTSIZE,
	},
	*shortype = &(Type) {
		.op = INT,
		.size = INTSIZE,
	},
	*longtype = &(Type) {
		.op = INT,
		.size = LONGSIZE,
	},
	*ulongtype = &(Type) {
		.op = INT,
		.size = LONGSIZE,
		.sign = 1
	},
	*ullongtype = &(Type) {
		.op = INT,
		.size = LLONGSIZE,
		.sign = 1
	},
	*llongtype = &(Type) {
		.op = INT,
		.size = LLONGSIZE,
	},
	*floattype = &(Type) {
		.op = FLOAT,
		.size = FLOATSIZE
	},
	*cfloattype = &(Type) {
		.op = FLOAT,
		.size = FLOATSIZE,
		.cplex = 1
	},
	*ifloattype = &(Type) {
		.op = FLOAT,
		.size = FLOATSIZE,
		.imag = 1
	},
	*doubletype = &(Type) {
		.op = FLOAT,
		.size = FLOATSIZE
	},
	*cdoubletype = &(Type) {
		.op = FLOAT,
		.size = 0,
		.cplex = 1
	},
	*idoubletype = &(Type) {
		.op = FLOAT,
		.size = 0,
		.imag = 1
	},
	*ldoubletype = &(Type) {
		.op = FLOAT,
		.size = LLFLOATSIZE
	},
	*cldoubletype = &(Type) {
		.op = FLOAT,
		.size = 0,
		.cplex = 1
	},
	*ildoubletype = &(Type) {
		.op = FLOAT,
		.size = 0,
		.imag = 1
	};

Type *
ctype(int8_t type, int8_t sign, int8_t size, int8_t cplex)
{
	if (type == CHAR && !sign)
		sign = options.charsign;
	if (sign == SIGNED)
		sign = 0;
	if (type == DOUBLE)
		type = FLOAT, size += LONG;

	switch (type) {
	case VOID:                      return voidtype;
	case CHAR:                      return (sign) ? uchartype  : chartype;
	case INT: switch (size) {
		case 0:                 return (sign) ? uinttype   : inttype;
		case SHORT:             return (sign) ? ushortype  : shortype;
		case LONG:              return (sign) ? longtype   : ulongtype;
		case LONG+LONG:         return (sign) ? ullongtype : llongtype;
		}
	case FLOAT: switch (size) {
		case 0: switch (cplex) {
			case 0:         return floattype;
			case COMPLEX:   return cfloattype;
			case IMAGINARY: return ifloattype;
		}
		case LONG: switch (cplex) {
			case 0:         return doubletype;
			case COMPLEX:   return cdoubletype;
			case IMAGINARY: return ifloattype;
		}
		case LONG+LONG: switch (cplex) {
			case 0:         return ldoubletype;
			case COMPLEX:   return cldoubletype;
			case IMAGINARY: return ildoubletype;
		}
		}
	}
}

Type *
mktype(Type *tp, uint8_t op,
       Symbol *sym, uint16_t nelem)
{
	static Type *typetab[NR_TYPE_HASH], **tbl;
	static uint8_t t;
	static unsigned short size;
	register Type *bp;

	t = (op  ^  (uint8_t) ((unsigned short) tp >> 3))
	         & NR_TYPE_HASH-1;
	tbl = &typetab[t];
	if (op != FTN || op != STRUCT || op != UNION || op != ENUM) {
		for (bp = *tbl; bp; bp = bp->next) {
			if (bp->type == tp && bp->op == op &&
			    bp->sym == sym && bp->nelem == nelem) {
				return bp;
			}
		}
	}

	switch (op) {
	case PTR: size = PTRSIZE; break;
	case FTN: size = 0; break;
	case ARY: size = tp->size * nelem; break;
	case ENUM: size = INTSIZE;
	case STRUCT: size = 0; break;
	default:  size = tp->size; break;
	}
	bp = xmalloc(sizeof(*bp));
	bp->next = *tbl;
	bp->type = tp;
	bp->op = op;
	bp->nelem = nelem;
	bp->sym = sym;
	bp->size = size;
	return *tbl = bp;
}

Type *
qualifier(Type *tp, uint8_t qlf)
{
	uint8_t q = tp->op;

	if (q & TQUALIFIER) {
		if (q == qlf)
			return tp;
		tp = tp->type;
		qlf |= q;
	}
	return mktype(tp, qlf|TQUALIFIER, NULL, 0);
}

#include <stdio.h>

static void
ptype(Type *tp)
{
	uint8_t op;
	struct funpar *fp;

	if (!tp)
		return;
	op = tp->op;
	if (op & TQUALIFIER) {
		if (op & CONST)
			fputs("const ", stdout);
		if (op & VOLATILE)
			fputs("volatile ", stdout);
		if (op & RESTRICT)
			fputs("restrict ", stdout);
	} else {
		switch (op) {
		case PTR: fputs("pointer ", stdout); break;
		case ARY: fputs("array ", stdout); break;
		case STRUCT: fputs("struct", stdout); break;
		case UNION: fputs("union", stdout); break;
		case ENUM: fputs("enum", stdout); break;
		case BOOL: fputs("bool", stdout); break;
		case INT:
			printf("int size=%u sign=%u", tp->size, tp->sign);
			break;
		case FLOAT:
			printf("float size=%u cplex=%u", tp->size, tp->cplex);
			break;
		case TYPENAME:
			printf("typename %s type ", tp->sym->name);
			break;
		case FTN:
			fputs("function(", stdout);
			for (fp = tp->u.pars; fp; fp = fp->next) {
				ptype(tp);
				if (fp->next)
					fputs(", ", stdout);
			}
			fputs(") ", stdout);
			break;
		}
	}
	ptype(tp->type);
}

void
printtype(Type *tp)
{
	printf("type = %p ", tp);
	ptype(tp);
	putchar('\n');
}

