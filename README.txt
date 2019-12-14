=============== PROJECT 2 ===============
===     Austen Kelly (951632601)	  ===
===            12/14/2019	       	  ===
=========================================

This project contains a semi-complete parser written for Quack.
If run with a valid* quack program, it outputs C code for that
program into ./src/output.c. The compiled version of that
output is stored in ./src/output.
It has instantiation checks fully implemented (minus typecase).
Type checking is implemented (again, other than for typecase),
except that for methods it requires exactly the expected type
(not super/sub types as desired...). It occasionally also has issues
with inherited classes that aren't present in the super class (e.g. with
hands.qk, it throws a type error that this.hands has no method foo since
the parent class Hand doesn't, even though both children do...)
I like to test mine on "samples/tiny.qk" (because it works)


(* valid and limited as described in the note below)

=========================================
NOTE/UPDATE: This now includes user defined classes! I have also cleaned
up a lot of issues with tmp variable assigning. The biggest outstanding
issue is that dynamic dispatch barely works, and I still get weird
seg-faults on function calls sometimes. In particular, if you run on 
tiny.qk but change the 'if' condition to be false, the print of y
(which should be "forty-two") fails. Also type checking has some issues.
But overall I feel good that classes work at all. I plan to keep trying 
to sort out issues and make it better, but I figured it's time to send in
a real update.

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
Pt(9, 2)
Blah(5, 10)
hello
42
4
3
2
1

