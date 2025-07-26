# gd_parser

Single header C++ parser for the Godot resource file format using https://github.com/Znurre/havoc and https://github.com/yhirose/cpp-peglib

This library does not care about the semantics of the files, it simply parses them to an AST that can then be transformed into a more high level structure.

# Usage

Simply add the h/hpp files to your include path and include `gd_parser.hpp`. Parsing a file is as easy as using `gd::parse` with an `std::istream` object.

```cpp
#include <gd_parser.hpp>

int main()
{
	std::ifstream stream("scene.tscn");

	auto file = gd::parse(stream);

	// Do stuff with the file here

	return 0;
}
```
