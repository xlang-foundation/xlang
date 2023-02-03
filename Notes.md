# Introduction 

base on expression eval
##from https://www.geeksforgeeks.org/expression-evaluation/
1. While there are still tokens to be read in,
   1.1 Get the next token.
   1.2 If the token is:
	   1.2.1 A number: push it onto the value stack.
	   1.2.2 A variable: get its value, and push onto the value stack.
	   1.2.3 A left parenthesis: push it onto the operator stack.
	   1.2.4 A right parenthesis:
		 1 While the thing on top of the operator stack is not a
		   left parenthesis,
			 1 Pop the operator from the operator stack.
			 2 Pop the value stack twice, getting two operands.
			 3 Apply the operator to the operands, in the correct order.
			 4 Push the result onto the value stack.
		 2 Pop the left parenthesis from the operator stack, and discard it.
	   1.2.5 An operator (call it thisOp):
		 1 While the operator stack is not empty, and the top thing on the
		   operator stack has the same or greater precedence as thisOp,
		   1 Pop the operator from the operator stack.
		   2 Pop the value stack twice, getting two operands.
		   3 Apply the operator to the operands, in the correct order.
		   4 Push the result onto the value stack.
		 2 Push thisOp onto the operator stack.
2. While the operator stack is not empty,
	1 Pop the operator from the operator stack.
	2 Pop the value stack twice, getting two operands.
	3 Apply the operator to the operands, in the correct order.
	4 Push the result onto the value stack.
3. At this point the operator stack should be empty, and the value
   stack should have only one value in it, which is the final result.


#Misc
map find less or greator
https://stackoverflow.com/questions/529831/returning-the-greatest-key-strictly-less-than-the-given-key-in-a-c-map
#Flask template syntax
https://flask.palletsprojects.com/en/2.1.x/tutorial/templates/
#jinja :Template Designer Documentation
https://jinja.palletsprojects.com/en/3.1.x/templates/

#for WSL, to compile PyEng, run:
sudo apt-get install python3.10-dev

#For uuid/uuid.h in linux -wsl, maybe need to run line below
sudo apt-get install uuid-dev

# for openssl required by http plugin
sudo apt-get install libssl-dev

#8/30/2022. Windows 11 Python install from Store, issues
https://stackoverflow.com/questions/58754860/cmd-opens-windows-store-when-i-type-python
#9/1/2022
srv.listen("::1", 8088) if use this , nodejs can't connect to 
change to srv.listen("::", 8088) or
srv.listen("0.0.0.0", 8088)
is OK
