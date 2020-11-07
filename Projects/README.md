# Add Folder to CMake

If you want to add a folder for do something :
* Duplicate the test folder and add him in the ```CMakeList.txt```
* With this line ```add_subdirectory(FolderName)```

After that go in the duplicate folder for some modification

* In the ```CMakeList.txt```
* Change the name of the project
```
project(test CXX)
to
project(FolderName CXX)
```
* And change the last line
```
add_sample_app("test" "Projects" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
to
add_sample_app("FolderName" "Projects" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
```