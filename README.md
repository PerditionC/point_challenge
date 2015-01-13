Sample implementations for programming challenge see [http://churchillnavigation.com/challenge/](http://churchillnavigation.com/challenge/ "Churchill Navigation Challenge") and also [reddit](http://www.reddit.com/r/programming/comments/2s0rv3/5k_for_the_fastest_code_2d_ranked_point_search/ "reddit announcement").

*update: also found another friendly person posted a sample implementation; looks to be the same algorithm but in more modern C++ style so cleaner: [https://github.com/template-lange/challenge](https://github.com/template-lange/challenge)

All source tested with Microsoft (R) Visual Studio/C++ version 2010 [MSVC2010] but should build with 2013 or with new projects in earlier versions.  Note: be sure to set build to x64 and Release.


Included is a stub DLL that provides the minimal foundation for a submission.  Note this does not link to the standard C/C++ libraries so also includes a stub DLL entry point.  

For a better starting point there is a reference DLL.  Note: this is not the source to the challenge's reference but a comparable implementation.  It runs slightly faster during search but slower during loading as it copies data and then sorts; to speed up loading the data could be copied directly to ultimate location and/or a faster sort implemented.  There are two variants which perform about the same (for any given run either may be faster than the other).  One which uses C++ vectors and std::sort, the other more like plain C which uses a plain array and standard library's qsort, but otherwise the same.  To ensure proper linkage a .DEF file is defined and the functions are exported with proper STDCALL interface, along with export "C" to prevent C++ name mangling.

To use, download challenge.zip from above location, extract point_search.exe and optionally reference.dll.  Run point_search with no options to see usage help.

    point_search reference.dll my_impl.dll


These sources are public domain.
For benefit of those in locations that don't recognize PD:

> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
> THE SOFTWARE. 
