/***************************************
   2014 by Chaofan LI <chaof@tamu.edu>
***************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "ruby.h"
#include "extconf.h"

#define round(x,e) ((x)>=0?(long)((x)+(e)):(long)((x)-(e)))
#define __LE 1
#define __EQ 3
#define __GE 2


//#define DEBUG


#ifdef  HAVE_ILCPLEX_CPLEX_H
#include <ilcplex/cplex.h>
static void free_and_null (char **ptr){
	if ( *ptr != NULL ) {
		free (*ptr);
		*ptr = NULL;
	}
}

/* Solve the Integer Linear Programming (ILP) Problem,
 * using CPLEX
 *
 *  min (max)    c    x
 *               A    x    op    b
 *               Int  x
 *                    x    >=    0
 */
static VALUE cplex(VALUE self, VALUE A, VALUE op, VALUE b, VALUE c, VALUE m_symbol){
	Check_Type(A, T_ARRAY) ;
	Check_Type(op, T_ARRAY) ;
	Check_Type(b, T_ARRAY) ;
	Check_Type(c, T_ARRAY) ;
	int Nrow = (int)RARRAY_LEN(A); 
	int Ncolumn = (int)RARRAY_LEN(c);
	int i ;
	
	VALUE ret_hash = rb_hash_new();
	VALUE constraints = rb_ary_new2(Nrow);
	VALUE variables = rb_ary_new2(Ncolumn);

	bool error_set = false ;
	VALUE error_type = rb_eFatal ;
	const char *error_msg = NULL ;
	FILE *log = fopen("./CPX.log", "w") ;
	

	char zprobname[] = "N/A" ;
	int objsen      = 0;
	double *zobj    = (double *) malloc(Ncolumn * sizeof(double));
	double *zrhs    = (double *)malloc(Nrow * sizeof *zrhs);
	char *zsense    = (char *)malloc(Nrow * sizeof *zsense) ;
	int *zmatbeg    = (int *)malloc(Ncolumn * sizeof *zmatbeg) ;
	int *zmatcnt    = (int *)malloc(Ncolumn * sizeof *zmatcnt) ;
	int *zmatind    = (int *)malloc(Nrow * Ncolumn * sizeof *zmatind) ;
	double *zmatval = (double *)malloc(Nrow * Ncolumn * sizeof *zmatval) ;
	double *zlb     = (double *) malloc(Ncolumn * sizeof *zlb);
	double *zub     = (double *) malloc(Ncolumn * sizeof *zub);
	char *zctype    = (char *)malloc(Ncolumn * sizeof * zctype) ;
	int status      = 0 ;

	/* Declare and allocate space for the variables and arrays where we will
	   store the optimization results including the status, objective value,
	   variable values, and row slacks. */

	int      solstat;
	double   objval;
	double *x       = (double *)malloc(Ncolumn * sizeof *x);
	double *slack   = (double *)malloc(Nrow * sizeof *slack) ;
	int cur_numrows, cur_numcols ;

	CPXENVptr     env = NULL;
	CPXLPptr      lp = NULL;


	if(TYPE(m_symbol) == T_SYMBOL){
		ID m =  rb_to_id (m_symbol)  ;
		if( m == rb_intern("min") ){
			objsen = CPX_MIN ;
		}else{if(m == rb_intern("max") ){
			objsen = CPX_MAX ;}}
	}
	
	if(RARRAY_LEN(op) != Nrow){
		error_set = true ;error_type = rb_eFatal; error_msg ="arguments does not match: op != Nrow";
		goto TERMINATE ;
	}

	if(RARRAY_LEN(b) != Nrow){
		error_set = true ;error_type = rb_eFatal; error_msg ="arguments does not match: b != Nrow";
		goto TERMINATE ;
	}

	for(i = 0; i < Nrow; i++){
		VALUE row_v = rb_ary_entry(A, i);
		int j;
		int constraint_type ;
		Check_Type(row_v, T_ARRAY);
		if(RARRAY_LEN(row_v) != Ncolumn){
			error_set = true ;error_type = rb_eFatal; error_msg ="arguments does not match: row_v != Ncolumn";
			goto TERMINATE ;
		}
		for(j = 0; j < Ncolumn; j++){
			zmatind[j * Nrow + i] = i ;
			zmatval[j * Nrow + i] = NUM2DBL(rb_ary_entry(row_v, j));
			
		}
		constraint_type = FIX2INT(rb_ary_entry(op,i));
		switch(constraint_type){
			case __LE : 
			zsense[i] = 'L' ;
			break;
			
			case __EQ :
			zsense[i] = 'E' ;
			break ;
			
			case __GE :
			zsense[i] = 'G' ;
			break ;
			
			default :
			error_set = true ;error_type = rb_eFatal; error_msg ="unknow contstraint type";
			goto TERMINATE ;
			break ;
		}
		zrhs[i] = NUM2DBL(rb_ary_entry(b, i));
	}
	for(i = 0; i < Ncolumn; i++){
		zmatbeg[i] = i * (int)Nrow ;
		zmatcnt[i] = (int)Nrow ;
		zlb[i] = 0.0 ;
		zub[i] = CPX_INFBOUND ;
		zctype[i] = 'I' ;
		zobj[i] = NUM2DBL(rb_ary_entry(c, i));
	}
	
	env = CPXopenCPLEX (&status);

	 /* If an error occurs, the status value indicates the reason for
	    failure.  A call to CPXgeterrorstring will produce the text of
	    the error message.  Note that CPXopenCPLEX produces no output,
	    so the only way to see the cause of the error is to use
	    CPXgeterrorstring.  For other CPLEX routines, the errors will
	    be seen if the CPXPARAM_ScreenOutput indicator is set to CPX_ON.  */

	if ( env == NULL ) {
		char  errmsg[CPXMESSAGEBUFSIZE];
		CPXgeterrorstring (env, status, errmsg);
		fprintf(stderr, "%s", errmsg) ;
		error_set = true ;error_type = rb_eFatal; error_msg ="Could not open CPLEX environment";
		goto TERMINATE ;
	 }

	/* Turn on output to the screen */
	#ifdef DEBUG
	status = CPXsetintparam (env, CPXPARAM_ScreenOutput, CPX_ON);
	#else
	status = CPXsetintparam (env, CPXPARAM_ScreenOutput, CPX_OFF);
	if(log != NULL){
		status = CPXsetlogfile (env, log) ;}

	#endif
	if ( status ) {
		error_set = true ;error_type = rb_eFatal; error_msg ="Failure to turn on screen indicator, error";
		goto TERMINATE ;
	}

	/* Create the problem. */   
	lp = CPXcreateprob (env, &status, zprobname);

	/* A returned pointer of NULL may mean that not enough memory
           was available or there was some other problem.  In the case of
           failure, an error message will have been written to the error
           channel from inside CPLEX.  In this example, the setting of
           the parameter CPXPARAM_ScreenOutput causes the error message to
           appear on stdout.  */
	if ( lp == NULL ) {
		error_set = true ;error_type = rb_eFatal; error_msg ="Failed to create LP";
		goto TERMINATE ;
	}

	/* Now copy the problem data into the lp */
	status = CPXcopylp	(env, lp, (int)Ncolumn, (int)Nrow, objsen, zobj, zrhs, zsense, 
				zmatbeg, zmatcnt, zmatind, zmatval, zlb, zub, NULL) ;
	if(status){
		error_set = true ;error_type = rb_eFatal; error_msg = "Failed to copy problem data" ;
		goto TERMINATE ;
	}
	/* Now copy the ctype array */ 
	status = CPXcopyctype (env, lp, zctype);
	if ( status ) {
		error_set = true ;error_type = rb_eFatal; error_msg = "Failed to copy ctype" ;
		goto TERMINATE ;
	}
	/* Optimize the problem and obtain solution. */
	status = CPXmipopt (env, lp); 
	if ( status ) { 
		error_set = true ;error_type = rb_eFatal; error_msg = "Failed to optimize MIP" ;
		goto TERMINATE ;
	}
	solstat = CPXgetstat (env, lp);
	/* Write the output to the screen. */  
	#ifdef DEBUG
	printf ("\nSolution status = %d\n", solstat);
	#endif
	status = CPXgetobjval (env, lp, &objval);
	if ( status && solstat ) {  
		error_set = true ;error_type = rb_eFatal; error_msg = "No MIP objective value available.  Exiting..." ;
		goto TERMINATE ;
	}
	#ifdef DEBUG
	printf ("Solution value  = %f\n\n", objval);
	#endif
	rb_hash_aset(ret_hash, ID2SYM(rb_intern("o")), DBL2NUM( objval) ); 
	/* The size of the problem should be obtained by asking CPLEX what
           the actual size is, rather than using what was passed to CPXcopylp.
           cur_numrows and cur_numcols store the current number of rows and
           columns, respectively.  */
	cur_numrows = CPXgetnumrows (env, lp);
	cur_numcols = CPXgetnumcols (env, lp);
	status = CPXgetx (env, lp, x, 0, cur_numcols-1);
	if ( status ) { 
		error_set = true ;error_type = rb_eFatal; error_msg = "Failed to get optimal integer x." ;
		goto TERMINATE ;
	}
	status = CPXgetslack (env, lp, slack, 0, cur_numrows-1); 
	if ( status ) {      
		error_set = true ;error_type = rb_eFatal; error_msg = "Failed to get optimal slack values" ;
		goto TERMINATE ;
	}
	for (i = 0; i < cur_numrows; i++) {
		rb_ary_store(constraints, i , DBL2NUM( slack[i]  ) );
		#ifdef DEBUG
		printf ("Row %d:  Slack = %f\n", i, slack[i]); 
		#endif
	}
	rb_hash_aset(ret_hash, ID2SYM(rb_intern("s")), constraints);
	for (i = 0; i < cur_numcols; i++){
		rb_ary_store(variables, i, INT2FIX( round(x[i], 0.5)  ) ) ;
		#ifdef DEBUG
		printf ("Column %d:  Value = %f\n", i, x[i]); 
		#endif
	}
	rb_hash_aset(ret_hash, ID2SYM(rb_intern("v")), variables);
	/* Finally, write a copy of the problem to a file. */
	status = CPXwriteprob (env, lp, "cplex.lp", NULL);  
	if ( status ) {
		#ifdef DEBUG
		fprintf (stderr, "Failed to write LP to disk.\n");
		#endif
	}
	
	/* Free up the problem as allocated by CPXcreateprob, if necessary */
TERMINATE:
	if ( lp != NULL ) {
		status = CPXfreeprob (env, &lp);
		if ( status ) {
			fprintf (stderr, "CPXfreeprob failed, error code %d.\n", status);
		}
	}

	/* Free up the CPLEX environment, if necessary */

	if ( env != NULL ) {
		status = CPXcloseCPLEX (&env);

		/* Note that CPXcloseCPLEX produces no output,
		   so the only way to see the cause of the error is to use
		   CPXgeterrorstring.  For other CPLEX routines, the errors will
		   be seen if the CPXPARAM_ScreenOutput indicator is set to CPX_ON. */

		if ( status ) {
			char  errmsg[CPXMESSAGEBUFSIZE];
			CPXgeterrorstring (env, status, errmsg);
			fprintf(stderr, "%s", errmsg) ;
			error_set = true ;error_type = rb_eFatal; error_msg = "Could not close CPLEX environment" ;
		}
	}
	/* Free up the log file */
	if( log != NULL){
		fclose(log) ;}

	/* Free up the problem data arrays, if necessary. */

	free_and_null ((char **) &zobj);
	free_and_null ((char **) &zrhs);
	free_and_null ((char **) &zsense);
	free_and_null ((char **) &zmatbeg);
	free_and_null ((char **) &zmatcnt);
	free_and_null ((char **) &zmatind);
	free_and_null ((char **) &zmatval);
	free_and_null ((char **) &zlb);
	free_and_null ((char **) &zub);
	free_and_null ((char **) &zctype);
	
	free_and_null ((char **) &x) ;
	free_and_null ((char **) &slack) ;

	if(error_set == true){
		rb_raise(error_type, "%s", error_msg);
	}else{
		return ret_hash ;
	}
}
#endif

#ifdef HAVE_LPSOLVE_LP_LIB_H 
#include <lpsolve/lp_lib.h>
/* Solve the Integer Linear Programming (ILP) Problem,
 * using lp_solve
 *
 *  min (max)    c    x
 *               A    x    op    b
 *               Int  x
 *                    x    >=    0
 */

static VALUE lpsolve(VALUE self, VALUE A, VALUE op, VALUE b, VALUE c, VALUE m_symbol){
	Check_Type(A, T_ARRAY) ;
	Check_Type(op, T_ARRAY) ;
	Check_Type(b, T_ARRAY) ;
	Check_Type(c, T_ARRAY) ;
	int Nrow = (int)RARRAY_LEN(A); 
	int Ncolumn = (int)RARRAY_LEN(c);
	int i;
	int ret ;
	REAL row[1 + Ncolumn] ;
	REAL result[1 + Nrow + Ncolumn];
	REAL b_dbl[Nrow] ;

	VALUE ret_hash = rb_hash_new();
	VALUE constraints = rb_ary_new2(Nrow);
	VALUE variables = rb_ary_new2(Ncolumn);

	lprec *lp = NULL;
	lp = make_lp(0, (int)Ncolumn);
	set_verbose(lp, SEVERE);

	if(TYPE(m_symbol) == T_SYMBOL){
		ID m =  rb_to_id (m_symbol)  ;
		if( m == rb_intern("min") ){
			set_minim(lp);
		}else{if(m == rb_intern("max") ){
			set_maxim(lp);}}
	}
	
	if(RARRAY_LEN(op) != Nrow){
		rb_raise(rb_eArgError, "Length of op does not match that of A");
	}

	if(RARRAY_LEN(b) != Nrow){
		rb_raise(rb_eArgError, "Length of b does not match that of A");
	}

	set_add_rowmode(lp, true);
	#ifdef DEBUG
	printf("number of constraints: %d\n", Nrow);
	#endif
	for(i = 0; i < Nrow; i++){
		VALUE row_v = rb_ary_entry(A, i);
		int j;
		int constraint_type ;
		Check_Type(row_v, T_ARRAY);
		if(RARRAY_LEN(row_v) != Ncolumn){
			rb_raise(rb_eArgError, "Length of row %d :%ld doen not match that of c:%d, i.e., the objective", i + 1, RARRAY_LEN(row_v) ,Ncolumn);
		}
		for(j = 0; j < Ncolumn; j++){
			row[j + 1] = NUM2DBL(rb_ary_entry(row_v, j));
		}
		constraint_type = FIX2INT(rb_ary_entry(op,i));
		b_dbl[i] = NUM2DBL(rb_ary_entry(b, i));
		add_constraint(lp, row, constraint_type, b_dbl[i]); 
	}
	set_add_rowmode(lp, false);
	#ifdef  DEBUG
	printf("number of variables: %d\n", Ncolumn);
	#endif
	for(i = 0; i < Ncolumn; i++){
		row[i + 1] = NUM2DBL(rb_ary_entry(c, i));
		set_int(lp, i + 1, true);
	}
	
	set_obj_fn(lp, row);
	#ifdef DEBUG
	printf("start solve\n");
	#endif
	ret = solve(lp);
	#ifdef DEBUG
	printf("solve return value: %d \n", ret);
	#endif
	switch(ret){
		case 2: rb_raise(rb_eFatal, "no solution");
		break ;

		default:
		break;
	}
	get_primal_solution(lp, result);
	#ifdef DEBUG

	for(i = 0; i < 1+get_Nrows(lp)+get_Ncolumns(lp); i++){
		if(i == 0){
			printf("obj: ");
		}else{if(i >= 1 && i <= get_Nrows(lp)){
			printf("constraints %d: ", i);
		}else{if(i >= 1 + get_Nrows(lp) && i <= get_Nrows(lp) + get_Ncolumns(lp)){
			printf("variable %d: ", i - get_Nrows(lp));
		}}}
		printf("%f\n", result[i]);
	}
	#endif	
	for(i = 1; i < 1 + get_Nrows(lp) + get_Ncolumns(lp); i++){
		if(i >= 1 && i <= get_Nrows(lp)){
			double slack = b_dbl[i - 1] - result[i];
			rb_ary_store(constraints, i - 1, DBL2NUM(slack >= 0 ? slack : -slack)) ;
		}else{if(i >= 1 + get_Nrows(lp) && i <= get_Nrows(lp) + get_Ncolumns(lp)){
			rb_ary_store(variables, i - 1 - get_Nrows(lp), INT2NUM(round(result[i], 0.5) ));
		}}
	}

	rb_hash_aset(ret_hash, ID2SYM(rb_intern("o")), DBL2NUM(result[0]));
	rb_hash_aset(ret_hash, ID2SYM(rb_intern("s")), constraints);
	rb_hash_aset(ret_hash, ID2SYM(rb_intern("v")), variables);
	
	delete_lp(lp);
	return ret_hash;

}
#endif

#ifdef HAVE_GUROBI_GUROBI_C_H 
#include "gurobi/gurobi_c.h"

VALUE gurobi(VALUE self, VALUE A, VALUE op, VALUE b, VALUE c, VALUE m_symbol){
	Check_Type(A, T_ARRAY) ;
	Check_Type(op, T_ARRAY) ;
	Check_Type(b, T_ARRAY) ;
	Check_Type(c, T_ARRAY) ;
	int Nrow = (int)RARRAY_LEN(A); 
	int Ncolumn = (int)RARRAY_LEN(c);
	int i;

	VALUE ret_hash = rb_hash_new();
	VALUE constraints = rb_ary_new2(Nrow);
	VALUE variables = rb_ary_new2(Ncolumn);

	GRBenv   *env   = NULL;
	GRBmodel *model = NULL;
	int       error = 0;
	double    slack[Nrow] ;
	double    *sol = (double *)calloc(Ncolumn, sizeof *sol);
	int       ind[Ncolumn];
	double    val[Ncolumn];
	double    obj[Ncolumn];
	char      vtype[Ncolumn];
	int       optimstatus;
	double    objval;

	/* Create environment */

	error = GRBloadenv(&env, "GRB.log");
	if (error) goto QUIT;

	/* Create an empty model */

	error = GRBnewmodel(env, &model, "N/A", 0, NULL, NULL, NULL, NULL, NULL);
	if (error) goto QUIT;


	/* Add variables */
	#ifdef  DEBUG
	printf("number of variables: %d\n", Ncolumn);
	#endif
	for(i = 0; i < Ncolumn; i++){
		obj[i] = NUM2DBL(rb_ary_entry(c, i));
		vtype[i] = GRB_INTEGER ;
	}
  	error = GRBaddvars(model, Ncolumn, 0, NULL, NULL, NULL, obj, NULL, NULL, vtype, NULL);
	if (error) goto QUIT;

	/* Change objective sense to maximization */
	if(TYPE(m_symbol) == T_SYMBOL){
		ID sense =  rb_to_id (m_symbol)  ;
		if( sense == rb_intern("min") ){
  			error = GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MINIMIZE);
		}else{if(sense == rb_intern("max") ){
  			error = GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MAXIMIZE);
		}}
	}
	error = GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MAXIMIZE);
	if (error) goto QUIT;

	/* Integrate new variables */
  	error = GRBupdatemodel(model);if (error) goto QUIT;

	#ifdef DEBUG
	printf("number of constraints: %d\n", Nrow);
	#endif
	for(i = 0; i < Nrow; i++){
		VALUE row_v = rb_ary_entry(A, i);
		int j, constraint_type, nz_count = 0;
		char zsense ;
		REAL b_dbl;
		Check_Type(row_v, T_ARRAY);
		if(RARRAY_LEN(row_v) != Ncolumn){
			rb_raise(rb_eArgError, 
			"Length of row %d :%ld doen not match that of c:%d, i.e., the objective", 
			i + 1, RARRAY_LEN(row_v) ,Ncolumn);
		}
		for(j = 0; j < Ncolumn; j++){
			val[j] = NUM2DBL(rb_ary_entry(row_v, j));
			if (val[j] != 0.0){
				ind[nz_count++] = j ;
			}
			//ind[j] = j;
		}
		constraint_type = FIX2INT(rb_ary_entry(op,i));
		switch(constraint_type){
			case LE : 
			zsense = GRB_LESS_EQUAL ;
			break;
			
			case EQ :
			zsense = GRB_EQUAL ;
			break ;
			
			case GE :
			zsense = GRB_GREATER_EQUAL ;
			break ;
			
			default :
			fprintf(stderr, "unknow contstraint type");
			goto QUIT ;
			break ;
		}	
		b_dbl = NUM2DBL(rb_ary_entry(b, i));
		error = GRBaddconstr(model, nz_count, ind, val, zsense, b_dbl, NULL);
		if (error){
			fprintf(stderr, "error encountered when adding constraint %d\n", i) ;
			goto QUIT;
		}
	}
	/* Optimize model */
  	error = GRBoptimize(model);if (error) goto QUIT;
  	/* Write model to 'mip1.lp' */
	error = GRBwrite(model, "gurobi.lp");if (error) goto QUIT;
	/* Capture solution information */
	error = GRBgetintattr(model, GRB_INT_ATTR_STATUS, &optimstatus);
	if (error) goto QUIT;

	error = GRBgetdblattr(model, GRB_DBL_ATTR_OBJVAL, &objval);
	if (error) goto QUIT;

	error = GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, Ncolumn, sol);
	if (error) goto QUIT;

	error = GRBgetdblattrarray(model, GRB_DBL_ATTR_SLACK, 0, Nrow, slack);
	if (error) goto QUIT;

	#ifdef DEBUG
	printf("\nOptimization complete\n");
	#endif
	
	if (optimstatus == GRB_OPTIMAL) {
		rb_hash_aset(ret_hash, ID2SYM(rb_intern("o")), rb_float_new( objval) ); 
		#ifdef DEBUG
		printf("Optimal objective: %.4e\n", objval);
		#endif
		for(i = 0; i < Ncolumn; i++){
			rb_ary_store(variables, i, INT2NUM(round(sol[i], 0.5) ) );
			#ifdef DEBUG
			printf("Column %d: %d\n", i, (int)round(sol[i], 0.5));
			#endif
		}
		for(i = 0; i < Nrow; i++){
			rb_ary_store(constraints, i, DBL2NUM(slack[i]) ) ;
			#ifdef DEBUG
			printf("slack %d: %f\n", i, slack[i]);
			#endif
		}
	} else if (optimstatus == GRB_INF_OR_UNBD) {
		#ifdef DEBUG
		printf("Model is infeasible or unbounded\n");
		#endif
	} else {
		#ifdef DEBUG
		printf("Optimization was stopped early\n");
		#endif
	}
	rb_hash_aset(ret_hash, ID2SYM(rb_intern("v")), variables);
	rb_hash_aset(ret_hash, ID2SYM(rb_intern("s")), constraints);

QUIT:
	/* Error reporting */
	if (error) {
		GRBfreemodel(model);
		GRBfreeenv(env);
		free(sol);
		rb_raise(rb_eFatal, "%s",  GRBgeterrormsg(env)) ;
	}
	/* Free all */
	GRBfreemodel(model);
	GRBfreeenv(env);
	free(sol);
	return ret_hash;
}
#endif

void Init_ILP(){
	#ifdef HAVE_LPSOLVE_LP_LIB_H
	rb_define_method(rb_cObject, "lpsolve", lpsolve, 5);
	#endif
	#ifdef HAVE_ILCPLEX_CPLEX_H
	rb_define_method(rb_cObject, "cplex", cplex, 5);
	#endif
	#ifdef HAVE_GUROBI_GUROBI_C_H 
	rb_define_method(rb_cObject, "gurobi", gurobi, 5);
	#endif
	rb_define_global_const( "LE", INT2FIX(1)) ;
	rb_define_global_const( "GE", INT2FIX(2)) ;
	rb_define_global_const( "EQ", INT2FIX(3)) ;
}
