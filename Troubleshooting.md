### Build fails
  
* Make sure you code is up-to-date. When updating existing repository, don't forget to update all submodules:

```
git pull
git submodule update --recursive
```

* Try to [get clean version](https://github.com/DiligentGraphics/DiligentEngine#clonning)

* Make sure you build environment is up-to-date:
  * When using gcc, make sure the compiler version is at least 7.4
  * Make sure you build your project with c++11 features enabled

### Projects don't run

* When running from the command line, make sure that the project's `assets` folder is set as working directory
