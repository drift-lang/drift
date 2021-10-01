# Drift Script

A minimalist and mundane scripting language.

I like all simple things, simple and beautiful, simple and strong. I know that all development in the world comes from simplicity. Every line of code, character, format, indentation, etc. is a beautiful product.

- Written in pure c99 language version.
- Bytecode execution.
- After optimization, the size is only about 100 kb.
- Provide external interfaces to access languages libraries that support links.

The official website is https://drift-lang.fun/, the document is being prepared.

### Build

The number of source files for the drift compiler is small, building them is also very simple.

Please ensure that the [GCC](https://www.gnu.org/software/gcc/) compiler is installed on your system. My GCC version is 11.2.0.

Execute the **build.sh** file using a UNIX like system. You can add **-bug** parameter to generate debugging information, which will build the **-fsanitize=address** parameter into the drift compiler.

1. Build the C file in the **src** directory as the target file.
2. Build the target file as an executable.
3. Build the C file under the **module** folder as the dynamic library file.

Now get the drift compiler and some dynamic standard libraries.
