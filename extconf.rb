require 'mkmf'

#ruby extconf.rb --with-DFG_ILP-dir=dir
#set the lib and include directory associated with DFG_ILP to dir/lib dir/include
#ruby extconf.rb
#will set the lib and include directory associated with DFG_ILP to ./lib and ./include
#ruby extconf.rb --with-DFG_ILP-lib=lib will set the lib directory to lib
#ruby extconf.rb --with-DFG_ILP-include=include will set the include directory to include
#lpsolve_lib = './lib'
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
have_cplex =  have_library(lpsolve, 'strcpy') and have_header('lp_lib.h') 
have_lpsolve = have_library(cplex, 'CPXcopylp') and have_header('ilcplex/cplex.h') 
if	(have_lpsolve or have_cplex) and
	have_library('pthread', 'pthread_create') and have_header('pthread.h') 

	RPATHFLAG << " -Wl,-rpath,./lib"
	create_header
	create_makefile('ILP')
end
