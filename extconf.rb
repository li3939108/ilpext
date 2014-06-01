require 'mkmf'

#ruby extconf.rb --with-DFG_ILP-dir=dir
#set the lib and include directory associated with DFG_ILP to dir/lib dir/include
#ruby extconf.rb
#will set the lib and include directory associated with DFG_ILP to ./lib and ./include
#ruby extconf.rb --with-DFG_ILP-lib=lib will set the lib directory to lib
#ruby extconf.rb --with-DFG_ILP-include=include will set the include directory to include
#lpsolve_lib = './lib'
dir_config('lpsolve55', './include', './lib')
dir_config('cplex1260', './include', './lib')


if	have_header('lp_lib.h') and have_library('lpsolve55', 'strcpy') and 
	have_header('ilcplex/cplex.h') and have_library('cplex1260', 'CPXcopylp') 
	have_header('pthread.h') and have_library('pthread', 'pthread_create')

	RPATHFLAG << " -Wl,-rpath,./lib"
	create_makefile('ILP')
end
