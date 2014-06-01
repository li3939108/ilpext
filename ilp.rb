#! /usr/bin/env ruby

require_relative 'ILP'

def ilp(a, op, b, c, min, method)
	self.send(method, a, op, b, c, min)
end 
