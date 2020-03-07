Note here first. This is indeed a free software. Hence, you have the right to redistribute and modify except one thing special, 
that is the titleand the name of program should remain the "Ruby&Waku" word for memorializing the important families of the author.

This program is developed under Code::Blocks 12.11 with mingw gcc 4.7.1 and runs on the WIN32 platform.
It is recommended to install Code::Blocks 12.11 first, and then open Reversi.cbp for easy compiling.

The AI algorithm uses MinMax tree with alpha-beta pruning. Especially, it has a priority next-step mechanism to make alpha-beta pruning more efficient.
After rewriting and tough debuging, now, AI algorithm has been parallelized. In theory, It can support and utilize as many cores as the CPU has.
For practical reasons, the current version supports up to 8 cores.
The search depth is 8 by default and can be to deepen and be parallelized by the option setting.
Furthermore, There is a benchmark option in the menu, which mainly tests the CPU's capability for integer and muti-thread.

Although this program is just a amateur program compared with other professional ones whose AI and UI are really good, it is, somehow, like my child.
I make it line by line, little by little, and spend much time testing and finding bugs on it.
By now, the AI performance makes me satisfied , either the computing speed or the chess skill. It is quite difficult to defeat the AI for me.
There are still some bugs that I have known and many places that could be improved. 
Maybe, I will implement the Transposition Table in order to avoid repetitive computing some other day.

At last, it is glad to meet you. 
