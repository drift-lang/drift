# Drift Script

A minimalist and mundane scripting language.

I like all simple things, simple and beautiful, simple and strong. I know that all development in the world comes from simplicity. Every line of code, character, format, indentation, etc. is a beautiful product.

- Written in pure c99 language version.
- Bytecode execution.
- After optimization, the size is only about 30 kb.
- Provide external interfaces to access languages libraries that support links.

The official website is https://drift-lang.fun/, the document is being prepared.

### Build

The number of source files for the drift compiler is small, building them is also very simple. Make sure the [GCC](https://www.gnu.org/software/gcc/) compiler and [Python](https://www.python.org/) interpreter are installed. My GCC version is 11.2.0

Then use the <code>python</code> command to execute the <code>build.py</code> file in the directory to compile. The directory will generate the executable file of drift. Execute it without parameters and get the prompt information.

Accessing the **-bug** parameter at the back indicates the GCC parameter **-fsanitize=address**, which is used for memory and bug detection.