# README #
This project uses OpenMP to run several Parrallel patterns using the C language.

Project for CP 1920
Student: 41774

### What is this repository for? ###

* This is the code for the Home Project for CP 2019-20

### How do I get set up? ###

* Summary of set up

```
git clone the project
open a terminal on the code folder (Whrere makefile is)
make
```

```
The program has 2 optional arguments -d and -t
-d activates debug mode
-t [n] makes it so the program uses n threads in openmp.

The sequential versions of the patterns where left alone but renamed to [pattern]_seq.
```

* How to run tests
    * Example: `./main 1000000`
    * Example with debug: `./main -d 10`
    * Example with num thread control: `./main -t 2 10`

