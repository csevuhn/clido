# clido

> clido is an extremely simple cli todo list
> 
> i wanted to make this because i want to use ncurses in projects more
> 
> i made this for linux but i dont see why it wouldnt work on windows

## compiling

> 1. make sure you have ncurses and gcc installed
> 2. make a new directory with the `main.c` file in
> 3. run `gcc main.c -o clido -lncurses` in the directory with the source file
> 4. run `clido` in that directory

## how to use

| key             | action                     |
|-----------------|----------------------------|
| `↑` / `k`       | move selection up          |
| `↓` / `j`       | move selection down        |
| `enter`         | toggle task done/undone    |
| `n`             | add new task               |
| `d`             | delete selected task       |
| `q`             | quit (auto-saves)          |

**note: using ctrl + c to quit will not auto save your list to a file. use q when you want to quit instead**

## what to add

> - [x] finish basic stuff
> - [ ] add support for user made configs
> - [ ] better error handling?
> - [ ] fix some tiny bugs
> - [ ] ui stuff
> - [ ] test for windows
> - [ ] publish to aur (HUGE maybe)
> - [ ] more keybinds
> - [ ] due dates
> - [ ] more subtle improvements


## contact

> email, pgp public keys and other platforms are all available on my github profile readme
>
> donations are also there
