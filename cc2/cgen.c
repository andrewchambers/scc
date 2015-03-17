
#include <stdarg.h>
#include <inttypes.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "cc2.h"


static Node *reguse[NREGS];
static char upper[] = {[DE] = D, [HL] = H, [BC] = B,  [IY] = IYH};
static char lower[] = {[DE] = E, [HL] = L, [BC] = C, [IY] = IYL};
static char pair[] = {
	[A] = A,
	[H] = HL, [L] = HL,
	[B] = BC, [C] = BC,
	[D] = DE, [E] = DE,
	[IYL] = IY, [IYH] = IY
};

Node
reg_E = {
	.op = REG,
	.reg = E
},
reg_D = {
	.op = REG,
	.reg = D
},
reg_H = {
	.op = REG,
	.reg = H
},
reg_L = {
	.op = REG,
	.reg = L
},
reg_C = {
	.op= REG,
	.reg = C
},
reg_B = {
	.op= REG,
	.reg = B
},
reg_A = {
	.op= REG,
	.reg = A
},
reg_IYL = {
	.op = REG,
	.reg = IYL
},
reg_IYH = {
	.op = REG,
	.reg = IYH
},
reg_DE = {
	.op = REG,
	.reg = DE
},
reg_HL = {
	.op = REG,
	.reg = HL
},
reg_BC = {
	.op = REG,
	.reg = BC
},
reg_IX = {
	.op = REG,
	.reg = IX
},
reg_IY = {
	.op = REG,
	.reg = IY
},
reg_SP = {
	.op = REG,
	.reg = SP
};

Node *regs[] = {
	[A] = &reg_A,
	[B] = &reg_B, [C] = &reg_C,
	[D] = &reg_D, [E] = &reg_E,
	[H] = &reg_H, [L] = &reg_L,
	[IYL] = &reg_IYL, [IYH] = &reg_IYH,
	[HL] = &reg_HL, [DE] = &reg_DE, [BC]= &reg_BC,
	[IX] = &reg_IX, [IY] = &reg_IY, [SP] = &reg_SP
};

static char
allocreg(Node *np)
{
	static char reg8[] = {A, B, C, D, E, H, L, IYL, IY, 0};
	static char reg16[] = {BC, DE, IY, 0};
	char *bp, c;

	switch (np->type.size) {
	case 1:
		for (bp = reg8; (c = *bp); ++bp) {
			if (reguse[c])
				continue;
			return c;
		}
		break;
	case 2:
		for (bp = reg16; (c = *bp); ++bp) {
			char u = upper[c], l = lower[c];

			if (reguse[u] || reguse[l])
				continue;
			return c;
		}
		break;
	}
	abort();
}

static void
moveto(Node *np, uint8_t reg)
{
	Symbol *sym = np->sym;
	char op = np->op;

	switch (np->type.size) {
	case 1:
		switch (op) {
		case CONST:
			code(LDI, regs[reg], np);
			break;
		case AUTO:
			code(LDL, regs[reg], np);
			break;
		default:
			abort();
		}
		reguse[reg] = np;
		break;
	case 2:
		switch (op) {
		case CONST:
			code(LDL, regs[reg], np);
			break;
		case AUTO:
			code(LDL, regs[lower[reg]], np);
			code(LDH, regs[upper[reg]], np);
			break;
		default:
			abort();
		}
		reguse[upper[reg]] = reguse[lower[reg]] = np;
		break;
	default:
		abort();
	}
	np->op = REG;
	np->reg = reg;
}

static void
move(Node *np)
{
	moveto(np, allocreg(np));
}

static void
push(uint8_t reg)
{
	Node *np;

	if (reg < NREGS)
		reg = pair[reg];
	if ((np = reguse[lower[reg]]) != NULL)
		np->op = PUSHED;
	if ((np = reguse[upper[reg]]) != NULL)
		np->op = PUSHED;
	code(PUSH, NULL, regs[reg]);
}

static void
accum(Node *np)
{
	switch (np->type.size) {
	case 1:
		if (reguse[A])
			push(A);
		moveto(np, A);
		break;
	case 2:
		if (reguse[H] || reguse[L])
			push(HL);
		moveto(np, HL);
		break;
	case 4:
	case 8:
		abort();
	}
}

static void
index(Node *np)
{
	if (reguse[H] || reguse[L])
		push(HL);
	code(LDI, &reg_HL, np);
	reguse[H] = reguse[L] = np;
	np->op = INDEX;
}

static void
conmute(Node *np)
{
	Node *p, *q;

	p = np->left;
	q = np->right;
	np->left = q;
	np->right = p;
}

static void
add(Node *np)
{
	Node *lp = np->left, *rp = np->right;

	if (rp->op == REG || lp->op == CONST) {
		conmute(np);
		lp = np->left;
		rp = np->right;
	}
	switch (np->type.size) {
	case 1:
		switch (lp->op) {
		case PAR:
		case AUTO:
			switch (rp->op) {
			case MEM:
				index(rp);
			case PAR:
			case AUTO:
			case CONST:
				accum(lp);
				break;
			default:
				abort();
			}
			break;
		case MEM:
			if (reguse[A]) {
				switch (rp->op) {
				case PAR:
				case AUTO:
				case CONST:
					break;
				case MEM:
					index(rp);
					goto add_A;
				default:
					abort();
				}
			}
			accum(lp);
			break;
		case REG:
			if (lp->reg != A)
				moveto(lp, A);
			switch (rp->op) {
			case REG:
			case AUTO:
			case PAR:
			case CONST:
			case MEM:
				break;
			default:
				abort();
			}		
			break;			
		default:
			abort();
		}
	add_A:
		code(ADD, lp, rp);
		np->op = REG;
		np->reg = A;
		break;
	case 2:
	case 4:
	case 8:
		abort();
	}
}

static void
assign(Node *np)
{
	Node *lp = np->left, *rp = np->right;

	switch (np->type.size) {
	case 1:
		switch (lp->op) {
		case AUTO:
			code(LDL, lp, rp);
			break;
		case REG:
			code(MOV, lp, rp);
			break;
		case MEM:
			index(lp);
			code(LDL, lp, rp);
			break;
		default:
			abort();
		}
		break;
	default:
		abort();
	}
}


static void (*opnodes[])(Node *) = {
	[OADD] = add,
	[OASSIG] = assign
};

static void
cgen(Node *np, Node *parent)
{
	Node *lp, *rp;

	if (!np)
		return;

	if (np->addable >= ADDABLE)
		return;

	lp = np->left;
	rp = np->right;
	if (!lp) {
		cgen(rp, np);
	} else if (!rp) {
		cgen(lp, np);
	} else {
		Node *p, *q;
		if (lp->complex > rp->complex)
			p = lp, q = rp;
		else
			p = rp, q = lp;
		cgen(p, np);
		cgen(q, np);
	}
	(*opnodes[np->op])(np);
}

static Node *
applycgen(Node *np)
{
	cgen(np, NULL);
	return NULL;
}

void
generate(void)
{
	extern char odebug;
	uint8_t i, size = curfun->u.f.locals;
	char frame =  size != 0 || odebug;

	if (frame) {
		code(PUSH, NULL, &reg_IX);
		code(MOV, &reg_IX, &reg_SP);
		if (size > 6) {
			code(MOV, &reg_HL, imm(-size));
			code(ADD, &reg_HL, &reg_SP);
			code(MOV, &reg_SP, &reg_HL);
		} else {
			for (i = size; i != 0; i-= 2)
				code(PUSH, NULL, &reg_HL);
		}
	}

	apply(applycgen);

	if (frame) {
		code(MOV, &reg_SP, &reg_IX);
		code(POP, &reg_IX, NULL);
		code(RET, NULL, NULL);
	}
}

/*
 * This is strongly influenced by
 * http://plan9.bell-labs.com/sys/doc/compiler.ps (/sys/doc/compiler.ps)
 * calculate addresability as follows
 *     AUTO => 11          value+fp
 *     REGISTER => 13      register
 *     STATIC => 12        (value)
 *     CONST => 20         $value
 */
Node *
genaddable(Node *np)
{
	Node *lp, *rp;

	if (!np)
		return np;

	np->complex = 0;
	np->addable = 0;
	lp = np->left;
	rp = np->right;
	switch (np->op) {
	case AUTO:
		np->addable = 11;
		break;
	case REG:
		np->addable = 13;
		break;
	case MEM:
		np->addable = 12;
		break;
	case CONST:
		np->addable = 20;
		break;
	default:
		if (lp)
			genaddable(lp);
		if (rp)
			genaddable(rp);
		break;
	}

	if (np->addable > 10)
		return np;
	if (lp)
		np->complex = lp->complex;
	if (rp) {
		int8_t d = np->complex - rp->complex;

		if (d == 0)
			++np->complex;
		else if (d < 0)
			np->complex = rp->complex;
	}
	if (np->complex == 0)
		++np->complex;
	return np;
}

void
addable(void)
{
	apply(genaddable);
}
