### Build fails
  
* Make sure your code is up-to-date. When updating existing repository, don't forget to update all submodules:

```
git pull
git submodule update --recursive
```

* Try to [get clean version](https://github.com/DiligentGraphics/DiligentEngine#clonning)

* Make sure your build environment is up-to-date:
  * When using gcc, make sure the compiler version is at least 7.4
  * Make sure you build your project with c++11 features enabled
  * When including Diligent headers, make sure that exactly one of `PLATFORM_WIN32`,
	`PLATFORM_UNIVERSAL_WINDOWS`, `PLATFORM_ANDROID`, `PLATFORM_LINUX`, `PLATFORM_MACOS`, and
	`PLATFORM_IOS` macros is defined as `1`.

### Projects don't run

* When running from the command line, make sure that the project's `assets` folder is set as working directory
