# Source Code Formatting

Diligent Engine uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to ensure
consistent source code formatting style throught the code base. The format is validated by appveyor and travis
for each commit and pull request, and the build will fail if any code formatting issue is found. You can check the
logs to find the exact problem. It is, however, may be hard to strictly follow the formatting rules, so using 
clang-format to automatically correct all issues when working with the source code is recommended.

:warning: The output of the clang-format tool may vary between versions even if the configuration file stays unchanged. 
Diligent Engine uses clang-format 10.0.0, which must also be used by anyone to make sure formatting results are the same.
For convenience, clang-format executables for Windows, Linux and Mac are checked in into the codebase and can be found at
[DiligentCore/BuildTools/FormatValidation](https://github.com/DiligentGraphics/DiligentCore/tree/master/BuildTools/FormatValidation).

## Validation

Diligent Engine provides script files that can be executed to validate the source code formatting of every module. These scripts
can be found under `BuildTools/FormatValidation` folder of each module. Besides that, CMake generates build targets
(e.g. `DiligentCore-ValidateFormatting`) that run the same scripts. Building these targets has the effect of format validation,
so if All-build finishes without errors, the code should be properly formatted.

## Using clang-format to format the source code

You can run clang-format to format files in place, for example:

```
clang-format -i src/*.cpp include/*.h
```

Please refer to [this page](https://clang.llvm.org/docs/ClangFormat.html) for more details.

If you have clang-format installed and python in your path, there will be a new git command
that will check if your currently modified files need to be formatted:

```
git clang-format
```

You can also set up a [git hook](https://git-scm.com/docs/githooks) to automatically check that every 
commit complies with the formatting rules. 

## Using clang-format with the IDE

clang-format [supports integration](https://clang.llvm.org/docs/ClangFormat.html) with many popular IDEs.
Formatting the code directly from the editor is probably the most convenient way as it let's you see the
results right-away.
In particular, to configure Visual Studio 2017 or later to use clang-format, open `Tools -> Options` dialog and
go to `Text Editor -> C/C++ -> Formatting` tab. Mark `Enable ClangFormat` checkbox, and make sure to use custom clang-format
file either from the source tree, or, if you have the right version installed, from LLVM distribution.
In the same dialog you can also configure various formatting options.

Note that in certain circumstances it may be preferrable to disable automatic formatting (please do not abuse
this option!), which can be done using the following special comments:

```cpp
// clang-format off
...
// clang-format on
```
