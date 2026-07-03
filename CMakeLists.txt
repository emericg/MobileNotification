cmake_minimum_required(VERSION 3.20)

project(MobileNotification LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

################################################################################

# Generic dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Qml)
set(CORE_LIBRARIES Qt::Core Qt::Qml)

# Generic sources
set(CORE_SOURCES
    MobileNotification.cpp MobileNotification.h MobileNotification_private.h
    MobileNotificationStyle.h
    MobileNotification_QmlDispatcher.cpp MobileNotification_QmlDispatcher.h)

# OS specific sources & dependencies
if (ANDROID)
    set(PLATFORM_SOURCES
        MobileNotification_android.cpp MobileNotification_android.h)
    set(PLATFORM_LIBRARIES ${CORE_LIBRARIES})
elseif (IOS)
    enable_language(OBJCXX)
    set(PLATFORM_SOURCES
        MobileNotification_ios.mm MobileNotification_ios.h)
    set(PLATFORM_LIBRARIES
        "-framework UserNotifications"
        "-framework Foundation"
        "-framework UIKit")
endif()

################################################################################

qt_add_library(MobileNotification STATIC)

qt_add_qml_module(MobileNotification
    URI MobileNotification
    VERSION 1.0
    PLUGIN_TARGET MobileNotification_plugin
    SOURCES ${CORE_SOURCES} ${PLATFORM_SOURCES}
    #OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml/MobileNotification"
)

target_link_libraries(MobileNotification PUBLIC ${CORE_LIBRARIES})
target_link_libraries(MobileNotification PRIVATE ${PLATFORM_LIBRARIES})

target_include_directories(MobileNotification PUBLIC ${CMAKE_CURRENT_LIST_DIR})

################################################################################

if (IOS)
    # Enable ARC for the Objective-C++ source
    set_target_properties(MobileNotification PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
    target_compile_options(MobileNotification PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
endif()

################################################################################

# On Android the module owns its Java sources and (optional) resources.
# We copy these files into QT_ANDROID_PACKAGE_SOURCE_DIR at configure time.
# We do not overwrite existing resources, because the parent application is free to use its own.
if (ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android/src")
    # If by any chance QT_ANDROID_PACKAGE_SOURCE_DIR exists as a variable (?) we use it
    if (NOT DEFINED MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR AND DEFINED QT_ANDROID_PACKAGE_SOURCE_DIR)
        set(MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR "${QT_ANDROID_PACKAGE_SOURCE_DIR}")
    endif()

    # If by any chance the directory "${CMAKE_SOURCE_DIR}/assets/android/" exists, we use it
    if (NOT DEFINED MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR AND EXISTS "${CMAKE_SOURCE_DIR}/assets/android")
        set(MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/android")
    endif()

    if (DEFINED MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR)
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/android/src"
             DESTINATION "${MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR}")

        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android/res")
            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/android/res"
                 DESTINATION "${MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR}")
        endif()

        message(STATUS "MobileNotification > Deployed Android Java/res into ${MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR}")

        # Build-time re-deploy, to iterate on the module's Java/res WITHOUT a CMake
        # reconfigure (the file(COPY) above only runs at configure time). Invoke on
        # demand with:  cmake --build <dir> --target MobileNotification_deploy_android
        # In Debug builds it is also added to ALL, so it re-copies on every build.
        set(_mobilenotification_deploy_all "")
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_mobilenotification_deploy_all ALL)
        endif()
        add_custom_target(MobileNotification_deploy_android ${_mobilenotification_deploy_all}
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/android/src"
                    "${MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR}/src"
            COMMENT "MobileNotification > Re-deploying Android Java/res (dev)"
            VERBATIM)
        unset(_mobilenotification_deploy_all)
    else()
        message(WARNING "MobileNotification > MOBILENOTIFICATION_ANDROID_PACKAGE_SOURCE_DIR is not set, "
                        "please provide the same path than your application's QT_ANDROID_PACKAGE_SOURCE_DIR before add_subdirectory(MobileNotification).")
    endif()
endif()

################################################################################
