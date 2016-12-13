/* See LICENSE file for copyright and license details. */

/*
name: TEST061
description: Test for macros without arguments but with parenthesis
error:
output:
V2	K	#NC
V4	K	#N9
G7	I	F	"main
{
\
	h	#I1
}
*/

#define X (2)
#define L (0)
#define H (1)
#define Q(x) x

int
main(void)
{
	return X == L + H + Q(1);
}
