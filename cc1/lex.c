
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

uint8_t lex_ns = NS_IDEN;
const char *filename;
unsigned linenum;

uint8_t yytoken;
struct yystype yylval;
char yytext[IDENTSIZ + 1];

static uint8_t
integer(char *s, char base)
{
	static Type *tp;
	static Symbol *sym;
	static char ch, size, sign;
	static long v;

	size = sign = 0;
type:
	switch (ch = toupper(getchar())) {
	case 'L':
		if (size == LONG + LONG)
			goto wrong_type;
		size += LONG;
		goto type;
	case 'U':
		if (sign == UNSIGNED)
			goto wrong_type;
		goto type;
	default:
		ungetc(ch, stdin);
		tp = ctype(INT, sign, size);
		break;
	wrong_type:
		error("invalid suffix in integer constant");
	}

	sym = install("", NS_IDEN);
	sym->type = tp;
	v = strtol(yytext, NULL, base);
	if (tp == inttype)
		sym->u.i = v;
	yylval.sym = sym;
	return CONSTANT;
}

static uint8_t
number(void)
{
	char ch, *bp;
	static char base;

	if ((ch = getchar()) == '0') {
		if (toupper(ch = getchar()) == 'X') {
			base = 16;
		} else {
			base = 8;
			ungetc(ch, stdin);
		}
	} else {
		base = 10;
		ungetc(ch, stdin);
	}

	for (bp = yytext ; bp < &yytext[IDENTSIZ]; *bp++ = ch) {
		ch = getchar();
		switch (base) {
		case 8:
			if (ch >= '7')
				goto end;
			/* passthru */
		case 10:
			if (!isdigit(ch))
				goto end;
			break;
		case 16:
			if (!isxdigit(ch))
				goto end;
			break;
		}
	}

end:
	if (bp == &yytext[IDENTSIZ])
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(ch, stdin);
	return integer(yytext, base);
}

static char *
escape(char *s)
{
	char c;

repeat:
	switch (getchar()) {
	case '\\': c = '\''; break;
	case 'a': c = '\a'; break;
	case 'f': c = '\f'; break;
	case 'n': c = '\n'; break;
	case 'r': c = '\r'; break;
	case 't': c = '\t'; break;
	case 'v': c = '\v'; break;
	case '\'': c = '\\'; break;
	case '"': c ='"'; break;
	case '?': c = '?'; break;
	case 'x': /* TODO: */
	case '0': /* TODO: */
	case 'u': /* TODO: */
	case '\n':
		 ++linenum;
		if ((c = getchar()) == '\\')
			goto repeat;
		break;
	default:
		warn("unknown escape sequence");
		return s;
	}

	*s = c;
	return ++s;
}

static uint8_t
character(void)
{
	static char c;
	Symbol *sym;

	getchar();   /* discard the initial ' */
	c = getchar();
	if (c == '\\')
		escape(&c);
	if (getchar() != '\'')
		error("invalid character constant");
	sym = install("", NS_IDEN);
	sym->u.i = c;
	sym->type = inttype;
	yylval.sym = sym;
	return CONSTANT;
}

static uint8_t
string(void)
{
	static char buf[STRINGSIZ+1];
	char *bp;
	int c;
	static Symbol *sym;

	getchar(); /* discard the initial " */

	for (bp = buf; bp < &buf[STRINGSIZ]; ) {
		switch (c = getchar()) {
		case EOF:
			error("found EOF while parsing");
		case '"':
			goto end_string;
		case '\\':
			bp = escape(bp);
			break;
		default:
			*bp++ = c;
		}
	}

end_string:
	if (bp == &buf[IDENTSIZ])
		error("string too long");
	*bp = '\0';
	sym = install("", NS_IDEN);
	sym->u.s = xstrdup(buf);
	sym->type = mktype(chartype, PTR, 0, NULL);
	yylval.sym = sym;
	return STRING;
}

static uint8_t
iden(void)
{
	char *bp;
	int c;
	Symbol *sym;

	for (bp = yytext; bp < &yytext[IDENTSIZ]; *bp++ = c) {
		if (!isalnum(c = getchar()) && c != '_')
			break;
	}
	if (bp == &yytext[IDENTSIZ])
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(c, stdin);

	sym = yylval.sym = lookup(yytext, lex_ns);
	if (!sym || sym->token == IDEN)
		return IDEN;
	yylval.token = sym->u.token;
	return sym->token;
}

static uint8_t
follow(int expect, int ifyes, int ifno)
{
	int c = getchar();

	if (c == expect) {
		yytext[1] = c;
		yytext[2] = 0;
		return ifyes;
	}
	ungetc(c, stdin);
	return ifno;
}

static uint8_t
minus(void)
{
	int c = getchar();

	yytext[1] = c;
	yytext[2] = '\0';
	switch (c) {
	case '-': return DEC;
	case '>': return INDIR;
	case '=': return SUB_EQ;
	default:
		yytext[1] = '\0';
		ungetc(c, stdin);
		return '-';
	}
}

static uint8_t
plus(void)
{
	int c = getchar();

	yytext[1] = c;
	yytext[2] = '\0';
	switch (c) {
	case '+': return INC;
	case '=': return ADD_EQ;
	default:
		yytext[1] = '\0';
		ungetc(c, stdin);
		return '+';
	}
}

static uint8_t
relational(uint8_t op, uint8_t equal, uint8_t shift, uint8_t assig)
{
	int c = getchar();

	yytext[1] = c;
	yytext[2] = '\0';

	if (c == '=')
		return equal;
	if (c == op)
		return follow('=', assig, shift);
	ungetc(c, stdin);
	yytext[1] = '\0';
	return op;
}

static uint8_t
logic(uint8_t op, uint8_t equal, uint8_t logic)
{
	int c = getchar();

	yytext[1] = c;
	yytext[2] = '\0';

	if (c == '=')
		return equal;
	if (c == op)
		return logic;
	ungetc(c, stdin);
	yytext[1] = '\0';
	return op;
}

static uint8_t
dot(void)
{
	int c;

	if ((c = getchar()) != '.') {
		ungetc(c, stdin);
		return '.';
	} else if ((c = getchar()) != '.') {
		error("incorrect token '%s'", yytext);
	} else {
		yytext[2] = yytext[1] = '.';
		yytext[3] = '\0';
		return ELLIPSIS;
	}
}

static uint8_t
operator(void)
{
	uint8_t c = getchar();

	yytext[0] = c;
	yytext[1] = '\0';
	switch (c) {
	case '<': return relational('<', LE, SHL, SHL_EQ);
	case '>': return relational('>', GE, SHR, SHR_EQ);
	case '&': return logic('&', AND_EQ, AND);
	case '|': return logic('|', OR_EQ, OR);
	case '=': return follow('=', EQ, '=');
	case '^': return follow('=', XOR_EQ, '^');
	case '*': return follow('=', MUL_EQ, '*');
	case '/': return follow('=', DIV_EQ, '/');
	case '!': return follow('=', NE, '!');
	case '-': return minus();
	case '+': return plus();
	case '.': return dot();
	default: return c;
	}
}

static int
skipspaces(void)
{

	int c;

	while (isspace(c = getchar())) {
		if (c == '\n')
			++linenum;
	}
	return c;
}

uint8_t
next(void)
{
	int c;

	ungetc(c = skipspaces(), stdin);

	if (isalpha(c) || c == '_') {
		yytoken = iden();
	} else if (isdigit(c)) {
		yytoken = number();
	} else if (c == '"') {
		yytoken = string();
	} else if (c == '\'') {
		yytoken = character();
	} else if (c == EOF) {
		strcpy(yytext, "EOF");
		yytoken = EOFTOK;
	} else {
		yytoken = operator();
	}
	return yytoken;
}

/* TODO: Remove calls to expect without a character */
void
expect(uint8_t tok)
{
	if (yytoken != tok)
		softerror("expected '%c' before '%s'", tok, yytext);
	else
		next();
}

uint8_t
ahead(void)
{
	int c;
	
	ungetc(c = skipspaces(), stdin);

	return c;
}

void
lexfile(const char *file)
{

	if (file == NULL) {
		filename = "(stdin)";
	} else {
		if (!freopen(file, "r", stdin))
			die("file '%s' not found", file);
		filename = file;
	}
	linenum = 1;
}
