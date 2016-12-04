wireless-project.cc is the only program used in the project. A mega-function is written to return throughput as a double
variable in the main. The implementation challenge faced is how to do multiple comparisons with a clear organization of
the code. Thus, multiple variables are introduced to pass into the function by reference. In this case, functions can be 
called multiple times in the main area, that will have a clear organizations on the result. 

Throughput is written in a .txt file in case terminal output is lost. However, results do not have a reference to compare, as 
variable settings in the experiment are changing. Hoever, throughput result is compared in a general sense with some standard 
reference as to see if there is a certain bias to affect result. 
