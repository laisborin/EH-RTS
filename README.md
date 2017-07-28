This application allows to study the behavior of 
real-time schedulers in the context of an energy 
harvesting real-time system with the use of LITMUSRT.

Under the default installation settings of LITMUSRT, 
we modified the example_periodic.c sample code for 
periodic real-time tasks present in /opt/tutorial/.

It was also necessary to modify the compilation flags 
of the file config.makefile in /opt/liblitmus/inc/.

After selecting the scheduler in the application and 
in the LITMUSRT and compile, to run the application do:
	./example_periodic < {task set file} {log battery file} {number of batteries, 1 or 2} {capacity of batteries} > {log misses file}

The task set file needs to be described as:
	<n:size>
	<period of task 1> <computation time of task 1>
	<period of task 2> <computation time of task 2>
	 		    .			              		.
	 		    .					              .
	 	    	.					              .
	<period of task n> <computation time of task n>
