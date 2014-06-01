#! /usr/bin/env  ruby

require './ilp.rb'

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
min = false

print ilp(a, op, b, c, min, :lpsolve), "\n"
