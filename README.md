# Mastermind
This is the source code of COMP30023 Computer Systems (2016S1) Project 2 - Multiplayer Mastermind game.

## Game rules

> Mastermind is a two-player game consisting of a codemaker and a codebreaker. The codemaker secretly selects a code consisting of an ordered sequence of four colours `c1c2c3c4`, each chosen from the set {A, B, C, D, E, F} of six possible colours, with repetitions allowed.
>
> The codebreaker then tries to guess the code by repeatedly submitting their nominated sequence of four colours from the set {A, B, C, D, E, F}.
>
> After each guess, the codemaker provides feedback to the codebreaker using two numbers: the number of correct colours in the correct positions `b` and the number of colours that are part of the code but not in the correct positions `m`, using the format `[b:m]`. For example, if the code is `ABCC` and the codebreaker's guess is `BCDC`, then the codemaker's response would be `[1:2]` since the codebreaker has guessed the second `C` correctly and in the correct position, while having guessed the `B` and the first `C` correctly, but in the wrong position.
>
> The codebreaker continues guessing until they guesses the code correctly, or until they reaches a maximum allowable number of guesses (set to 10 in this project) without having correctly identified the secret code.

## Compiling and running the program

```
$ gcc -o server server.c
$ gcc -o client client.c
$ ./server <port-number> <secret-code>
$ ./client <server-IP> <port-number>
```

The <secret-code> on server's side is optional. If it is not filled in, the program will randomly generate a secret code.
