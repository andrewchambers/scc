
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sizes.h>
#include <cc.h>
#include "cc1.h"

static FILE *yyin;
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
	switch (ch = toupper(getc(yyin))) {
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
		ungetc(ch, yyin);
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
	register char ch, *bp;
	static char base;

	if ((ch = getc(yyin)) == '0') {
		if (toupper(ch = getc(yyin)) == 'X') {
			base = 16;
		} else {
			base = 8;
			ungetc(ch, yyin);
		}
	} else {
		base = 10;
		ungetc(ch, yyin);
	}

	for (bp = yytext ; bp < &yytext[IDENTSIZ]; *bp++ = ch) {
		ch = getc(yyin);
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
	ungetc(ch, yyin);
	return integer(yytext, base);
}

static char *
escape(char *s)
{
	char c;

repeat:
	switch (getc(yyin)) {
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
		if ((c = getc(yyin)) == '\\')
			goto repeat;
		break;
	default:
		warn(1, "unknown escape sequence");
		return s;
	}

	*s = c;
	return ++s;
}

static uint8_t
character(void)
{
	static char c;
	register Symbol *sym;

	getc(yyin);   /* discard the initial ' */
	c = getc(yyin);
	if (c == '\\')
		escape(&c);
	if (getc(yyin) != '\'')
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
	register char *bp;
	register int c;
	static Symbol *sym;

	getc(yyin); /* discard the initial " */

	for (bp = buf; bp < &buf[STRINGSIZ]; ) {
		switch (c = getc(yyin)) {
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
	sym->type = mktype(chartype, PTR, 0);
	yylval.sym = sym;
	return STRING;
}

void
init_keywords(void)
{
	static struct {
		char *str;
		uint8_t token, value;
	} *bp, buff[] = {
		{"auto", SCLASS, AUTO},
		{"break", BREAK, BREAK},
		{"_Bool", TYPE, BOOL},
		{"case", CASE, CASE},
		{"char", TYPE, CHAR},
		{"const", TQUALIFIER, CONST},
		{"continue", CONTINUE, CONTINUE},
		{"default", DEFAULT, DEFAULT},
		{"do", DO, DO},
		{"double", TYPE, DOUBLE},
		{"else", ELSE, ELSE},
		{"enum", TYPE, ENUM},
		{"extern", SCLASS, EXTERN},
		{"float", TYPE, FLOAT},
		{"for", FOR, FOR},
		{"goto", GOTO, GOTO},
		{"if", IF, IF},
		{"int", TYPE, INT},
		{"long", TYPE, LONG},
		{"register", SCLASS, REGISTER},
		{"restrict", TQUALIFIER, RESTRICT},
		{"return", RETURN, RETURN},
		{"short", TYPE, SHORT},
		{"signed", TYPE, SIGNED},
		{"sizeof", SIZEOF, SIZEOF},
		{"static", SCLASS, STATIC},
		{"struct", TYPE, STRUCT},
		{"switch", SWITCH, SWITCH},
		{"typedef", SCLASS, TYPEDEF},
		{"union", TYPE, UNION},
		{"unsigned", TYPE, UNSIGNED},
		{"void", TYPE, VOID},
		{"volatile", TQUALIFIER, VOLATILE},
		{"while", WHILE, WHILE},
		{NULL, 0, 0},
	};
	register Symbol *sym;
	extern short symid;

	for (bp = buff; bp->str; ++bp) {
		sym = install(bp->str, NS_IDEN);
		sym->token = bp->token;
		sym->u.token = bp->value;
	}
	symid = 0;
}

static uint8_t
iden(void)
{
	register char *bp;
	register int c;
	register Symbol *sym;

	for (bp = yytext; bp < &yytext[IDENTSIZ]; *bp++ = c) {
		if (!isalnum(c = getc(yyin)) && c != '_')
			break;
	}
	if (bp == &yytext[IDENTSIZ])
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(c, yyin);

	sym = yylval.sym = lookup(yytext, NS_IDEN);
	if (!sym || sym->token == IDEN)
		return IDEN;
	yylval.token = sym->u.token;
	return sym->token;
}

static uint8_t
follow(int expect, int ifyes, int ifno)
{
	register int c = getc(yyin);

	if (c == expect) {
		yytext[1] = c;
		yytext[2] = 0;
		return ifyes;
	}
	ungetc(c, yyin);
	return ifno;
}

static uint8_t
minus(void)
{
	register int c = getc(yyin);

	yytext[1] = c;
	yytext[2] = '\0';
	switch (c) {
	case '-': return DEC;
	case '>': return INDIR;
	case '=': return SUB_EQ;
	default:
		yytext[1] = '\0';
		ungetc(c, yyin);
		return '-';
	}
}

static uint8_t
plus(void)
{
	register int c = getc(yyin);

	yytext[1] = c;
	yytext[2] = '\0';
	switch (c) {
	case '+': return INC;
	case '=': return ADD_EQ;
	default:
		yytext[1] = '\0';
		ungetc(c, yyin);
		return '+';
	}
}

static uint8_t
relational(uint8_t op, uint8_t equal, uint8_t shift, uint8_t assig)
{
	register int c = getc(yyin);

	yytext[1] = c;
	yytext[2] = '\0';

	if (c == '=')
		return equal;
	if (c == op)
		return follow('=', assig, shift);
	ungetc(c, yyin);
	yytext[1] = '\0';
	return op;
}

static uint8_t
logic(uint8_t op, uint8_t equal, uint8_t logic)
{
	register int c = getc(yyin);

	yytext[1] = c;
	yytext[2] = '\0';

	if (c == '=')
		return equal;
	if (c == op)
		return logic;
	ungetc(c, yyin);
	yytext[1] = '\0';
	return op;
}

static uint8_t
dot(void)
{
	int c;

	if ((c = getc(yyin)) != '.') {
		ungetc(c, yyin);
		return '.';
	} else if ((c = getc(yyin)) != '.') {
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
	register uint8_t c = getc(yyin);

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

	register int c;

	while (isspace(c = getc(yyin))) {
		if (c == '\n')
			++linenum;
	}
	return c;
}

uint8_t
next(void)
{
	register int c;

	ungetc(c = skipspaces(), yyin);

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

void
expect(register uint8_t tok)
{
	if (yytoken != tok)
		unexpected();
	next();
}

uint8_t
ahead(void)
{
	register int c;
	
	ungetc(c = skipspaces(), yyin);

	return c;
}

void
open_file(register const char *file)
{
	if (yyin != NULL)
		fclose(yyin);
	if (file == NULL) {
		yyin = stdin;
		filename = "(stdin)";
	} else {
		if ((yyin = fopen(file, "r")) == NULL)
			die("file '%s' not found", file);
		filename = file;
	}
	linenum = 1;
}

