# minesweeperradar
Cheat at Minesweeper with this SDL program!

It is a program which shows the revealed grid of your current Windows XP Minesweeper session. It also aligns a mouse cursor sprite with your pointing at the Minesweeper window, effectively allowing you to visually play the revealed minesweeper to completion.

![You can see!](preview_a.png?raw=true "You can see!")
![Now even I can clear Minesweeper!](preview_b.png?raw=true "Now even I can clear Minesweeper!")

This program checks for the existence of a `WINMINE.EXE` instance using the Win32 API. Once hooked, it makes use of the fact that this old Windows XP Minesweeper game has a finitely-sized, stack-allocated grid in which it stores its mines. In other words, you are free to use `ReadProcessMemory` with a fixed address space to index the Minesweeper grid in another program. All that is necessary is to find the process handle. Finding the window titled "Minesweeper" leads to its process ID by means of `GetWindowProcessThreadId`, whereupon `OpenProcess` yields the process handle. This program knows when an instance of Minesweeper opens or closes, and will react accordingly, so the radar and the game may be started and closed in any order.

The only question with this approach is how to find the memory addresses that would yield the grid. One method, which I used, is to open `WINMINE.EXE` in a debugger. It is a little gauche, and relatively obscure knowledge, but clicking a tile in Minesweeper elicits a call to `PlaySound`, albeit without playing any sound. However, this is good because this allows jumping to, say, an asm section in which I was able to guess that the branch that would jump to game over, would certainly check for a bomb, which would be inside the grid. Naturally the asm contains nothing but raw memory addresses on the stack, where the grid happens to be. As for what I would do had the grid been heap allocated, I actually dunno if I would be allowed to dereference any old pointer I find in another program to jump into its heap memory. Maybe it's possible. It may be worth looking into.
