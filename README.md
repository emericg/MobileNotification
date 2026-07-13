# MobileNotification

MobileNotification allows Qt / QML mobile applications to post notifications and set app icon badges.
 
You can see it in action in the [MobileNotification demo](https://github.com/emericg/MobileNotification_demo).

> Supports Qt 6.8+ with CMake.

> Supports iOS 16+. Tested up to iOS 17.7 devices.

> Supports Android 9+ (API 28). Tested up to Android 16 (API 36) devices.

> [!WARNING]
> Still a work in progress at the moment...


## Features

- Android notifications
- Android application icon badge number (best effort)
- iOS notifications
- iOS application icon badge number


## Quick start

### Build

To get started, simply checkout the MobileNotification repository as a submodule, or copy the
MobileNotification directory into your project, then include the `CMakeLists.txt` CMake project file:

```cmake
add_subdirectory(MobileNotification/)
target_link_libraries(${PROJECT_NAME} PRIVATE MobileNotification MobileNotification_plugin)
```

You might need some hacks so the QML Language Server recognize the MobileNotification module:

```cmake
set(QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/MobileNotification/" CACHE STRING "QML Modules import paths" FORCE)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
```

### Setup

> TODO

### Use

> TODO


## Documentation

> TODO


## Caveats

> TODO


## License

This project is licensed under the [MIT license](LICENSE.md).

> Copyright (c) Emeric Grange <emeric.grange@gmail.com>
