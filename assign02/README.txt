RONALD GARCIA

--Implementation details--


`token.h`

The new tokens that I added to the lexer are:

  TOK_ASSIGNMENT,
  TOK_OR,
  TOK_AND,
  TOK_LESS_THAN,
  TOK_LESS_THAN_EQUAL,
  TOK_GREATER_THAN,
  TOK_GREATER_THAN_EQUAL,
  TOK_EQUAL,
  TOK_NOT_EQUAL,
  TOK_VARIABLE

These are to address the new functionalities of this minilang.

--lexer--

In the lexer, an interesting design choice to make was regarding the reading of the tokens that were valid single tokens.

For instance, when reading "==" versus "=". The way I approached this was to read ahead to see if the next character created another valid token.

--parser2--

For the parser, I approached it by implementing top-down, starting with Stmt and going down to R.

--interp + environment--

In environment, I opted to use Values, as I was having difficulties with the allocation of memory (now, my program is valgrind safe).

I used the map `m_vars` in environment to implement analyze() and execute(). I opted to use 
recursive functions ..._recurse(curnode, env), as the process of reading a tree is inherently 
recursive. While traversing the tree, each node has a specific behavior that is largely independent
of the previous nodes.

Additionally, I would like to thank you again for giving me the opportunity to take this course and the leniency on the due date of this assignment! 
I believe that I would be able to implement milestone 1 of assignment 2 on time.

