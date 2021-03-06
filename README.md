# [[export_template]]

### 1. Motivation

There used to be an export template feature in c++98 but it was removed for some good reasons listed in [[N1426] Why We Can’t Afford Export](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2003/n1426.pdf) . This leaves us in the situation that template heavy code tends to be distributed as header-only libraries, but that has multiple drawbacks. The same code may need to be built over and over again in every TU, and this can become especially problematic with compile-time constexpr algorithms becoming more and more popular. Modules should be able to cache the parsed AST for the templates at least, but the instantiations may still need to be done in every TU as reusing them across different TUs may still be quite complicated for distributed builds in particular. With CUDA if you want to call a function template defined in a .cu file compiled with nvcc from a .cpp file compiled with a regular c++ compiler, you can't refactor it into a header-only library because the CUDA kernels will not compile in the .cpp TU and compiling the world with nvcc is impractical. These problems have led many to declaring and then explicitly instantiating the templates, which allows the definition to be built exactly once with the right compiler, but this has its own set of drawbacks. If you have many template parameters and/or non-type template parameters, instantiating the template for every possible combination of arguments can quickly become impractical. You can choose to add instantiations for the particular sets of template arguments that are actually used somewhere in a particular project, but then in a different project a different set might be needed. For testing or final packaging you might want to instantiate them with a wide range of arguments, but you might not want to keep rebuilding that entire set for local development builds. In any case if you have many such templates, manually adding the instantiations and keeping them up to date when function signatures or the number of template parameters change can become quite tedious.

### 2. What is this ?

This tool automatically generates the minimum number of explicit template instantiations for function/class templates declared with the [[export_template]] attribute that are required for the final application to link, allowing these templates to be declared in a header, instantiated in only one TU by including a file that contains the generated instantiations, and used anywhere without having to specify in advance all possible/allowed template arguments (except that currently the template arguments can only involve built-in types/values or declared symbols in the context of the header). Here's a brief example:

Interface.h:
```c++
namespace foo {
  template < typename T >
  [[export_template]] void bar();
}
```

Implementation.cpp:
```c++
#include "Interface.h"
// the following .xti file is automatically generated by the tool
#include <Interface.xti>

namespace foo {
  template < typename T >
  void bar() {
    // impl
  }
}
```

Main.cpp
```c++
#include "Interface.h"
int main() {
  foo::bar<int>();
}
```

The generated Interface.xti file looks like:
```c++
// exp foo::bar
template void foo::bar<int>();
```

The tool currently requires the application to be built with MSBuild. Once the tool is built and installed, applications with existing MSBuild project files can add the included build customization file to them, and those that use CMake to generate Visual Studio project files can simply add two lines to their CMakeLists.txt as follows:
```cmake
project(demo_proj LANGUAGES CXX)
find_package(export_template REQUIRED)
add_executable(demo
  Interface.h
  Implementation.cpp
  Main.cpp
)
target_export_template(demo)
```

Currently the generated .xti files are intentionally not cleaned between rebuilds, so while the initial build may be more costly, rebuilds and incremental builds will be optimal. By default the .xti files are placed under `$(SolutionDir)intermediate\src\$(ProjectName)\` but they can also be placed next to the headers, and then you can check them into git and as long as those who pull and work on the application's repository don't need new unique instantiations of the exported templates, they don't need to install the tool, and can build the application with any platform/toolset.

### 3. How does this work ?

When headers containing [[export_template]] change, the build customization invokes the tool which generates .xti files for them, initially containing the names of the exported class/function templates. The tool uses the [cppast library](https://github.com/foonathan/cppast) to parse the headers. The linker emits unresolved external symbol errors for any missing template instantiations. When the build customization detects that there were link errors, it walks the project dependency tree to find all of the .xti files in transitively referenced projects, calls the tool which parses the build log to find the unresolved external symbols, finds the minimum set of explicit instantiations that are needed to resolve those errors, goes through all of the referenced .xti files to find the one which exports a particular unresolved symbol and adds the explicit instantiation to it. Then the build customization tries to build the project again, triggering the compilation of the implementation files that include the changed .xti files. These implementation files may reference additional exported templates, so the build customization keeps running the tool and building again until all of the external symbols are resolved.

### 4. How to build and install it ?

- Install a pre-built LLVM x64 binary (e.g from https://llvm.org/builds/) to the default path (C:\Program Files\LLVM).
- Get the fmt and pcre libraries with e.g vcpkg (the tool needs x64-windows builds for these)
- Clone this repo and use CMake to generate x64 Visual Studio solutions for it. Remember to specify the vcpkg toolchain file for CMake, but otherwise it should just find everything it needs.
- Build it. The tool will automatically be installed to `C:/Program Files/export_template` after the build finishes.
