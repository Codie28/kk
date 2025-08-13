# kk

a pager like less but with a lot *less* (haha get it) fetaures
this was a project to try and learn C and also how pagers work

## quick start

```bash
$ make kk
$ ./kk [file]
```

this project uses https://github.com/tsoding/flag.h for flag parsing

## features

- vertical scrolling
- vim like keybindings
- help page
- 26 marks
- color mode "-r" (parses basic ANSI)
- diff mode "-d" for `git diff`

## todo

- [ ] searching 
#this might be abit out of louge with me idk but i do need it if i ever gana use
#this outside of testing
- [ ] more jump commands
- [ ] make it read the file dynamicly and store it somehow
    - [ ] maybe search data structers
- [ ] make a windows version

- [/] git integration
    - [/] detect patch files and display the -/+ on column
    - [ ] add marks for `git diff` files (lines with @@)
    - [ ] add commands to go between `git logs` or add marks

## note to self

main goal of this projects isnt to make the next big pager
but making it expendable enough that maybe someday someway
i will actualy have some niche uses for this and maybe its
what programming is all about making stuff that arent just
perfect things but matters even the smallest amount  :m  .
