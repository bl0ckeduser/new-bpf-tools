/*
  Miniature scheme interpreter, based on
  https://github.com/bl0ckeduser/wannabe-lisp,
  revision a0659e4f7be0f024a7641486fc30548be00cbcbb,
  dated Fri Jul 12 22:51:03 2013

  Sample code:

(define (square n) (* n n))

(define (abs x)			
	(if (>= x 0)		
		x		
		(- x)))	

(define (fact n)
	(if (= n 0)
		1
		(* n (fact (- n 1)))))

(define (compose f g)
	(lambda (x) (f (g x))))

(define (foldr list func id)
	(if (null? list)
		id
		(func (car list) (foldr (cdr list) func id))))

(define (pair-max a b) (if (> a b) a b))

(define (list-max list)
	(foldr list pair-max 0))

(define (select f)
	(lambda (a b)
		(if (f a)
			(cons a b)
			b)))

(define (filter f list)
	(foldr list (select f) '()))

(define (exclude n list)
	(filter
		(lambda (x) (not (= x n)))
		list))

(define (sorted list)
	(if (null? list)
		'()
		(cons (list-max list)
			(sorted (exclude (list-max list) list)))))

 */

int NULL = 0;
int SYMBOL = 0;
int NUMBER = 1;
int LIST = 2;
int CONS = 3;
int CLOSURE = 4;
int BOOL = 5;
int REF = 0;

typedef struct env {
	int count;
	int alloc;
	char **sym;
	char *ty;
	char **ptr;
	struct env *father;
} env_t;

typedef struct env_ref {
	struct env* e;
	int i;
} env_ref_t;

typedef struct list {
	char head[32];
	int type;
	int val;
	int cc;
	int ca;
	/* XXX: env_t *closure; */
	struct env *closure;
	struct list **c;
} list_t;

env_t *global;

char *c_malloc(size)
{
	char *ptr = malloc(size);
	if (!ptr) {
		printf("Error: malloc(%ld) has failed\n", size);
		exit(1);
	}
	add_ptr(ptr);
	return ptr;
}

list_t *new_list()
{
	list_t* nl = c_malloc(sizeof(list_t));
	nl->type = SYMBOL;
	nl->val = 0;
	nl->ca = 0;
	nl->cc = 0;
	nl->head[0] = 0;
	nl->c = NULL;
	return nl;
}

list_t* mksym(char *s)
{
	list_t *sl = new_list();
	strcpy(sl->head, s);
	sl->type = SYMBOL;
	return sl;
}

list_t* makebool(int cbool)
{
	list_t *b = new_list();
	b->type = BOOL;
	b->val = cbool;
	return b;
}

/*
 * Make a new environment object
 */
env_t* new_env()
{
	env_t *e = c_malloc(sizeof(env_t));

	e->count = 0;
	e->alloc = 8;
	e->sym = c_malloc(e->alloc * sizeof(char *));
	e->ty = c_malloc(e->alloc * sizeof(char));
	e->ptr = c_malloc(e->alloc * sizeof(char *));
	e->father = NULL;

	return e;
}

/*
 * Look up a symbol in an environment-chain
 */
/* 
 * XXX: original version returned a struct,
 * not a pointer, but wannabe compiler doesn't
 * support that
 */
env_ref_t* lookup(env_t *e, char *sym)
{
	int i;
	env_ref_t* ret = malloc(sizeof(env_ref_t));

	/* 
	 * Try to find the symbol in the
	 * current environment 
	 */
	for (i = 0; i < e->count; ++i) {
		if (!strcmp(e->sym[i], sym)) {
			ret->i = i;
			ret->e = e;
			return ret;
		}
	}

	/* 
	 * Otherwise try father env,
	 * falling back on the default
	 * error-value {0, NULL} 
	 */
	ret->i = 0;
	ret->e = NULL;
	if (e->father) {
		free(ret);
		ret = lookup(e->father, sym);
	}

	return ret;
}

/* 
 * (1 2 3 4)
 * => (1 . (2 . (3 . (4 . NIL))))
 */
list_t* makelist(list_t* argl)
{
	list_t *nw, *nw2, *nw3;
	list_t *cons;
	int i;
	
	nw3 = nw = new_list();
	nw->type = CONS;
	for (i = 0; i < argl->cc; ++i) {
		/* recursive application is necessary 
	 	 * so as to make possible cases like:
		 * 		]=> (caadr '(1 (1 2)))
		 * 		1	
		 */
		if (argl->c[i]->type == LIST) {
			cons = makelist(argl->c[i]);
			add_child(nw, cons);
		} else
			add_child(nw, argl->c[i]);
		if ((i + 1) < argl->cc) {
			nw2 = new_list();
			nw2->type = CONS;
			add_child(nw, nw2);
			nw = nw2;
		} else {
			/* put in a nil */
			add_child(nw, mksym("NIL"));
		}
	}
	return nw3;
}

list_t* eval(list_t *l, env_t *env)
{
	env_ref_t* er;
	list_t *argl, *proc;
	list_t *ev;
	list_t *nw, *nw2, *nw3;
	list_t *pred;
	env_t *ne;
	int i;

	/* 
	 * Deal with special forms (lambda, define, ...) first
	 */

	/* (lambda (arg-1 arg-2 ... arg-n) exp1 exp2 ... expN) */
	if (l->type == LIST && !strcmp(l->c[0]->head, "lambda")) {
		nw = new_list();
		nw->type = CLOSURE;
		nw->closure = env;
		for (i = 1; i < l->cc; ++i)
			add_child(nw, l->c[i]);
		return nw;
	}

	/* (define ...) family of special-forms */
	if (l->type == LIST && !strcmp(l->c[0]->head, "define")) {
		/* (define SYMBOL EXP) */
		if (l->cc == 3 && l->c[1]->type == SYMBOL) {
			env_add(env, l->c[1]->head, REF,
				(ev = eval(l->c[2], env)));
			return ev;
		/* 		(define (twice x) (+ x x))
		 * =>	(define twice (lambda (x) (+ x x)))
		 */ 
		} else if (l->cc == 3 && l->c[1]->type == LIST
			&& l->c[1]->cc && l->c[1]->c[0]->type == SYMBOL) {
			proc = l->c[1]->c[0];
			argl = new_list();
			argl->type = LIST;
			for (i = 1; i < l->c[1]->cc; ++i)
				add_child(argl, l->c[1]->c[i]);
			nw = new_list();
			nw->type = CLOSURE;
			nw->closure = env;
			add_child(nw, argl);
			add_child(nw, l->c[2]);
			env_add(env, proc->head, REF,
				nw);
			return nw;
		} else {
			printf("Error: improper use of `define' special form\n");
			exit(1);
		}
	}

	/* (set! var val) */
	if (l->type == LIST && !strcmp(l->c[0]->head, "set!")) {
		/* check arg count */
		if (l->cc != 3) {
			printf("Error: improper use of `set!' special form\n");
			exit(1);
		}

		env_set(env, l->c[1]->head, REF,
			(ev = eval(l->c[2], env)));
	
		return ev;
	}

	/* if special form -- (if BOOL A B) */
	if (l->type == LIST && !strcmp(l->c[0]->head, "if")) {
		/* check arg count */
		if (l->cc != 4) {
			printf("Error: improper use of `if' special form\n");
			exit(1);
		}
		
		/* evaluate the predicate,
		 * ensuring it is of proper type */
		pred = eval(l->c[1], env);
		if (pred->type != BOOL) {
			printf("Error: boolean expected as 1st argument of `if'\n");
			exit(1);
		}

		/* return A or B depending upon predicate */
		if (pred->val) {
			return eval(l->c[2], env);
		} else {
			return eval(l->c[3], env);
		}
	}

	/* cond special form --
	 * (cond (p1 e1) (p2 e2) ... (pN eN)) 
	 * the special predicate-symbol `else' always matches
	 */
	if (l->type == LIST && l->cc >= 2 && l->c[0]->type == SYMBOL
		&& !strcmp(l->c[0]->head, "cond")) {

		for (i = 1; i < l->cc; ++i) {
			if (l->c[i]->cc != 2) {
				printf("Error: arguments to `cond' should be");
				printf(	   " two-element lists...\n");
				exit(1);
			}
			/* deal with `else' special case */
			if (l->c[i]->c[0]->type == SYMBOL
				&& !strcmp(l->c[i]->c[0]->head, "else")) {
				return eval(l->c[i]->c[1], env);
			}
			/* general case */
			pred = eval(l->c[i]->c[0], env);
			if (pred->type != BOOL) {
				printf("Error: boolean expected in `cond'\n");
				exit(1);
			}
			if (pred->val)
				return eval(l->c[i]->c[1], env);
		}

		/* not sure what to return when nothing matches,
		 * I guess `nil' is reasonable ... */
		return mksym("NIL");
	}

	/* (list X Y Z ... ) */
	/* note that unlike for quoted lists, this involves evaluating
	 * the arguments before building the list */
	if (l->type == LIST && l->cc > 1 && l->c[0]->type == SYMBOL
		&& !strcmp(l->c[0]->head, "list")) {
		ev = new_list();
		for (i = 1; i < l->cc; ++i)
			add_child(ev, l->c[i]);
		evlist(ev, env);
		return makelist(ev);	/* cons-ify */
	}

	/* quote */
	if (l->type == LIST && l->cc == 2 && l->c[0]->type == SYMBOL
		&& !strcmp(l->c[0]->head, "QUOTE")) {
		if (l->c[1]->type == SYMBOL)
			return l->c[1];
		else if (l->c[1]->type == LIST) {
			/* cons-ify */
			return makelist(l->c[1]);
		}
	}

	/* 
	 * If the special cases don't match a list,
	 * it's a function application, e.g.
	 *		(+ 1 2 3)
 	 */
	if (l->type == LIST && l->cc) {
		/* group arguments */
		argl = new_list();
		argl->type = LIST;
		for (i = 1; i < l->cc; ++i)
			add_child(argl, l->c[i]);

		/* evaluate arguments */
		evlist(argl, env);

		/* evaluate procedure */
		proc = eval(l->c[0], env);

		/* apply procedure to arguments */
		return apply(proc, argl);
	}

	/* symbol */
	if (l->type == SYMBOL) {
		/* look up, climbing up environment chain */
		er = lookup(env, l->head);
		if (er->e == NULL) {
			printf("Error: unbound variable `%s'\n", l->head);
			exit(1);
		}
		
		return er->e->ptr[er->i];
	}

	return l;
}

/*
 * Primitive operations
 */

/*
 * Add primitive operator name to environment
 */
add_primop(env_t *e, char *sym)
{
	list_t *l = new_list();
	l->type = LIST;
	add_child(l, mksym("PRIM-OP"));
	add_child(l, mksym(sym));
	env_add(e, sym, REF, l);
}

install_primitives(env_t *env)
{
	add_primop(env, "+");
	add_primop(env, "-");
	add_primop(env, "*");

	add_primop(env, "=");
	add_primop(env, "<=");
	add_primop(env, ">=");
	add_primop(env, ">");
	add_primop(env, "<");

	add_primop(env, "car");
	add_primop(env, "cdr");
	add_primop(env, "cons");

	add_primop(env, "not");

	add_primop(env, "null?");
	add_primop(env, "pair?");
}

list_t* do_prim_op(char *name, list_t *args)
{
	int i = 0;
	int j;
	int val;
	list_t* nl = c_malloc(sizeof(list_t));

	if (!strcmp(name, "not")) {
		val = 0;
		if (args->cc != 1) {
			printf("`not' expects one argument");
			exit(1);
		}
		/* r6rs.pdf section 11.8, page 47 */
		val = args->c[0]->type == BOOL && !args->c[i]->val;
		return makebool(val);
	}

	if (!strcmp(name, "+")) {
		val = 0;
		for (i = 0; i < args->cc; ++i) {
			if (args->c[i]->type != NUMBER) {
				printf("Error: + expects numbers\n");
				exit(1);
			}
			val += args->c[i]->val;
		}
		nl->type = NUMBER;
		nl->val = val;
		return nl;
	}

	if (!strcmp(name, "-")) {
		if (args->cc == 1) {
			if (args->c[0]->type != NUMBER) {
				printf("Error: - expects numbers\n");
				exit(1);
			}
			val = -args->c[0]->val;
		} else {
			for (i = 0; i < args->cc; ++i) {
				if (args->c[i]->type != NUMBER) {
					printf("Error: - expects numbers\n");
					exit(1);
				}
				if (i == 0)
					val = args->c[i]->val;
				else
					val -= args->c[i]->val;
			}
		}
		nl->val = val;
		nl->type = NUMBER;
		return nl;
	}

	if (!strcmp(name, "*")) {
		val = 1;
		for (i = 0; i < args->cc; ++i) {
			if (args->c[i]->type != NUMBER) {
				printf("Error: * expects numbers\n");
				exit(1);
			}
			val *= args->c[i]->val;
		}
		nl->type = NUMBER;
		nl->val = val;
		return nl;
	}

	if (!strcmp(name, "=")) {
		if (args->cc != 2
		|| args->c[0]->type != NUMBER
		|| args->c[1]->type != NUMBER) {
			printf("Error: = expects two numbers\n");
			exit(1);
		}
		return makebool(args->c[0]->val == args->c[1]->val);
	}

	if (!strcmp(name, ">")) {
		if (args->cc != 2
		|| args->c[0]->type != NUMBER
		|| args->c[1]->type != NUMBER) {
			printf("Error: = expects two numbers\n");
			exit(1);
		}
		return makebool(args->c[0]->val > args->c[1]->val);
	}

	if (!strcmp(name, "<")) {
		if (args->cc != 2
		|| args->c[0]->type != NUMBER
		|| args->c[1]->type != NUMBER) {
			printf("Error: = expects two numbers\n");
			exit(1);
		}
		return makebool(args->c[0]->val < args->c[1]->val);
	}

	if (!strcmp(name, "<=")) {
		if (args->cc != 2
		|| args->c[0]->type != NUMBER
		|| args->c[1]->type != NUMBER) {
			printf("Error: = expects two numbers\n");
			exit(1);
		}
		return makebool(args->c[0]->val <= args->c[1]->val);
	}

	if (!strcmp(name, ">=")) {
		if (args->cc != 2
		|| args->c[0]->type != NUMBER
		|| args->c[1]->type != NUMBER) {
			printf("Error: = expects two numbers\n");
			exit(1);
		}
		return makebool(args->c[0]->val >= args->c[1]->val);
	}

	if (!strcmp(name, "cons")) {
		if (args->cc != 2) {
			printf("Error: `cons' expects 2 arguments\n");
			exit(1);
		}
		/* just return the list as-is for now */
		memcpy(nl, args, sizeof(list_t));
		nl->type = CONS;
		return nl;
	}

	if (!strcmp(name, "car")) {
		if (args->cc != 1) {
			printf("Error: `car' expects 1 argument\n");
			exit(1);
		}
		if (args->c[0]->type != CONS) {
			printf("Error: `car' expects a linked-list\n");
			exit(1);
		}
		if (args->c[0]->cc < 1) {
			printf("Error: `car' has failed\n");
			exit(1);
		}
		return args->c[0]->c[0];
	}

	if (!strcmp(name, "cdr")) {
		if (args->cc != 1) {
			printf("Error: `cdr' expects 1 argument\n");
			exit(1);
		}
		if (args->c[0]->type != CONS) {
			printf("Error: `cdr' expects a linked-list\n");
			exit(1);
		}
		if (args->c[0]->cc < 2) {
			printf("Error: `cdr' has failed\n");
			exit(1);
		}
		return args->c[0]->c[1];
	}

	if (!strcmp(name, "null?")) {
		if (args->cc != 1)
			return makebool(0);
		if (args->c[0]->type == SYMBOL && !strcmp(args->c[0]->head, "NIL"))
			return makebool(1);
		if (args->c[0]->type == CONS && args->c[0]->cc == 0)
			return makebool(1);
		return makebool(0); 
	}

	if (!strcmp(name, "pair?")) {
		return makebool(args->cc == 1 
			&& args->c[0]->type == CONS);
	}

	return NULL;
}

list_t* apply(list_t *proc, list_t *args)
{
	env_t *ne;
	list_t *last;
	int i;

	/* Primitive operation */
	if (proc->cc == 2 && !strcmp(proc->c[0]->head, "PRIM-OP"))
		return do_prim_op(proc->c[1]->head, args);

	/* General case */
	if (proc->type == CLOSURE) {
		/* Check that parameter count matches */
		if (proc->c[0]->cc != args->cc) {
			printf("Error: arg count mismatch in closure application\n");
			goto afail;
		}

		/* Build a new environment */
		ne = new_env();
		ne->father = proc->closure;

		/* Bind the formal parameters 
		 * in the new environment */
		for (i = 0; i < proc->c[0]->cc; ++i)
			env_add(ne, proc->c[0]->c[i]->head,
				REF, args->c[i]);
	
		/* Evaluate the bodies of the closure
		 * in the new environment which includes
		 * the bound parameters	*/
		for (i = 1; i < proc->cc; ++i)
			last = eval(proc->c[i], ne);

		/* Return the evaluation of the last body */
		return last;
	}

afail:
	printf("Error: `apply' has failed\n");
	exit(1);
}

/* 
 * CLI reader
 */
int do_read(char *buf)
{
	int bal = -1;
	char tmp[1024];
	char *p;
	int i = 0;
	int bl = 0;
	int ind;

	/* Loop as long as there 
	 * are unclosed parentheses */
	while (bal) {
		/* Print prompt */
		if (!i++)
			printf("]=> ");
		else {
			/* new user line, missing parentheses */
			printf("... ");
			/* auto-indent */
			if (bal > 0 && bal < 5) {
				for (ind = 0; ind < bal; ++ind)
					printf("   ");
			}
		}
		fflush(stdout);

		if (!fgets(tmp, 1024, stdin)) {
			printf("\n");
			return 0;
		}

		if (*tmp == ';')
			/* XXX: continue */
			goto contlab;

		/* skip empty lines, parser hates them */
		if (*tmp == '\n') {
			if (i == 1) --i;
			/* 
			 * blank line
			 * => autocomplete parentheses
			 *
			 */
			if (++bl > 0 && bal > 0) {
				printf ("... ");
				for (ind = 0; ind < bal; ++ind) {
					printf(")");
					strcat(buf, ")");
				}
				fflush(stdout);
				printf("\n");				
				break;
			}
			/* XXX: continue */
			goto contlab;
		} else
			bl = 0;

		/* eat newline */
		for (p = tmp; *p; ++p) {
			if (*p == '\n' || *p < 0) {
				*p = 0;
				break;
			}
		}

		/* add inputted line to complete buffer */
		strcat(buf, tmp);
		strcat(buf, " ");

		/* count parentheses balance */
		bal = 0;
		for (p = buf; *p; ++p) {
			if (*p == '(')
				++bal;
			else if (*p == ')')
				--bal;
		}
		
		contlab:;
	}

	return 1;
}

/*
 * Add a symbol to an environment
 */
int env_add(env_t *e, char *sym, int ty, char *p)
{
	int c = e->count;
	char **nsym;
	char *nty;
	char **nptr;
	int i;

	if (++(e->count) >= e->alloc) {
		e->alloc += 16;
		nsym = c_malloc(e->alloc * sizeof(char *));
		nty = c_malloc(e->alloc);
		nptr = c_malloc(e->alloc * sizeof(char *));
		
		for (i = 0; i < e->alloc - 16; ++i) {
			nsym[i] = e->sym[i];
			nty[i] = e->ty[i];
			nptr[i] = e->ptr[i];
		}

		e->sym = nsym;
		e->ty = nty;
		e->ptr = nptr;
	}

	e->sym[c] = c_malloc(strlen(sym) + 1);
	strcpy(e->sym[c], sym);

	e->ty[c] = ty;

	e->ptr[c] = p;
}

/*
 * Modify a symbol's value, as in set!
 */
env_set(env_t *e, char *sym, int ty, char *p)
{
	env_ref_t* ref = lookup(e, sym);

	if (ref->e == NULL) {
		printf("Error: unbound variable `%s'\n", sym);
		exit(1);
	}

	ref->e->sym[ref->i] = c_malloc(strlen(sym) + 1);
	strcpy(ref->e->sym[ref->i], sym);

	ref->e->ty[ref->i] = ty;

	ref->e->ptr[ref->i] = p;
}

/*
 * Evalute each member of a list, in-place
 */
evlist(list_t* l, env_t *env)
{
	int i = 0;
	for (i = 0; i < l->cc; ++i)
		l->c[i] = eval(l->c[i], env);
}

add_child(list_t *parent, list_t* child)
{
	list_t **new;
	int i;

	if (++(parent->cc) >= parent->ca) {
		parent->ca += 16;
	
		new = c_malloc(parent->ca * sizeof(list_t *));

		for (i = 0; i < parent->ca - 16; ++i)
			new[i] = parent->c[i];
		
		parent->c = new;
	}

	parent->c[parent->cc - 1] = child;
}

int validname(char c)
{
	return
		c != '('
	&&	c != ')'
	&&	c != ' '
	&&	c != '\t'
	&&	c != '\n';
}

int isnum(char c)
{
	return c >= '0' && c <= '9';
}

/*
 * Parse a string into an S-expression
 */
char* build(list_t* l, char *expr)
{
	char *p, *q;
	char *old;
	int i;
	int lambda = 0;
	char tok[16];
	int sgn;
	list_t* child;
	list_t* child2;

	p = expr;

	if (*p == '\'') {	/* quoting */
		child = new_list();
		p = build(child, p + 1);
		add_child(l, mksym("QUOTE"));
		add_child(l, child);
		l->type = LIST;
	}
 	else if (*p == '(') {	/* list */
		++p;
		l->type = LIST;
		l->cc = 0;
		i = 0;
		while (*p != ')' && *p) {
			while (*p && *p == ' ' || *p == '\t')
				++p;
			if (!*p)
				break;
			child = new_list();
			old = p;
			p = build(child, p);
			if (p == old)
				break;
			add_child(l, child);
		}

		if (*p++ != ')') {
			printf("\nError: ) expected\n");
			exit(1);
		}
	}
	else if (isnum(*p) || (*p == '-' && isalnum(*(p+1)))) {	/* number */
		q = tok;
		if (*p == '-') {
			sgn = -1;
			++p;
		} else {
			sgn = 1;
		}
		while (*p && isnum(*p))
			*q++ = *p++;
		*q = 0;
		l->type = NUMBER;
		int tmp;
		sscanf(tok, "%d", &tmp);
		l->val = tmp;		/* XXX: &(a->b) */
		l->val *= sgn;
	} else {	/* symbol */
		while (*p == ' ' || *p == '\t')
			++p;
		q = tok;
		while (*p != '(' && *p != ')' && validname(*p))
			*q++ = *p++;
		*q = 0;
		l->type = SYMBOL;
		strcpy(l->head, tok);
	}

	return p;
}

/*
 * Mark-sweep garbage collector
 */

char **ptrs;
char* mark;
int len = 0;
int alloc = 0;

gc_selfdestroy()
{
	int i = 0;
	for (i = 0; i < len; ++i)
		free(ptrs[i]);
	free(mark);
	free(ptrs);
}

final_clean_up()
{
	gc();
	gc_selfdestroy();
}

add_ptr(char *p)
{
	int i;
	for (i = 0; i < len; ++i)
		if (ptrs[i] == p) {
			mark[i] = 0;
			return;
		}
	
	if (++len >= alloc) {
		alloc += 16;
		ptrs = realloc(ptrs, alloc * sizeof(char *));
		mark = realloc(mark, alloc);
		if (!ptrs || !mark) {
			printf("Error: marksweep: realloc has failed\n");
			exit(1);
		}
	}
	mark[len - 1] = 0;
	ptrs[len - 1] = p;
}

do_mark(char* p, int m)
{
	int i;

	if (!p)
		return;

	for (i = 0; i < len; ++i) {
		if (ptrs[i] == p) {
			mark[i] = m;
			return;
		}
	}

	/* 
	 * add_ptr(p);
	 * do_mark(p, m);
	 */
}

gc()
{
	int i;
	int orig;

	char **copy_ptr;
	char *copy_mark;

	/* mark */
	marksweep(global);

	/* sweep */
	for (i = 0; i < len; ++i)
		if (ptrs[i] != NULL)
			if (mark[i] == 0)
				free(ptrs[i]);

	/* make a copy of the object list */
	copy_ptr = malloc(len * sizeof(char *));
	if (!copy_ptr) {
		printf("marksweep: malloc failed\n");
		exit(1);
	}
	memcpy(copy_ptr, ptrs, len * sizeof(char *));

	copy_mark = malloc(len);
	memcpy(copy_mark, mark, len);

	/* make a new list with the nonfreed stuff */
	orig = len;
	len = 0;
	for (i = 0; i < orig; ++i) 
		if (copy_mark[i] != 0)
			add_ptr(copy_ptr[i]);

	/* erase temporary copies */
	free(copy_ptr);
	free(copy_mark);
}

marksweep_list(list_t *l)
{
	int i;

	do_mark(l, 1);
	do_mark(l->c, 1);

	if (l->type == LIST || l->type == CLOSURE)
		for (i = 0; i < l->cc; ++i)
			marksweep_list(l->c[i]);

	if (l->type == CLOSURE && l->closure != global)
		marksweep(l->closure);
}

marksweep(env_t *e)
{
	int i;

	do_mark(e, 1);
	do_mark(e->sym, 1);
	do_mark(e->ty, 1);
	do_mark(e->ptr, 1);

	for (i = 0; i < e->count; ++i)
		do_mark(e->sym[i], 1);

	for (i = 0; i < e->count; ++i)
		marksweep_list(e->ptr[i]);

	if (e->father && e->father != global)
		marksweep(e->father);
}

/*
 * S-expression output
 */

printout(list_t *l)
{
	printout_iter(l, 0);
}

printout_iter(list_t* l, int d)
{
	int i;

	/* switch based on type */
	if (l->type == NUMBER) {
		printf("%d", l->val);
	} else if(l->type == SYMBOL) {
		printf("%s", l->head);
	} else if (l->type == LIST) { 
		printf("(%s", l->head);
		for (i = 0; i < l->cc; ++i) {
			if (i > 0)
				printf(" ");
			printout_iter(l->c[i], d + 1);
		}
		printf(")");
	} else if (l->type == CLOSURE) {
		printf("#<CLOSURE:%p>", l->closure);
	} else if (l->type == BOOL) {
		if (l->val)
			printf("#t");
		else
			printf("#f");
	} else if (l->type == CONS) {
		if (l->cc == 2 && l->c[1]->cc == 0 && 
			!(l->c[1]->type == SYMBOL && !strcmp(l->c[1]->head, "NIL"))) {
			printf("(");
			printout_iter(l->c[0], d + 1);
			printf(" . ");
			printout_iter(l->c[1], d + 1);
			printf(")");
		} else if (l->cc) {
			printf("(");
			while (1) {
				printout_iter(l->c[0], d + 1);
				if (l->cc == 2 && l->c[1]->cc) {
					printf(" ");
					l = l->c[1];
				} else
					break;
			}
			printf(")");
		} else {
			printf("()");
		}
	}
}

/*
 * main REPL loop
 */

/* strip newline */
strip_nl(char *s)
{
	while (*s) {
		if (*s == '\n') {
			*s = 0;
			break;
		}
		++s;
	}
}

int main(int argc, char **argv)
{
	char *buf = malloc(1024 * 1024 * 2);
	char *p;
	list_t *expr = new_list();
	int i, c;
	char *ptr, *old;

	/* safety herp derp */
	sprintf(buf, "");

	/* Create global environment */
	global = new_env();

	/* Set up the built-in procedure symbols 
	 * for arithmetic, comparisons, etc. */
	install_primitives(global);

	while (1) {		
		/* read a (syntactic) line */
		if (!do_read(buf))
			break;

		if (!*buf)
			break;

		/* parse input into a tree */
		build(expr, buf);

		/* arghhh */
		if (feof(stdin))
			break;

		/* eval and print */
		printout(eval(expr, global));
		printf("\n");

		/* clean up for the next iteration */
		gc();
		sprintf(buf, "");
		expr = new_list();
	}

	final_clean_up();
	free(buf);

	return 0;
}
