# 构建文档

软电话群呼客户端。

## Windows 平台构建

### 依赖安装

环境要求：

1. CMake >= 3.10
2. Visual Studio / MSVC（推荐 2019 或更高版本，安装时需勾选 C++ 开发组件）
2. Git

> Windows 平台编译第三方库建议新建一个 `thrid_party` 的文件夹，将库克隆至统一位置便于管理。
>

#### Boost 编译

```powershell
git clone https://github.com/boostorg/boost.git
cd boost
bootstrap.bat
b2 --prefix="C:\Boost" --build-dir=build --build-type=complete toolset=msvc variant=release threading=multi address-model=64 install 
```

#### PjProject 编译

```powershell
git clone https://github.com/pjsip/pjproject.git
```

1. 复制 `pjlib/include/pj/config_site_sample.h` 为 `pjlib/include/pj/config_site.h`。

   删除默认宏，添加以下宏定义：

   ```cpp
   #ifndef __PJ_CONFIG_SITE_H__
   #define __PJ_CONFIG_SITE_H__
   
   #define PJSUA_MAX_ACC 128
   #define PJSUA_MAX_CALLS 64
   #define PJSUA_MAX_BUDDIES 128
   #define PJSUA_MAX_CONF_PORTS 256
   
   #endif
   ```

2. 使用 Visual Studio 打开 `pjproject` 目录中的 `pjproject-vs14.sln`（位于 `pjproject` 根目录）。

3. 在 Visual Studio 中选择 `x64` 和 `Release` 配置，生成解决方案。

4. 生成的库文件位于各子项目的 `lib` 目录。

#### JsonCpp 编译

```powershell
git clone https://github.com/open-source-parsers/jsoncpp.git
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 -T host=x64
cmake --build build --config Release
```

#### spdlog 编译

```powershell
git clone https://github.com/gabime/spdlog.git
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 -T host=x64
cmake --build build --config Release
```

#### OpenSSL 安装

打开 [Win32 OpenSSL Download](https://slproweb.com/products/Win32OpenSSL.html) 页面，下载 [Win64OpenSSL-3_5_2.exe](https://slproweb.com/download/Win64OpenSSL-3_5_2.exe)。

安装后使用在 `CMakeLists.txt` 中修改包含 OpenSSL 库的路径。

### 项目编译

```powershell
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 -T host=x64
cmake --build build --config Release
```

编译完成后，执行文件在 `build/Release` 目录下。

## Linux 平台构建

### 依赖安装

环境要求：

- CMake >= 3.10
- GCC >= 11 或 Clang >= 12
- Git

Linux 平台建议使用包管理器，方便管理，也可以源码编译。

```sh
# apt
sudo apt update
sudo apt install -y build-essential cmake git pkg-config \
    libboost-all-dev libssl-dev libspdlog-dev libjsoncpp-dev

# pacman
sudo pacman -Syyu
sudo pacman -S boost openssl spdlog jsoncpp

# 其他包管理器同理
```

#### PjProject 编译

```
git clone https://github.com/pjsip/pjproject.git

cd pjproject
./configure
make
sudo make install
```

### 项目编译

```sh
cmake -B build
cmake --build build -j$(nproc)
```

## 注意事项

- 由于 `PjProject` 在 Windows 平台使用 Visual Studio 编译较为方便，所以为了兼容性，其余的库均使用 Visual Studio 进行编译。
- 如果使用动态库，则需要设置 `PATH`（Windows） 或 `LD_LIBRARY_PATH`（Linux）指向库文件所在目录。
- 如果遇到第三方库 `CMakeLists.txt` 出现版本较低的问题时，可以手动修改 `cmake_minimum_required(VERSION 3.10)` 里的版本。
