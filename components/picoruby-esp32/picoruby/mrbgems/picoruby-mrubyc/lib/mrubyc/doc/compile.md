# How to compile?

To make mruby/c sample programs, just `make` in top directory. `Makefile` generates mruby/c VM library `libmrubyc.a` and mruby/c executables.

## libmrubyc.a

`libmrubyc.a` is generated in `/src` directory. `libmrubyc.a` contains the mruby/c VM.


## mruby/c executables

Three mruby/c executables are generated in `/sample_c` directory.
These executables introduce several approaches to running the mruby/c VM.

- `sample_include`<br>
The mruby bytecode is internalized into the executable as a C array.
This is an example of the simplest procedure to start the mruby/c VM.
- `sample_no_scheduler`<br>
This is an example of taking a single mruby bytecode (`.mrb` file) as an argument and executing it.
- `sample_scheduler`<br>
This is also an example of taking a single mruby bytecode (`.mrb` file) as an argument and executing it.
Generates a task that executes bytecode.
- `sample_concurrent`<br>
This is an example that takes multiple mruby bytecodes as arguments and executes them concurrently.
- `sample_myclass`<br>
This is an example of defining mruby/c classes and methods in C.

## How to run your mruby bytecode

First, mruby bytecode is generated from the mruby source code(`.rb` file). The mruby compiler(`mrbc`) is required for bytecode generation.

```
mrbc your_program.rb
```

Now you get the mruby bytecode(`your_program.mrb`).
This bytecode is executed in the `sample_scheduler` program.

```
sample_scheduler your_program.mrb
```
