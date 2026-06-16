# vcpkg Configuration Export

This document exports the vcpkg configuration and dependencies used in this project for integration into another Visual Studio project.

## Overview

This project uses **vcpkg** for dependency management. The vcpkg integration is automatic through Visual Studio's built-in vcpkg support.

## vcpkg Integration Method

The project uses **automatic vcpkg integration** via Visual Studio, not manifest mode. This is evident from:

1. No `vcpkg.json` manifest file in the project root
2. Presence of `vcpkg.applocal.log` files showing DLL deployment
3. Standard Visual Studio project structure with `%(AdditionalIncludeDirectories)` and `%(AdditionalLibraryDirectories)` macros

## Required Dependencies

Based on the DLL deployment log and CMakeLists.txt, the following packages are required:

### Core Dependencies

```bash
# Install these packages via vcpkg
vcpkg install wxwidgets:x64-windows
vcpkg install boost-thread:x64-windows
vcpkg install boost-system:x64-windows
vcpkg install libarchive:x64-windows
vcpkg install zlib:x64-windows
vcpkg install freeglut:x64-windows
vcpkg install opengl:x64-windows
vcpkg install pugixml:x64-windows
```

### Dependency Details

From `vcpkg.applocal.log`, the following DLLs are deployed:

```
archive.dll          - libarchive
zlib1.dll            - zlib
bz2.dll              - bzip2 (libarchive dependency)
liblzma.dll          - lzma (libarchive dependency)
libcrypto-3-x64.dll  - OpenSSL (libarchive dependency)
lz4.dll              - lz4 (libarchive dependency)
zstd.dll             - zstd (libarchive dependency)
boost_thread-vc143-mt-x64-1_88.dll - Boost Thread
freeglut.dll         - FreeGLUT
libGLESv2.dll        - OpenGL ES (ANGLE)
wxbase32u_vc_x64_custom.dll - wxWidgets Base
pcre2-16.dll         - PCRE2 (wxWidgets dependency)
wxbase32u_net_vc_x64_custom.dll - wxWidgets Net
wxbase32u_xml_vc_x64_custom.dll - wxWidgets XML
libexpat.dll         - Expat (wxWidgets dependency)
wxmsw32u_aui_vc_x64_custom.dll - wxWidgets AUI
wxmsw32u_core_vc_x64_custom.dll - wxWidgets Core
jpeg62.dll           - libjpeg (wxWidgets dependency)
libpng16.dll         - libpng (wxWidgets dependency)
tiff.dll             - libtiff (wxWidgets dependency)
wxmsw32u_gl_vc_x64_custom.dll - wxWidgets OpenGL
wxmsw32u_html_vc_x64_custom.dll - wxWidgets HTML
```

## Visual Studio Project Configuration

### Project Properties

**Platform Toolset**: `v143` (Visual Studio 2022)
**Windows SDK**: `10.0.26100.0`
**C++ Standard**: `C++17` (`stdcpp17`)
**Character Set**: Unicode

### Preprocessor Definitions

**Debug Configuration**:
```
WIN32
_CRTDBG_MAP_ALLOC
__DEBUG__
WXUSINGDLL
wxMSVC_VERSION_AUTO
__EXPERIMENTAL__
LIVE_SERVER
```

**Release Configuration**:
```
NDEBUG
__RELEASE__
WXUSINGDLL
wxMSVC_VERSION_AUTO
_CRT_SECURE_NO_WARNINGS
```

### Include Directories

The project uses vcpkg's automatic include directory injection:
```xml
<AdditionalIncludeDirectories>%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
```

This macro automatically includes vcpkg-installed package headers.

### Library Directories

The project uses vcpkg's automatic library directory injection:
```xml
<AdditionalLibraryDirectories>%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
```

For Release x64, there's also a custom dependency path:
```xml
<AdditionalLibraryDirectories>$(SolutionDir)..\dependencies\vs\lib\;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
```

### Additional Dependencies

```xml
<AdditionalDependencies>comctl32.lib;Rpcrt4.lib;WS2_32.lib;%(AdditionalDependencies)</AdditionalDependencies>
```

- `comctl32.lib` - Windows Common Controls
- `Rpcrt4.lib` - RPC Runtime
- `WS2_32.lib` - Windows Sockets 2

### Compiler Options

**Release x64 Specific**:
```xml
<Optimization>Full</Optimization>
<InlineFunctionExpansion>AnySuitable</InlineFunctionExpansion>
<IntrinsicFunctions>true</IntrinsicFunctions>
<FavorSizeOrSpeed>Speed</FavorSizeOrSpeed>
<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
<FunctionLevelLinking>false</FunctionLevelLinking>
<MultiProcessorCompilation>true</MultiProcessorCompilation>
<BufferSecurityCheck>false</BufferSecurityCheck>
<AdditionalOptions>-Zm114 %(AdditionalOptions)</AdditionalOptions>
```

**Debug x64 Specific**:
```xml
<Optimization>Disabled</Optimization>
<BasicRuntimeChecks>EnableFastChecks</BasicRuntimeChecks>
<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>
<AdditionalOptions>/bigobj %(AdditionalOptions)</AdditionalOptions>
```

### Linker Options

**Release x64**:
```xml
<GenerateDebugInformation>true</GenerateDebugInformation>
<SubSystem>Windows</SubSystem>
<LargeAddressAware>true</LargeAddressAware>
<OptimizeReferences>true</OptimizeReferences>
<EnableCOMDATFolding>true</EnableCOMDATFolding>
```

**Debug x64**:
```xml
<GenerateDebugInformation>true</GenerateDebugInformation>
<GenerateMapFile>true</GenerateMapFile>
<MapFileName>$(TargetName).map</MapFileName>
<MapExports>true</MapExports>
<SubSystem>Windows</SubSystem>
```

## Setting Up vcpkg in Another Project

### Step 1: Install vcpkg

```bash
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Integrate with Visual Studio (run as administrator)
.\vcpkg integrate install
```

### Step 2: Install Required Packages

```bash
# Install all required packages for x64-windows
vcpkg install wxwidgets:x64-windows
vcpkg install boost-thread:x64-windows
vcpkg install boost-system:x64-windows
vcpkg install libarchive:x64-windows
vcpkg install zlib:x64-windows
vcpkg install freeglut:x64-windows
vcpkg install opengl:x64-windows
vcpkg install pugixml:x64-windows

# Or install all at once
vcpkg install wxwidgets boost-thread boost-system libarchive zlib freeglut opengl pugixml --triplet x64-windows
```

### Step 3: Configure Visual Studio Project

1. **Open your .vcxproj file** and ensure these settings:

```xml
<PropertyGroup Label="Globals">
  <VCProjectVersion>16.0</VCProjectVersion>
  <ProjectGuid>{YOUR-GUID-HERE}</ProjectGuid>
  <Keyword>Win32Proj</Keyword>
  <RootNamespace>YourProjectName</RootNamespace>
  <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
</PropertyGroup>

<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
  <ConfigurationType>Application</ConfigurationType>
  <UseDebugLibraries>false</UseDebugLibraries>
  <PlatformToolset>v143</PlatformToolset>
  <WholeProgramOptimization>true</WholeProgramOptimization>
  <CharacterSet>Unicode</CharacterSet>
</PropertyGroup>
```

2. **Add vcpkg include/library directories**:

```xml
<ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
  <ClCompile>
    <AdditionalIncludeDirectories>%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    <PreprocessorDefinitions>NDEBUG;__RELEASE__;WXUSINGDLL;wxMSVC_VERSION_AUTO;_CRT_SECURE_NO_WARNINGS;%(PreprocessorDefinitions)</PreprocessorDefinitions>
    <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
    <LanguageStandard>stdcpp17</LanguageStandard>
  </ClCompile>
  <Link>
    <AdditionalDependencies>comctl32.lib;WS2_32.lib;%(AdditionalDependencies)</AdditionalDependencies>
    <AdditionalLibraryDirectories>%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
    <SubSystem>Windows</SubSystem>
  </Link>
</ItemDefinitionGroup>
```

### Step 4: Verify Integration

After vcpkg integration, Visual Studio will automatically:
- Add vcpkg include directories to `%(AdditionalIncludeDirectories)`
- Add vcpkg library directories to `%(AdditionalLibraryDirectories)`
- Link required libraries automatically
- Deploy DLLs to output directory via `vcpkg.applocal.log`

## Alternative: Manifest Mode (vcpkg.json)

If you prefer manifest mode for better reproducibility, create a `vcpkg.json` file:

```json
{
  "name": "your-project-name",
  "version": "1.0.0",
  "dependencies": [
    "wxwidgets",
    "boost-thread",
    "boost-system",
    "libarchive",
    "zlib",
    "freeglut",
    "opengl",
    "pugixml"
  ],
  "builtin-baseline": "latest",
  "overrides": []
}
```

Then add to your .vcxproj:

```xml
<PropertyGroup Label="Vcpkg">
  <VcpkgEnabled>true</VcpkgEnabled>
  <VcpkgConfiguration>Release</VcpkgConfiguration>
  <VcpkgTriplet>x64-windows</VcpkgTriplet>
  <VcpkgManifestInstall>true</VcpkgManifestInstall>
</PropertyGroup>
```

## CMake Configuration (Alternative Build System)

If using CMake instead of Visual Studio projects:

```cmake
cmake_minimum_required(VERSION 3.1)
project(your-project)

# Set vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(OpenGL REQUIRED)
find_package(LibArchive REQUIRED)
find_package(Boost 1.34.0 COMPONENTS thread system REQUIRED)
find_package(wxWidgets COMPONENTS html aui gl adv core net base REQUIRED)
find_package(GLUT REQUIRED)
find_package(ZLIB REQUIRED)
find_package(pugixml CONFIG REQUIRED)

# Include directories
include(${wxWidgets_USE_FILE})
include_directories(
  ${Boost_INCLUDE_DIRS}
  ${LibArchive_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIR}
  ${GLUT_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(your-target
  ${wxWidgets_LIBRARIES}
  ${Boost_LIBRARIES}
  ${LibArchive_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${GLUT_LIBRARIES}
  ${ZLIB_LIBRARIES}
  pugixml::pugixml
)
```

## Troubleshooting

### Issue: vcpkg packages not found

**Solution**: Ensure vcpkg is integrated with Visual Studio:
```bash
vcpkg integrate install
```

### Issue: Wrong architecture (x86 vs x64)

**Solution**: Always specify the triplet when installing:
```bash
vcpkg install package-name:x64-windows
```

### Issue: DLLs not copied to output directory

**Solution**: vcpkg automatically copies DLLs via MSBuild targets. Ensure:
1. vcpkg integration is active
2. Project is using `%(AdditionalLibraryDirectories)` macro
3. Check `vcpkg.applocal.log` in output directory

### Issue: Linker errors with wxWidgets

**Solution**: Ensure preprocessor definitions include:
```
WXUSINGDLL
wxMSVC_VERSION_AUTO
```

## Additional Notes

### wxWidgets Custom Build

The DLL names show `_custom` suffix (e.g., `wxbase32u_vc_x64_custom.dll`), indicating a custom wxWidgets build. This might be:
- Custom configuration options
- Custom patches
- Specific feature set

If using standard vcpkg wxWidgets, DLL names will be different but functionality should be equivalent.

### Boost Version

The project uses Boost 1.88 (`boost_thread-vc143-mt-x64-1_88.dll`). vcpkg will install the latest version, which should be compatible.

### OpenGL Implementation

The project uses ANGLE (`libGLESv2.dll`) for OpenGL ES support on Windows. This is automatically included with vcpkg's OpenGL package.

---

## Quick Setup Script

Save this as `setup-vcpkg.bat`:

```batch
@echo off
echo Setting up vcpkg dependencies...

REM Check if vcpkg is installed
if not exist "vcpkg\vcpkg.exe" (
    echo Cloning vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    call bootstrap-vcpkg.bat
    cd ..
)

echo Installing packages...
vcpkg\vcpkg install wxwidgets:x64-windows
vcpkg\vcpkg install boost-thread:x64-windows
vcpkg\vcpkg install boost-system:x64-windows
vcpkg\vcpkg install libarchive:x64-windows
vcpkg\vcpkg install zlib:x64-windows
vcpkg\vcpkg install freeglut:x64-windows
vcpkg\vcpkg install opengl:x64-windows
vcpkg\vcpkg install pugixml:x64-windows

echo Integrating with Visual Studio...
vcpkg\vcpkg integrate install

echo Done! Open your Visual Studio solution and rebuild.
pause
```

---

## End of Export

This document contains all vcpkg configuration details needed to set up dependencies in another Visual Studio project.
