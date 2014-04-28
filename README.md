WHAT IS THIS?
=============

Linux Kernel source code for the device bq Aquaris 5

BUILD INSTRUCTIONS?
===================

Specific sources are separated by branches and each version is tagged with it's corresponding number. First, you should
clone the project:

	$ git clone git@github.com:bq/aquaris-5.git

After it, choose the version you would like to build:

	$ cd aquaris-5/
	$ git checkout 1.4.1-20140328-1139


Finally, build the kernel:

	$ ./makeMtk -t eastaeon89_wet_jb2 n k
