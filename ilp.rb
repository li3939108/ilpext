#! /usr/bin/env ruby

require './ILP.so'

def ilp(a, op, b, c, m, method)
	self.send(method, a, op, b, c, m)
end 
