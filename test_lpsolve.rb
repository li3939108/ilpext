#! /usr/bin/env  ruby

require_relative 'ilp'

#solve the ILP problem

# min(max)  c   x
#           a   x   op   b
#           int x


c=[	0,	1]
a=[	[-1,	1], 
	[3,	2], 
	[2,	3]]
op=[	
	LE, 
	EQ,
	LE]
#rhs
b=[
	1, 
	10, 
	12]
#

print ilp(a, op, b, c, :max, :lpsolve), "\n"
