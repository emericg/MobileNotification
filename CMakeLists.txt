cmake_minimum_required(VERSION 3.20)

project(MobileNotification LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

################################################################################

set(ANDROID_DEPLOY_PATH "AUTO" CACHE STRING "Path to deploy Android resources into (AUTO / OFF / path)")

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

if (ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android/src")
    # On Android the module owns its Java sources and (optional) resources.
    # We copy these files into QT_ANDROID_PACKAGE_SOURCE_DIR at configure time.
    # We do not overwrite existing resources, because the parent application is free to use its own.

    if (ANDROID_DEPLOY_PATH AND NOT ANDROID_DEPLOY_PATH STREQUAL "OFF")

        if (ANDROID_DEPLOY_PATH STREQUAL "AUTO")
            # If by any chance QT_ANDROID_PACKAGE_SOURCE_DIR exists as a variable, or if
            # one of the following directories exists (platforms/android/, assets/android/, android/)
            if (DEFINED QT_ANDROID_PACKAGE_SOURCE_DIR)
                set(ANDROID_DEPLOY_PATH "${QT_ANDROID_PACKAGE_SOURCE_DIR}")
            elseif (EXISTS "${CMAKE_SOURCE_DIR}/platforms/android")
                set(ANDROID_DEPLOY_PATH "${CMAKE_SOURCE_DIR}/platforms/android")
            elseif (EXISTS "${CMAKE_SOURCE_DIR}/assets/android")
                set(ANDROID_DEPLOY_PATH "${CMAKE_SOURCE_DIR}/assets/android")
            elseif (EXISTS "${CMAKE_SOURCE_DIR}/android")
                set(ANDROID_DEPLOY_PATH "${CMAKE_SOURCE_DIR}/android")
            else()
                message(WARNING "MobileNotification > ANDROID_DEPLOY_PATH is not set!")
            endif()
        endif()

        if (EXISTS "${ANDROID_DEPLOY_PATH}")
            # Copy Java sources
            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/android/src"
                 DESTINATION "${ANDROID_DEPLOY_PATH}")

            message(STATUS "MobileNotification > Deployed Android Java/res into ${ANDROID_DEPLOY_PATH}")
        else()
            message(WARNING "MobileNotification > ANDROID_DEPLOY_PATH ${ANDROID_DEPLOY_PATH} does not exist!")
        endif()
    endif()
endif()

################################################################################
