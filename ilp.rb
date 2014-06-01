#! /usr/bin/env ruby

require './ILP.so' if RUBY_PLATFORM.downcase.include?("linux")
require './ILP.dylib' if RUBY_PLATFORM.downcase.include?("darwin")

def ilp(a, op, b, c, min, method)
	self.send(method, a, op, b, c, min)
end 
