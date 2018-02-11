cmake_minimum_required (VERSION 3.6)

project(NativeAppBase)

if(PLATFORM_WIN32)

	set(SOURCE 
		src/Win32/WinMain.cpp
	)
	set(INCLUDE
		include/Win32/Win32AppBase.h
	)

	function(add_win32_app TARGET_NAME SOURCE INCLUDE ASSETS)
		add_executable(${TARGET_NAME} WIN32 ${SOURCE} ${INCLUDE} ${ASSETS})
	endfunction()

elseif(PLATFORM_UNIVERSAL_WINDOWS)

	set(SOURCE 
		src/UWP/dummy.cpp
	)
	set(INCLUDE
		include/UWP/UWPAppBase.h
	)

	# Windows Runtime types cannot be included into static libraries
	# https://social.msdn.microsoft.com/Forums/en-US/269db513-64ef-4817-a025-43954f614eb3/lnk4264-why-are-static-libraries-not-recommended-when-authoring-windows-runtime-types?forum=winappswithnativecode
	# So as a workaround, we will include all source files into the target app project
	function(add_uwp_app TARGET_NAME SOURCE INCLUDE ASSETS)
		get_target_property(NATIVE_APP_SOURCE_DIR NativeAppBase SOURCE_DIR)

		set(UWP_SOURCE
			${NATIVE_APP_SOURCE_DIR}/src/UWP/Common/DeviceResources.cpp
			${NATIVE_APP_SOURCE_DIR}/src/UWP/App.cpp
			${NATIVE_APP_SOURCE_DIR}/src/UWP/UWPAppBase.cpp
		)

		set(UWP_INCLUDE
			${NATIVE_APP_SOURCE_DIR}/src/UWP/Common/DeviceResources.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/Common/DirectXHelper.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/App.h
			${NATIVE_APP_SOURCE_DIR}/include/UWP/UWPAppBase.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/Common/StepTimer.h
		)

		add_executable(${TARGET_NAME} WIN32 ${SOURCE} ${INCLUDE} ${ASSETS} ${UWP_SOURCE} ${UWP_INCLUDE})

		target_include_directories(${TARGET_NAME} 
		PUBLIC
			${NATIVE_APP_SOURCE_DIR}/Src/UWP
		)

		source_group("UWP Common\\src" FILES ${UWP_SOURCE})
		source_group("UWP Common\\include" FILES ${UWP_INCLUDE})

	endfunction(add_uwp_app)

elseif(PLATFORM_ANDROID)

	set(SOURCE 
		src/Android/AndroidAppBase.cpp
	)
	set(INCLUDE
		include/Android/AndroidAppBase.h
	)
	function(add_android_app TARGET_NAME SOURCE INCLUDE ASSETS)
		get_target_property(NATIVE_APP_SOURCE_DIR NativeAppBase SOURCE_DIR)
		set(ANDROID_SOURCE
			${NATIVE_APP_SOURCE_DIR}/src/Android/AndroidMain.cpp
		)
		add_library(${TARGET_NAME} SHARED ${SOURCE} ${ANDROID_SOURCE} ${INCLUDE} ${ASSETS})
		target_link_libraries(${TARGET_NAME} 
		PRIVATE 
			android
			NativeAppGlue
		)
		#target_include_directories(${TARGET_NAME} 
		#PRIVATE
		#	${ANDROID_NDK}/sources/android/cpufeatures
		#)
		source_group("Android" FILES ${ANDROID_SOURCE})
	endfunction()

elseif(PLATFORM_LINUX)

    set(SOURCE 
        src/Linux/LinuxMain.cpp
    )
    function(add_linux_app TARGET_NAME SOURCE INCLUDE ASSETS)
        add_executable(${TARGET_NAME} ${SOURCE} ${INCLUDE} ${ASSETS})
	endfunction()
	
elseif(PLATFORM_MACOS)

    set(SOURCE
        src/MacOS/Renderer.cpp
    )
    set(INCLUDE
        src/MacOS/Renderer.h
    )

	function(get_native_app_macos_source APPLE_SOURCE APPLE_INCLUDE APPLE_RESOURCES APPLE_INFO_PLIST APPLE_INCLUDE_DIRS)
		get_target_property(NATIVE_APP_SOURCE_DIR NativeAppBase SOURCE_DIR)

		set(${APPLE_SOURCE}
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/main.m
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX/WindowController.m
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX/AppDelegate.m
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX/FullscreenWindow.m
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX/GLView.m
		)
        set_source_files_properties(${APPLE_SOURCE} PROPERTIES
            COMPILE_FLAGS "-x objective-c++"
        )
        set( APPLE_SOURCE ${APPLE_SOURCE} PARENT_SCOPE )

		set(${APPLE_INCLUDE}
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX/WindowController.h
       		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX/AppDelegate.h
       		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX/FullscreenWindow.h
       		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX/GLView.h
		    PARENT_SCOPE
		)

		set(${APPLE_RESOURCES}
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Data/OSX/Base.lproj/Main.storyboard
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Data/OSX/Images.xcassets/AppIcon.appiconset/dg.icns
		    PARENT_SCOPE
		)

		set(${APPLE_INFO_PLIST}
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Data/OSX/Info.plist
		    PARENT_SCOPE
		)

		set(${APPLE_INCLUDE_DIRS}
        		${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/OSX
			PARENT_SCOPE
		)
	endfunction()

elseif(PLATFORM_IOS)

    set(SOURCE
        src/MacOS/Renderer.cpp
    )
    set(INCLUDE
        src/MacOS/Renderer.h
    )

	function(get_native_app_ios_source APPLE_SOURCE APPLE_INCLUDE APPLE_RESOURCES APPLE_INFO_PLIST APPLE_INCLUDE_DIRS)
		get_target_property(NATIVE_APP_SOURCE_DIR NativeAppBase SOURCE_DIR)

		set(${APPLE_SOURCE}
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/main.m
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/iOS/AppDelegate.m
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/iOS/EAGLView.m
		)
        set_source_files_properties(${APPLE_SOURCE} PROPERTIES
            COMPILE_FLAGS "-x objective-c++"
        )
        set( APPLE_SOURCE ${APPLE_SOURCE} PARENT_SCOPE )

		set(${APPLE_INCLUDE}
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/iOS/AppDelegate.h
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/iOS/EAGLView.h
		    PARENT_SCOPE
		)

		set(${APPLE_RESOURCES}
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Data/iOS/Base.lproj/Main.storyboard
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Data/iOS/Base.lproj/LaunchScreen.xib
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Data/iOS/Images.xcassets/AppIcon.appiconset/dg-icon.png
		    PARENT_SCOPE
		)

		set(${APPLE_INFO_PLIST}
		    ${NATIVE_APP_SOURCE_DIR}/Apple/Data/iOS/info.plist
		    PARENT_SCOPE
		)

		set(${APPLE_INCLUDE_DIRS}
        		${NATIVE_APP_SOURCE_DIR}/Apple/Source/Classes/iOS
			PARENT_SCOPE
		)
	endfunction()

endif()

list(APPEND INCLUDE 
	include/AppBase.h
	include/NativeAppBase.h
)


add_library(NativeAppBase STATIC ${SOURCE} ${INCLUDE})
set_common_target_properties(NativeAppBase)

target_include_directories(NativeAppBase
PUBLIC
	include
)


if(MSVC)
	target_compile_options(NativeAppBase PRIVATE -DUNICODE)
	
	if(PLATFORM_UNIVERSAL_WINDOWS)
		# Disable w4189: local variable is initialized but not referenced
		# Disable w4063: case is not a valid value for switch of enum
		# Consume the windows runtime extensions (/ZW)
		target_compile_options(NativeAppBase INTERFACE /wd4189 /wd4063)
	endif()
endif()

target_link_libraries(NativeAppBase 
PRIVATE 
	BuildSettings
	Common
)

if(PLATFORM_WIN32)

	target_include_directories(NativeAppBase
	PUBLIC 
		include/Win32
	)

elseif(PLATFORM_UNIVERSAL_WINDOWS)
	
	target_include_directories(NativeAppBase 
	PUBLIC 
		include/UWP 
		src/UWP
	)

elseif(PLATFORM_ANDROID)
	target_link_libraries(NativeAppBase PUBLIC NDKHelper NativeAppGlue PRIVATE android)
	target_include_directories(NativeAppBase
	PUBLIC
	    include/Android
	)
elseif(PLATFORM_LINUX)
	target_link_libraries(NativeAppBase 
	PRIVATE 
		X11
	)
	target_include_directories(NativeAppBase 
	PUBLIC 
		include/Linux
	)

elseif(PLATFORM_MACOS OR PLATFORM_IOS)
    target_include_directories(NativeAppBase PUBLIC
        src/MacOS
    )
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})

set_target_properties(NativeAppBase PROPERTIES
	FOLDER Common
)
