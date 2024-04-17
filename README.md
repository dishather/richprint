# richprint
Print compiler information stored in Rich Header of PE executables.

## What is that "Rich Header of PE executables"?
It's a (usually small) section of binary data created by Microsoft linker.
The data is located between old MZ header stub (also called DOS stub) and
PE header. The data is encoded using a simple key, the only readable
part of the whole section being the word "Rich".

The section is ubiquitous: you can find it in almost any type of PE
(portable executable) file: .EXE, .DLL. .CPL (control panel applets), etc.
Yet, if the file was created by a non-Microsoft linker, it will not have
a Rich header.

The official name for this section is not known for sure, most likely it
is something similar to "build prodid block".

The unofficial names for this section are "Rich Header" or "Rich Section",
for obvious reasons.

The format of Rich Header and the gory details of decoding it can be found
in [the excellent article by Daniel Pistelli.](http://www.ntcore.com/files/richsign.htm)

## So, what does it contain after all?
Short and useless answer: the Rich Header contains the list of all @comp.id's
used to create the executable file, together with their counts.

Long and elaborate answer:
* When you compile a source file with a Microsoft compiler, it puts a special
  @comp.id record into the resulting object file. This @comp.id can be used to
  identify the exact version of the compiler (down to the build number).
  Compilers from different languages have similar, but distinct @comp.id's.
  This means that by looking at the @comp.id of an object file, you can tell
  not only the exact version of the compiler, but also whether the source file
  was a C or a C++ file. Assembler (ml.exe) and cvtres (utility for converting
  RES files into object files) also have their own @comp.id.
* When you link a program using Microsoft linker, it puts the list of
  all @comp.id's into the Rich Header, followed by a count of files. E.g., if
  your program was built from 10 C++ files and one ASM file, and you used
  Microsoft Visual Studio 2013 to compile and link it, you are likely to have
  the following records in your program's Rich Header:
  <pre>
  00e1520d e1 21005 10 [C++] VS2013 build 21005
  00df520d df 21005  1 [ASM] VS2013 build 21005
  00de520d de 21005  1 [LNK] VS2013 build 21005
  </pre>
  The order of the records depends on the linker's input, but the @comp.id
  tells us (and everyone) that you used Microsoft (R) C/C++ Optimizing Compiler Version 18.00.21005.1
  from Visual Studio 2013 RTM (i.e., without any updates installed).
  The last record is usually the linker's own @comp.id, but this is not always so.
  Also note that *every object file* counts; this includes run-time library
  files supplied by your vendor. Also, records that describe symbols imported
  and exported by DLLs have their distinct @comp.id's.

## Sources of information
I gathered some @comp.id's from my own collection of Visual Studio editions.

Some were interpolated using open sources (e.g., an
[excellent list of Visual Studio versions](https://dev.to/yumetodo/list-of-mscver-and-mscfullver-8nd)
by @yumetodo. In this list, _MSC_FULL_VER contains the build number, so it is
easy to interpolate the @comp.id's when you know the numbering scheme for
different tools.

"Interpolated" values are most likely to be correct. I checked some interpolated
@comp.id's against real-world values, and they matched. Yet, interpolated
values are marked with (*) for - ehm... - completeness?

## How can this information be used?
In any way you like. For example, to satisfy your curiosity by inspecting the binaries in your system.
Also, Rich Headers can allegedly be used in forensics.

## Can I prevent Microsoft tools from emitting this header?
Yes you can. Provide this undocumented option to the linker: `/emittoolversioninfo:no`.

Thanks to [Oliver Schneider](https://github.com/assarbad) for pointing this out.
