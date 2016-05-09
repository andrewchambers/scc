
#include <assert.h>
#include <stdlib.h>

#include "arch.h"
#include "../../cc2.h"

enum sflags {
	ISTMP  = 1,
	ISCONS = 2
};

static char opasmw[] = {
	[OADD] = ASADDW,
	[OSUB] = ASSUBW,
	[OMUL] = ASMULW,
	[OMOD] = ASMODW,
	[ODIV] = ASDIVW,
	[OSHL] = ASSHLW,
	[OSHR] = ASSHRW,
	[OLT] = ASLTW,
	[OGT] = ASGTW,
	[OLE] = ASLEW,
	[OGE] = ASGEW,
	[OEQ] = ASEQW,
	[ONE] = ASNEW,
	[OBAND] = ASBANDW,
	[OBOR] = ASBORW,
	[OBXOR] = ASBXORW,
	[OCPL] = ASCPLW
};

static char opasml[] = {
	[OADD] = ASADDL,
	[OSUB] = ASSUBL,
	[OMUL] = ASMULL,
	[OMOD] = ASMODL,
	[ODIV] = ASDIVL,
	[OSHL] = ASSHLL,
	[OSHR] = ASSHRL,
	[OLT] = ASLTL,
	[OGT] = ASGTL,
	[OLE] = ASLEL,
	[OGE] = ASGEL,
	[OEQ] = ASEQL,
	[ONE] = ASNEL,
	[OBAND] = ASBANDL,
	[OBOR] = ASBORL,
	[OBXOR] = ASBXORL,
	[OCPL] = ASCPLL
};

static char opasms[] = {
	[OADD] = ASADDS,
	[OSUB] = ASSUBS,
	[OMUL] = ASMULS,
	[OMOD] = ASMODS,
	[ODIV] = ASDIVS,
	[OSHL] = ASSHLS,
	[OSHR] = ASSHRS,
	[OLT] = ASLTS,
	[OGT] = ASGTS,
	[OLE] = ASLES,
	[OGE] = ASGES,
	[OEQ] = ASEQS,
	[ONE] = ASNES,
	[OBAND] = ASBANDS,
	[OBOR] = ASBORS,
	[OBXOR] = ASBXORS,
	[OCPL] = ASCPLS
};

static char opasmd[] = {
	[OADD] = ASADDD,
	[OSUB] = ASSUBD,
	[OMUL] = ASMULD,
	[OMOD] = ASMODD,
	[ODIV] = ASDIVD,
	[OSHL] = ASSHLD,
	[OSHR] = ASSHRD,
	[OLT] = ASLTD,
	[OGT] = ASGTD,
	[OLE] = ASLED,
	[OGE] = ASGED,
	[OEQ] = ASEQD,
	[ONE] = ASNED,
	[OBAND] = ASBANDD,
	[OBOR] = ASBORD,
	[OBXOR] = ASBXORD,
	[OCPL] = ASCPLD
};

static Node *
tmpnode(Node *np)
{
	Symbol *sym;

	sym = getsym(TMPSYM);
	sym->type = np->type;
	sym->kind = STMP;
	np->u.sym = sym;
	np->op = OTMP;
	np->flags |= ISTMP;
	return np;
}

static Node *
load(Node *np)
{
	Node *new;
	int op;
	Type *tp = &np->type;

	new = tmpnode(newnode());
	new->left = np;
	new->type = *tp;

	switch (tp->size) {
	case 1:
		op = ASLDB;
		break;
	case 2:
		op = ASLDH;
		break;
	case 4:
		op = (tp->flags & INTF) ? ASLDW : ASLDS;
		break;
	case 8:
		op = (tp->flags & INTF) ? ASLDL : ASLDD;
		break;
	default:
		abort();
	}
	code(op, new, np, NULL);

	return new;
}

static Node *
cast(Node *nd, Node *ns)
{
	Type *ts, *td;
	Node *tmp;
	int op, disint, sisint;
	extern Type uint32type, int32type;

	if ((ns->flags & (ISTMP|ISCONS)) == 0)
		ns = nd->left = load(ns);
	td = &nd->type;
	ts = &ns->type;
	disint = (td->flags & INTF) != 0;
	sisint = (ts->flags & INTF) != 0;

	if (disint && sisint) {
		if (td->size <= ts->size)
			return nd;
		assert(td->size == 4 || td->size == 8);
		switch (ts->size) {
		case 1:
			op = (td->size == 4) ? ASEXTBW : ASEXTBL;
			break;
		case 2:
			op = (td->size == 4) ? ASEXTHW : ASEXTHL;
			break;
		case 4:
			op = ASEXTWL;
			break;
		default:
			abort();
		}
		/*
		 * unsigned version of operations are always +1 the
		 * signed version
		 */
		op += (td->flags & SIGNF) == 0;
	} else if (disint) {
		/* conversion from float to int */
		switch (ts->size) {
		case 4:
			op = (td->size == 8) ? ASSTOL : ASSTOW;
			break;
		case 8:
			op = (td->size == 8) ? ASDTOL : ASDTOW;
			break;
		default:
			abort();
		}
		/* TODO: Add signess */
	} else if (sisint) {
		/* conversion from int to float */
		switch (ts->size) {
		case 1:
		case 2:
			tmp = tmpnode(newnode());
			tmp->type = (ts->flags&SIGNF) ? int32type : uint32type;
			tmp->left = ns;
			nd->left = ns = cast(tmp, ns);
		case 4:
			op = (td->size == 8) ? ASSWTOD : ASSWTOS;
			break;
		case 8:
			op = (td->size == 8) ? ASSLTOD : ASSLTOS;
			break;
		default:
			abort();
		}
		/* TODO: Add signess */
	} else {
		/* conversion from float to float */
		op = (td->size == 4) ? ASEXTS : ASTRUNCD;
	}
	code(op, tmpnode(nd), ns, NULL);
	return nd;
}

Node *
cgen(Node *np)
{
	Node *l, *r, *ifyes, *ifno, *next;
	Symbol *sym;
	Type *tp;
	int op, off;
	char *tbl;

	if (!np)
		return NULL;

	setlabel(np->label);
	l = cgen(np->left);
	r = cgen(np->right);
	tp = &np->type;

	switch (np->op) {
	case OSTRING:
		abort();
	case OCONST:
	case OLABEL:
		np->flags |= ISCONS;
	case OMEM:
	case OAUTO:
		return np;
	case OSHR:
	case OMOD:
	case ODIV:
	case OLT:
	case OGT:
	case OLE:
	case OGE:
		/*
		 * unsigned version of operations are always +1 the
		 * signed version
		 */
		off = (tp->flags & SIGNF) == 0;
		goto binary;
	case OADD:
	case OSUB:
	case OMUL:
	case OSHL:
	case OBAND:
	case OBOR:
	case OBXOR:
	case OEQ:
	case ONE:
		off = 0;
	binary:
		switch (tp->size) {
		case 4:
			tbl = (tp->flags & INTF) ? opasmw : opasms;
			break;
		case 8:
			tbl = (tp->flags & INTF) ? opasml : opasmd;
			break;
		default:
			abort();
		}
		op = tbl[np->op] + off;
		if ((l->flags & (ISTMP|ISCONS)) == 0)
			l = np->left = load(l);
		if ((r->flags & (ISTMP|ISCONS)) == 0)
			r = np->right = load(r);
		code(op, tmpnode(np), l, r);
		return np;
	case ONOP:
	case OBLOOP:
	case OELOOP:
		return NULL;
	case OCAST:
		return cast(np, l);
	case OADDR:
		np->flags |= ISTMP;
		np->op = OTMP;
		np->u.sym = l->u.sym;
		return np;
	case OPTR:
		np->left = load(load(l));
		return tmpnode(np);
	case OCPL:
	case OPAR:
	case ONEG:
	case OINC:
	case ODEC:
		abort();
	case OASSIG:
		switch (tp->size) {
		case 1:
			op = ASSTB;
			break;
		case 2:
			op = ASSTH;
			break;
		case 4:
			op = (tp->flags & INTF) ? ASSTW : ASSTS;
			break;
		case 8:
			op = (tp->flags & INTF) ? ASSTL : ASSTD;
			break;
		default:
			abort();
		}
		code(op, l, r, NULL);
		return r;
	case OCALL:
	case OFIELD:
	case OCOMMA:
	case OASK:
	case OCOLON:
	case OAND:
	case OOR:
		abort();
	case OBRANCH:
		if (l && (l->flags & (ISTMP|ISCONS)) == 0)
			l = np->left = load(l);
		next = np->next;
		if (next->label) {
			sym = getsym(TMPSYM);
			sym->kind = SLABEL;
			next->label = sym;
		}
		ifyes = label(np->u.sym);
		ifno = label(next->label);
		op = ASBRANCH;
		np = np->left;
		goto emit_jump;
	case OJMP:
		ifyes = label(np->u.sym);
		op = ASJMP;
		np = ifno = NULL;
	emit_jump:
		code(op, np, ifyes, ifno);
		deltree(ifyes);
		deltree(ifno);
		return NULL;
	case ORET:
		if (l && (l->flags & (ISTMP|ISCONS)) == 0)
			l = np->left = load(l);
		code(ASRET, l, NULL, NULL);
		return NULL;
	case OCASE:
	case ODEFAULT:
	case OTABLE:
	case OSWITCH:
	default:
		abort();
	}
}

/*
 * This is strongly influenced by
 * http://plan9.bell-labs.com/sys/doc/compiler.ps (/sys/doc/compiler.ps)
 * calculate addresability as follows
 *     AUTO => 11          value+fp
 *     REG => 11           reg
 *     STATIC => 11        (value)
 *     CONST => 11         $value
 * These values of addressability are not used in the code generation.
 * They are only used to calculate the Sethi-Ullman numbers. Since
 * QBE is AMD64 targered we could do a better job here, and try to
 * detect some of the complex addressing modes of these processors.
 */
Node *
sethi(Node *np)
{
	Node *lp, *rp;

	if (!np)
		return np;

	np->complex = 0;
	np->address = 0;
	lp = np->left;
	rp = np->right;

	switch (np->op) {
	case OAUTO:
	case OREG:
	case OMEM:
	case OCONST:
		np->address = 11;
		break;
	default:
		sethi(lp);
		sethi(rp);
		break;
	}

	if (np->address > 10)
		return np;
	if (lp)
		np->complex = lp->complex;
	if (rp) {
		int d = np->complex - rp->complex;

		if (d == 0)
			++np->complex;
		else if (d < 0)
			np->complex = rp->complex;
	}
	if (np->complex == 0)
		++np->complex;
	return np;
}
