# keep this file for some comments and issues

# ABI
- xhost.h never use any stl class,memory alloc/free, new/delete need to keep in same side
- value.h still keep <string>, but just keep in local usage, not cross ABI 
- class inside xport.h need to keep pass reference not value
- X::Port::Function may still have ABI issues, because different compiler may has  different layout of lambda

comments 
