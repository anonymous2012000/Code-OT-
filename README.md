# OTs

Dependency:

* GMP: https://gmplib.org/

Runnig a test: 
* Clone download the repository (i.e., Code-OT--main).
* Install the library and unzip the downloaded file, Code-OT--main.zip.
* there will be two folders in the unzipped file:
        * Helix-OT--1-out-of-n-OT
        * Priority-OT--ordered-t-out-of-n-OT
* Open your terminal and "cd" to one of the folders (e.g., cd /Path_to_Helix-OT--1-out-of-n-OT)
* Run the following command lines in order:

        g++ -std=c++11 -lgmpxx -lgmp main.cpp -o test
  
        ./test


In the main file, in each folder, you can change the pramteres for different values of "n", "invocations", and "number of tests". 


