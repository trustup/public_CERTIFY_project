# P-ABC

P-ABC library (distributed privacy-preserving Attribute-Based Credentials based on [PS multisignatures](https://eprint.iacr.org/2020/016)) using wrapper library for [EC arithmetic](./lib/pfecCwrapper/README.md).

The current version of the project uses the [Miracle Core Library](https://github.com/miracl/core) C implementation of EC arithmetic.


## Instalation and build
The project is based on cmake, and should work for both Linux and Windows platforms (**note**: *for now it has mainly been tested in Debian and Windows 10, using the  -G "Unix Makefiles" option in cmake for building*). Some of the potential issues we found are detailed in other parts of the doc.

The following sections in this Readme give a detailed breakdown on the setup, build, testing and outputs of the library. However, it is possible to use the script ''setupscript.sh'' for automatizing the processes. We encourage its use only for setup, or when needed for pipelines/jobs, but advise the usual build method (cmake/make) for normal use (especially if you plan to build multiple times/work with the code). More details in the specific [section](README.md#automatization-script).

### Setup
Before being able to use the project, you need to make sure that you have the necessary tools installed, namely gcc, (g++ can also be requested by cmake even if the whole project uses strictly C), make and cmake. If you do not have them installed, an example on how to set them up would be:
```
apt-get install gcc
echo export CC=/usr/bin/gcc >>~/.bashrc
apt-get install g++
echo export GXX=/usr/bin/g++ >>~/.bashrc
apt-get install cmake
```

Additionally, the project has two extra dependencies. 
- First, the [Miracle/core](https://github.com/miracl/core/tree/master/c) library is currently used as the implementation of the mathematical operations of pairining-friendly elliptic curves. Thus, you need to compile the respective C library for BLS12381 (following the instruction in the link), and copy the file with the name core_unix.a (or core_win.a if on Windows) to the respective directory (Miracl_BLS12381_32b or Miracl_BLS12381_64b depending on which bit configuration you used).
- Second, the Cmocka framework is used for testing. As tests are currently included in the build by default (plans for adding a configurable option in the future), you need to install Cmocka before building the project. To do that, you can follow the steps in pfecCwrapper/test/cmocka/INSTALL.md.

### Build
The build process is the same as with other cmake projects:
```sh
mkdir build
cd build
cmake ..
```
Compile from generated Makefile (In build directory):
```sh
make
```
There is a configuration parameter that establishes the specific wrapper instantiation that will be used for building the dpabc implementation. The previous commands will build the dpabc library with the default value (**NOTE** Once you have set the variable, it is stored in CMake cache, so even if you run *cmake ..* without arguments it will use the last value you set for the variable). Currently, you can choose between two instantiations of the pfecCwrapper library:
* "pfec_Miracl_Bls381_64": (**Default** value) Instantiataion of the BLS381 curve with a 64 bit representation (i.e., for better performance in 64 bit architectures), based on the [Miracle/core](https://github.com/miracl/core/tree/master/c) library. 
* "pfec_Miracl_Bls381_32": Instantiataion of the BLS381 curve with a 32 bit representation (i.e., for 32 bit architectures), based on the [Miracle/core](https://github.com/miracl/core/tree/master/c) library.

To do this, specify the instantiation on the cmake command as variable WRAPPER_INSTANTIATION. For instance, you can execute:
```sh
cd build
cmake -DWRAPPER_INSTANTIATION=pfec_Miracl_Bls381_32 ..
make
```

### Testing
The project supports testing of both the wrapper library and the dpabc implementation. Tests will be built by default, if you wish to disable it you need to comment the respective lines in the CMakeLists.txt files (plans for adding a configurable option in the future). 

Testing is based on the cmocka library. To be able to properly generate and run tests you need to complete the installation steps for cmocka, whichi is included in pfecCwrapper/test/cmocka/INSTALL.md (if you are on Windows, you will have to update the path to the cmocka dll in CMakeLists.txt for correct execution of the tests, or manually copy the dll to the build directory).
Once everything is built correctly, you can (verbosely) run the tests with the command (inside the build directory):
```sh
ctest -V
```

### Outputs
An example binary with a simple protocol execution will be generated to *build/output/bin* directory. Additionally, a static library for the dpabc code will be generated to *build/output/lib*. 
For convenience, the static library along with all the static libraries that it will need (i.e., wrapper ec library), are bundled into the file *build/libdpabc_psms_bundled.a*. 

Remember that, in both cases, if you want to use the methods for these libraries in your code you need to include the respective headers (i.e., those in the *include* directories from both projects).

For instance, if your *main.c* file (you can try it with the *src/example/main.c* from this project) uses the library, you may execute the following command (with each directory containing the corresponding files):
```
gcc.exe -I./include -I./include/ecwrapper main.c -L. -ldpabc_psms_bundled  
```

If you do not use the bundled library, you would need to include all the static libraries in the bundled directory:
```
 gcc.exe -I./include -I./include/ecwrapper main.c -L. -ldpabc_psms_bundled  -lpfec_Miracl_Bls381_64   -lcore_win  
```

### Automatization script
The ''setupscript.sh'' script allows automatic setup of the project, and optionally building, testing and exporting the bundled library. Its main purpose is the setup step, where the script:
- Checks whether Cmocka is installed, and performs the installation process otherwise using cmake/make install. **Note:** This process requires privileges (sudo).
- Compiles the Miracl/core library and copies the result to the corresponding directory

Additionally, the script accepts the following options:
- "-b": Build the project (with cmake and make)
- "-t": Test the project. Needs to be used along "-b" to work
- "-d <export_directory>": Defines a directory for copying the bundled library libdpabc_psms_bundled.a. Needs to be used along "-b" to work
- "-s": Skip setup.


## Documentation
You can genereta a documentation for the whole library by running *doxygen* in the docs folder


## Potential issues
The aforementioned potential issues with cmocka in Windows.

If there is a linking error with Miracl/core's library, you need to substitute the correspoinding static labrary (e.g., core_win.a) for the one generated by following the compilation steps on their repository [Miracle/core](https://github.com/miracl/core/tree/master/c)

