#! /usr/bin/env ruby
# 2014 by Chaofan Li <chaof@tamu.edu>

require_relative 'ILP'

def ilp(a, op, b, c, m, method)
#solve the ILP problem
#
# min(max)  c   x
#           a   x   op   b
#           int x
#               x   >=   0

	self.send(method, a, op, b, c, m)
end 
