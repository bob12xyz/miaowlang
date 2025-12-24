# miaowlang

miaowlang is a functional, statically typed programming language with a lisp syntax. It combines ahead of time compilation and type safety with the expressiveness of lisp.
The miaow compiler is based on llvm.

## syntax

### blankets and cats
```lisp
(meow "hello")
```
miaowlang is composed of many embedded "blankets". In each layer of blanket there are can be many cats. In the example above, the blanket has two cats in it: `meow` and `"hello"`. 
The first cat in a blanket is the function and all subsequent cats are arguments.
Each cat can be a sub-blanket wrapping multiple cats:

```lisp
(meow (->S (+ 1 1))) ;prints 2
```

Here, (+ 1 1) is a blanket with three cats that evaluates to the number two. Then, the `->S` cat converts that to a string and meow prints the string `"2"`.

### mice

You can leave a comment with the mice syntax.

```lisp
(meow "hello") ; this is mouse
```

Only full line comments are allowed.

### collars

Each cat can have a collar. For example:

```lisp
(def Str:greeting "hello")
```

The `Str` collar type annotates greeting to be a string. Collars can be applied to cats (`Char:33`) or blankets (`Char:(+ 33 1)`).
The current types supported are Int, Bool, Str, Array.


### beds
Curly braces denote beds. A bed can contain many blankets with cats in them. Every miaow program is actually a single bed.

```lisp
{
    (def Str:greeting "hello")
    (def Char:exclamation 33)
    (append greeting exclamation)
    (meow greeting)
}
```

The compiler will go down the blankets on the bed one by one in order.
With beds, you can have control flow. There are two control flows currently in miaow: if and while.

```lisp

(if (< 1 0) { ; if 1 < 0
    (meow "one is less than zero") ; then bed
} {
    (meow "one is more than zero") ; else bed
})

```

you can also define functions with beds

```lisp
(fun Nil:(print_greeting) {
    (meow "hello world")
})

(print_greeting) ; prints hello world
```

### arrays
You can have a pile of cats with an array.

```lisp
(def Array<Int>:favorite_numbers [1 2 3])
(remove favorite_numbers 0)
(meow (->S (get favorite_numbers 0))) ; prints 2
```
Arrays are dynamically sized. 

### fish
You can leave out fish for the preprocessing cat to eat.
```lisp
!define HELLO "hello"
{(echo HELLO)} ; prints "hello"
```
The two preprocessing fish currently supported are !define and !import.
Import reads another miaow file and combines it into the current file.

### playing with other cats

You can play with cats from other languages. For example:

hello.c
```c
void sayhello() {
    puts("hello from c cat");
}
```
```lisp
(extern (sayhello))
(sayhello) ; prints "hello from c cat"
```

## compiling
`make` will build a `miaow` binary. 
`miaow hello.miaow -o hello.ll` compiles hello.miaow to hello.ll. 
Then, `clang hello.ll -o hello` will produce the hello binary.

