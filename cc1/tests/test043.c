/* See LICENSE file for copyright and license details. */

/*
name: TEST043
description: Test for double typedef (taken from plan9 kernel)
error:
output:
S2	"Clock0link	#N8	#N2
M6	P	"clock	#N0
M8	P	"link	#N2
G9	S2	"cl0
G11	I	F	"main
{
\
	G9	M6	.P	@F	c0
	h	#I0
}
*/

typedef struct Clock0link Clock0link;
typedef struct Clock0link {
	void		(*clock)(void);
	Clock0link*	link;
} Clock0link;



Clock0link cl0;

int
main(void)
{
	(*cl0.clock)();
	return 0;
}
