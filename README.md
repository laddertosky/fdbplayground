# Why?
To be familiar with foundationDB

# Targets
:white_large_square Compiling foundationDB from source code.
:white_large_square 

# Steps
## preparation
1. Install g++-11, gcc-11. I used to use g++-13, gcc-13 from build-essentials, but somehow broken during hours of compilation...
```
sudo apt install -y g++-11, gcc-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11
sudo update-alternatives --set g++ /usr/bin/g++-11
sudo update-alternatives --set gcc /usr/bin/gcc-11

```

1. Clone the foundationDB [source code](https://www.github.com/apple/foundationdb), checkout to the latest release, here I user 7.3.43.
```
cd /opt
git clone https://www.github.com/apple/foundationdb
cd foundationdb
git checkout 7.3.43
```
### manual fix
There is a commit about missing the third parameters for system call open in the main branch, but not merged/cherry-picked in the release.
This will break the compiling process, but I don't know why their CI system didn't complaint about this.
https://github.com/apple/foundationdb/commit/c1ba5d5733bea67b3a6f4a6ff8246e4137cb3ba4#diff-0977e6820509adb180b6798786d671503de757a951e9b7725d0966589d8a6044R692

1. Install cmake Version 3.13 or higher [CMake](https://cmake.org/)
```
sudo apt install -y cmake
```

1. Install [Mono](https://www.mono-project.com/download/stable/)

```
sudo gpg --homedir /tmp --no-default-keyring --keyring /usr/share/keyrings/mono-official-archive-keyring.gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
echo "deb [signed-by=/usr/share/keyrings/mono-official-archive-keyring.gpg] https://download.mono-project.com/repo/ubuntu stable-focal main" | sudo tee /etc/apt/sources.list.d/mono-official-stable.list
sudo apt update
sudo apt install -y mono-dev

```

1. Install [Ninja](https://ninja-build.org/)
```
sudo apt install -y ninja
```

1. Install missing packages from foundationDB instruction: [jemalloc](https://www.github.com/jemalloc/jemalloc), autoconf(Required to compile jemalloc from source code, more accurately, the autogen.sh script requires it.), liblz4-dev, python3-pip, python3-venv

Compiling and installng jemaloc with latest release, here I use 5.3.0:
```
cd /opt
sudo apt install -y autoconf

git clone https://www.github.com/jemalloc/jemalloc
cd jemalloc
git checkout 5.3.0

./autogen.sh
make
sudo make install

```

Installing other packages with apt:
```
sudo apt install -y \
    liblz4-dev \
    python3-pip \
    python3-venv
```

1. Checkout into the foundationDB directory, create a blank build directory.
```
cd <PATH_TO_FOUNDATIONDB_DIRECTORY>`
mkdir build

```

1. Build with the following instruction, USE_WERROR is not set due to the waring messages from deprecated calls since OpenSSL 3.0:
```
cd build
cmake -G Ninja ..
ninja
```
