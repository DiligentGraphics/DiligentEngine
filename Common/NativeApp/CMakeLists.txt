cmake_minimum_required (VERSION 3.6)

project(NativeAppBase)

if(PLATFORM_WIN32)
	set(SOURCE 
		src/Win32/WinMain.cpp
	)
elseif(PLATFORM_UNIVERSAL_WINDOWS)

	# Windows Runtime types cannot be included into static libraries
	# https://social.msdn.microsoft.com/Forums/en-US/269db513-64ef-4817-a025-43954f614eb3/lnk4264-why-are-static-libraries-not-recommended-when-authoring-windows-runtime-types?forum=winappswithnativecode
	# So as a workaround, we will include all source files into the target app project
	function(get_native_app_uwp_source UWP_SOURCE UWP_INCLUDE UWP_INCLUDE_DIR)
		get_target_property(NATIVE_APP_SOURCE_DIR NativeAppBase SOURCE_DIR)

		set(${UWP_SOURCE}
			${NATIVE_APP_SOURCE_DIR}/src/UWP/Common/DeviceResources.cpp
			${NATIVE_APP_SOURCE_DIR}/src/UWP/App.cpp
			${NATIVE_APP_SOURCE_DIR}/src/UWP/SampleAppMain.cpp
			${NATIVE_APP_SOURCE_DIR}/src/UWP/TwEventUWP.cpp
			PARENT_SCOPE
		)

		set(${UWP_INCLUDE}
			${NATIVE_APP_SOURCE_DIR}/src/UWP/Common/DeviceResources.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/Common/DirectXHelper.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/Common/StepTimer.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/App.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/pch2.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/SampleAppMain.h
			${NATIVE_APP_SOURCE_DIR}/src/UWP/TwEventUWP.h
			PARENT_SCOPE
		)

		set(${UWP_INCLUDE_DIR}
			${NATIVE_APP_SOURCE_DIR}/Src/UWP
			PARENT_SCOPE
		)
	endfunction(get_native_app_uwp_source)

elseif(PLATFORM_ANDROID)
	set(SOURCE 
		src/Android/AndroidMainImpl.cpp
	)
elseif(PLATFORM_LINUX)
	set(SOURCE 
		src/Linux/LinuxMain.cpp
	)
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
	include/NativeAppBase.h
	include/NativeAppData.h
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
		target_compile_options(NativeAppBase INTERFACE /wd4189 /wd4063 /ZW)
	endif()
endif()

target_link_libraries(NativeAppBase 
PRIVATE 
	BuildSettings
	Common
)

if(PLATFORM_UNIVERSAL_WINDOWS)
	
elseif(PLATFORM_ANDROID)
	target_link_libraries(NativeAppBase PRIVATE NDKHelper android NativeAppBaseGlue)
	target_include_directories(NativeAppBase PRIVATE
		${ANDROID_NDK}/sources/android/cpufeatures
	)
elseif(PLATFORM_LINUX)
	target_link_libraries(NativeAppBase PRIVATE X11)
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
