# Why?
To be familiar with foundationDB

# Targets
- :white_check_mark: Compiling foundationDB from source code.
- :white_check_mark: Initialize fdb cluster with built binary. 
- :white_check_mark: Issue KV operations (get, set, getrange, delete) with fdbcli.
- :white_check_mark: Use [C API](https://apple.github.io/foundationdb/api-c.html) to issue KV operations (get, set, getrange, delete).
- :white_check_mark: Measure single getrange performance.
- :white_check_mark: Compare single vs multiple (parallel) getrange performance.
- :white_check_mark: Understand [read snapshot](https://apple.github.io/foundationdb/developer-guide.html#snapshot-isolation).
- :white_check_mark: Understand [transaction conflict](https://apple.github.io/foundationdb/developer-guide.html#conflict-ranges).

# Steps
## preparation
1. Install g++-11, gcc-11. I used to use g++-13, gcc-13 from build-essentials, but somehow broken during hours of compilation...
```
sudo apt install -y g++-11, gcc-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 13
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 13
sudo update-alternatives --set g++ /usr/bin/g++-11
sudo update-alternatives --set gcc /usr/bin/gcc-11

```

2. Clone the foundationDB [source code](https://www.github.com/apple/foundationdb), checkout to the latest release, here I user 7.3.43.
```
cd /opt
git clone https://www.github.com/apple/foundationdb
cd foundationdb
git checkout 7.3.43
```
### manual fix
There is a commit about missing the third parameters for system call open in the main branch, but not merged/cherry-picked in the release.
This will break the compiling process, but I don't know why their CI system didn't complain about this.
https://github.com/apple/foundationdb/commit/c1ba5d5733bea67b3a6f4a6ff8246e4137cb3ba4#diff-0977e6820509adb180b6798786d671503de757a951e9b7725d0966589d8a6044R692

3. Install cmake Version 3.13 or higher [CMake](https://cmake.org/)
```
sudo apt install -y cmake
```

4. Install [Mono](https://www.mono-project.com/download/stable/)

```
sudo gpg --homedir /tmp --no-default-keyring --keyring /usr/share/keyrings/mono-official-archive-keyring.gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
echo "deb [signed-by=/usr/share/keyrings/mono-official-archive-keyring.gpg] https://download.mono-project.com/repo/ubuntu stable-focal main" | sudo tee /etc/apt/sources.list.d/mono-official-stable.list
sudo apt update
sudo apt install -y mono-dev

```

5. Install [Ninja](https://ninja-build.org/)
```
sudo apt install -y ninja
```

6. Install missing packages from foundationDB instruction: [jemalloc](https://www.github.com/jemalloc/jemalloc), autoconf(Required to compile jemalloc from source code, more accurately, the autogen.sh script requires it.), liblz4-dev, python3-pip, python3-venv

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

7. Checkout into the foundationDB directory, create a blank build directory.
```
cd <PATH_TO_FOUNDATIONDB_DIRECTORY>
mkdir build

```

8. Build with the following instruction, USE_WERROR is not set due to the warning messages from deprecated calls since OpenSSL 3.0:
```
cd build
cmake -G Ninja ..
ninja
```

9. Try to run ctest, but the test_venv_setup tests using deprecated distutils, which removed from python3.10. My Ubuntu 24.04 uses python3.12, so I decided to skip it for now.
(It's also required to install libffi-dev for this test)

Edit:
Even if I install python3.8 and distutils, the test (test_venv_setup) still failed.
It failed at `python3 setup.py install`, which is also deprecated.

## Initialization
1. Use the built binary (fdbmonitor, fdbcli), cluster file, configuration file to setup new database in the cluster
```
# The server side

cd <PATH_TO_FOUNDATIONDB_DIRECTORY>/build
./bin/fdbmonitor --conffile ./sandbox/foundationdb.conf --lockfile /tmp/fdbmonitor.pid

```

```
# The client side

cd <PATH_TO_FOUNDATIONDB_DIRECTORY>/build
./bin/fdbcli -C ./fdb.cluster

```
2. For the first time, fdbcli will throw a confusing error message: 'The database is unavailable.' Even though the log message says the connection was refused, it did connect to the cluster. The real problem is that there is no database initialized in this cluster. So we should create one inside the fdbcli.

```
fdb> configure new single ssd

```
Note that there is no corresponding log shown in the fdbmonitor, which is also confusing.

3. Then we could type status in fdbcli to check the database status. If we exit the fdbcli and reentry, the welcome message will include 'The database is available.'.

```
fdb> status

```
## Development setup
1. Because I build this repository from source code, I need to install those headers (.h) and shared libraries (.so) before my development. The default path for installation is /usr/local

```
cd <PATH_TO_FOUNDATIONDB_DIRECTORY>/build
sudo cmake --install ./bindings/c/

```

## Development note
1. In getrange, if we want to obtain the whole user key ranges [\x00, \xff), even though using \xff with 0 end_or_equal, fdb will throw "Key outside legal range. Currently I use \xfe as end key, because I don't use key starting with \xfe.

2. It's not uncommon to forget `fdb_future_block_until_ready(FDBFuture*)` normally will not return error code, even so, it does not return the error about the corresponding future. Use `fdb_future_get_error(FDBFuture*)` to obtain the required error code, especially for the future from `fdb_transaction_commit(FDBTransaction* tr)`.

