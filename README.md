# Drift Script

A minimalist and mundane scripting language.

I like all simple things, simple and beautiful, simple and strong. I know that all development in the world comes from simplicity. Every line of code, character, format, indentation, etc. is a beautiful product.

- Written in pure C(gnu99) language version.
- Bytecode execution.
- For the drift language, see my blog post at https://bingxio.fun/article/2021-05-24.html.
- After optimization, the size is only about 30 kb.
- Provide external interfaces to access languages libraries that support links.
- Compilation topics are available for reference.

The official website is https://drift-lang.fun/, the document is being prepared.

### Download and install

The number of source files for the drift compiler is small, building them is also very simple. Make sure the [GCC](https://www.gnu.org/software/gcc/) compiler and [Python3](https://www.python.org/) interpreter are installed. My GCC version is 8.3.0.

Then use the <code>python</code> command to execute the <code>build.py</code> file in the directory to compile. The directory will generate the executable file of drift. Execute it without parameters and get the prompt information.

### Why create this project

Drift is a fixed-type language, any object has its type attribute, and each variable declaration needs to specify the type.

It is like a toy language, but as it develops slowly, you will find that it is so strong. Twelve keywords, fixed grammatical format, to deal with problems. You will find that using a simple programming language to do simple things, I think this is a truth that programmers will understand.

A better language I think is the C language, but drift is also possible as a simple compilation course, although it is not so perfect by me.

The main body of the compiler is three basic steps, lexical analyzer, syntax analyzer, bytecode generation and virtual machine evaluation. The parser uses the [TDOP-Pratt](https://tdop.github.io/) algorithm, and the virtual machine uses the basic structure of frames and stacks.

### Contributing

You can think of it as a toy language, with very few keywords and very little syntactic sugar. For performance improvement and better design ideas, please submit a pull request.