# Add Folder to CMake

If you want to add a folder for do something :
* Duplicate the test folder and add him in the ```CMakeList.txt```
* With this line ```add_subdirectory(FolderName)```

After that go in the duplicate folder for some modification

* In the ```CMakeList.txt``` in the folder
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

# Add File to project

If we take your new folder before and you want to add some files :
* Put your files in ```FolderName/src/``` or in ```FolderName/assets/```
* And add them in the ```CMakeList.txt```
```
# If your file is a source file
set(SOURCE
    src/File1.cpp
)

# If your file is an include file
set(INCLUDE
    src/File1.h
    src/File2.h
)

# If your file is a shader file
set(SHADERS
    assets/File3.vsh
    assets/File3.psh
)

# If your file is an asset file
set(ASSETS
    assets/File4.png
)
```