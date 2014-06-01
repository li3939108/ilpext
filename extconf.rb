require 'mkmf'
#  2014 by Chaofan Li <chaof@tamu.edu>

lpsolve = 'lpsolve'
cplex = 'cplex'
have_lpsolve = false
have_cplex = false
Dir.entries("./lib").each{|file|
	if(File.exist?('./lib/' + file) and file[3, 7] == 'lpsolve')
		lpsolve = file.split('.')[0][3..-1]
	elsif(File.exist?('./lib/' + file) and file[3, 5] == 'cplex')
		cplex = file.split('.')[0][3..-1]
	end
}
dir_config(lpsolve, './include', './lib') 
dir_config(cplex, './include', './lib') 
have_cplex =  have_library(lpsolve, 'strcpy') and have_header('lpsolve/lp_lib.h') 
have_lpsolve = have_library(cplex, 'CPXcopylp') and have_header('ilcplex/cplex.h') 
if	(have_lpsolve or have_cplex) and
	have_library('pthread', 'pthread_create') and have_header('pthread.h') 

	RPATHFLAG << " -Wl,-rpath,./lib"
	create_header
	create_makefile('ILP')
end
