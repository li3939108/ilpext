#! /usr/bin/env ruby

require_relative 'ILP'

def ilp(a, op, b, c, m, method)
	self.send(method, a, op, b, c, m)
end 
