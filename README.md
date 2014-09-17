WHAT IS THIS?
=============

Linux Kernel source code for the device bq Aquaris 5

BUILD INSTRUCTIONS?
===================

Specific sources are separated by branches and each version is tagged with it's corresponding number. First, you should
clone the project:

	$ git clone git@github.com:bq/aquaris-5.git

After it, choose the version you would like to build

Jelly Bean: 1.x

KitKat: 2.x

    $ cd aquaris-5/

    $ git checkout 2.0.0-20140910-1000


Finally, build the kernel:

Jelly Bean:

	$ ./makeMtk -t eastaeon89_wet_jb2 n k

KitKat:

	$ ./makeMtk -t eastaeon89_wet_kk n k
