#! /usr/bin/env ruby

require './ILP.so'

def ilp(a, op, b, c, min, method)
	self.send(method, a, op, b, c, min)
end 
