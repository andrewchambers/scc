/* See LICENSE file for copyright and license details. */

/*
name: TEST027
description: Test of cpp stringizer
error:
output:
G3	I	F	"main
{
\
A5	P	"p
V7	K	#N19
Y6	V7	"	(
	#"hello is better than bye
	#K00
)
	A5	Y6	'P	:P
	h	A5	@K	gI
}
*/

#define x(y) #y

int
main(void)
{
	char *p;
	p = x(hello)  " is better than bye";

	return *p;
}
