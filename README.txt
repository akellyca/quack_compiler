=============== PROJECT 2 ===============
===     Austen Kelly (951632601)	  ===
===            12/09/2019	       	  ===
=========================================

This project contains a semi-complete parser written for Quack.
If run with a valid* quack program, it outputs C code for that
program into ./src/output.c. The compiled version of that
output is stored in ./src/output.
It has instantiation checks fully implemented (minus typecase).
Type checking is implemented (again, other than for typecase),
except that for methods it requires exactly the expected type
(not super/sub types as desired...).
I like to test mine on "samples/tiny.qk"


(* valid and limited as described in the note below)

=========================================
NOTE:
This current version does not support any user defined classes!
As such, it only runs with quack programs containing only builtin
types. (Also, since it doesn't work with classes, I haven't implemented
generation for things like AST::Dot, AST::Return, AST::Construct.)
It also doesn't work on typecase statements. That being said,
I am fairly confident that it handles all other types of statements.
(It does produce some C warnings because I don't cast variables up
but it runs anyway.)

I would really love to get at least code generation for classes
implemented, along with co/contravariance checking for overridden 
methods in type checking. I'll put in updates as I make progress
through the week. Thank you for being so patient!
=========================================

To set up:
	1) Run "make image"
	2) Run "docker image ls" (to check it successfully
				created the image "quack_compiler")
	3) Run "docker run -it quack_compiler"

The dockerfile copies from michalyoung/cis461:base, creates
my project as quack_compiler, and builds the program.

To run (after building):

	./bin/quack_compiler samples/tiny.qk > src/output.c

To run the generated code (compile and then run):
	
	gcc src/output.c -o src/output
	./src/output

For testing with samples/tiny.qk, the output should be:
42
3
hello
forty-two
4
3
2
1

