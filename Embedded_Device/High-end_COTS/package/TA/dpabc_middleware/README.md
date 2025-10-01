# Privacy-preserving distributed ABC library as a trusted application

## Description

This repository contains the code necessary to run [a privacy-preserving distributed ABC library](https://www.sciencedirect.com/science/article/pii/S2214212621001824) (dpabc from now on) in the [trusted execution environment](https://en.wikipedia.org/wiki/Trusted_execution_environment) provided by [OP-TEE](https://optee.readthedocs.io/en/latest/) as a trusted application (TA).

## Building

To build this test you'll need the tools provided by [OP-TEE](https://optee.readthedocs.io/en/latest/) it can be build manually but automatic building is recomended, the steps to automatic build are the following:

Install the [prerequisites](https://optee.readthedocs.io/en/latest/building/prerequisites.html#prerequisites) without actually building the solution, for Ubuntu 22 and later this is:

```
DEBIAN_FRONTEND=noninteractive
apt update && apt upgrade -y
apt install -y \
    adb \
    acpica-tools \
    autoconf \
    automake \
    bc \
    bison \
    build-essential \
    ccache \
    cpio \
    cscope \
    curl \
    device-tree-compiler \
    e2tools \
    expect \
    fastboot \
    flex \
    ftp-upload \
    gdisk \
    git \
    libattr1-dev \
    libcap-ng-dev \
    libfdt-dev \
    libftdi-dev \
    libglib2.0-dev \
    libgmp3-dev \
    libhidapi-dev \
    libmpc-dev \
    libncurses5-dev \
    libpixman-1-dev \
    libslirp-dev \
    libssl-dev \
    libtool \
    libusb-1.0-0-dev \
    make \
    mtools \
    netcat \
    ninja-build \
    python3-cryptography \
    python3-pip \
    python3-pyelftools \
    python3-serial \
    python-is-python3 \
    rsync \
    swig \
    unzip \
    uuid-dev \
    wget \
    xdg-utils \
    xterm \
    xz-utils \
    zlib1g-dev
curl https://storage.googleapis.com/git-repo-downloads/repo > /bin/repo && chmod a+x /bin/repo
```

Clone [OP-TEE](https://optee.readthedocs.io/en/latest/) and build the toolchains by following their [instructions](https://optee.readthedocs.io/en/latest/building/gits/build.html). To test with Qemu run:

```
mkdir <optee-dir>
cd <optee-dir>
repo init -u https://github.com/OP-TEE/manifest.git -m qemu_v8.xml && repo sync -j10
make -C build -j2 toolchains
```

Before bulding, clone this repository into the OP-TEE directory (<optee-dir>) and move the [test_dpabc](https://ants-gitlab.inf.um.es/jmanuel/dpabc_trusted_application/-/tree/main/test_dpabc?ref_type=heads) directory into <optee-dir>/optee_examples and run the [config script](https://ants-gitlab.inf.um.es/jmanuel/dpabc_trusted_application/-/blob/main/test_dpabc/config.sh?ref_type=heads).

```
git clone https://ants-gitlab.inf.um.es/jmanuel/dpabc_trusted_application
cp -r dpabc_trusted_application/test_dpabc optee_examples/
cd optee_examples/test_dpabc
./config.sh 64
```

You'll be prompted to choose between a list of eliptic curves, for this project only BLS12381 (31 on the list) is necessary, so type 31, press enter and then type 0 to finish.

Finally cd into OP-TEE build directory and buid the entire project.

```
cd ../../build
make -j$(nproc) check
```

This will build not only the tests but also the trusted application and a static library to be used in other applications that need to access the TA through this middleware. 

## Running

To run the tests in qemu get in OP-TEE's build directory and execute OP-TEE.

```
cd <optee-dir>/build
make run
```

Two additional consoles will pop-up one showing the debug information of the secure world (Secure World) and another showing the qemu emulation (Normal World), to initiate the emulation type 'c' in the qemu prompt, then logint into the normal world as root user with no password and execute: dpabc_test.

```
(qemu) c
```

```
buildroot login: root
# dpabc_test
```

## Project Structure

```
dpabc_middleware/
├── host
│   └── lib
│       └── aceunit
│           ├── include
│           └── lib
└── ta
    ├── include
    └── lib
        └── p-abc
              └── lib
                   └── pfecCwrapper
                       ├── docs
                       ├── include
                       ├── lib
                       │   ├── Miracl_BLS12381_32b
                       │   ├── Miracl_BLS12381_64b
                       │   └── Miracl_Core
                       └── src
                           ├── Miracl_BLS12381_32b
                           └── Miracl_BLS12381_64b
```
    
The project follows the structure of an OP-TEE [trusted application](https://optee.readthedocs.io/en/latest/building/trusted_applications.html) with a host application (host) which makes calls to the secure world via the api provided by the trusted application (ta).

The ta implements all the [dpabc library](https://www.sciencedirect.com/science/article/pii/S2214212621001824) functionality while the host uses [aceunit](https://github.com/christianhujer/aceunit) to implement unit testing of the library.

<!-- ## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers. -->
