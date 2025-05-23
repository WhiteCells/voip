### voip

#### Dependenice

- C++17
- CMake >= 3.10
- pjsip pjsua2
- boost
  - asio
  - beast
- jsoncpp
- spdlog

```json
git submodule update --init --recursive
```

#### Build

```sh
cmake -B build
cmake --build build -j$(nproc)
./build/voip/voip
```

#### Docker

```sh
docker build -t voip -f dockerfile .
docker run -it voip bash
```
