# CERTIFY OP-TEE

## Building

To test this device in qemu you'll need the tools provided by [OP-TEE](https://optee.readthedocs.io/en/latest/) it can be build manually but automatic building is recomended, the steps to automatic build are the following:

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

Before bulding, clone this repository into the OP-TEE directory (<optee-dir>) and move the [security_api](https://ants-gitlab.inf.um.es/jmanuel/certify-op-tee/-/tree/main/security_api) directory into <optee-dir>/optee_examples and run the config script.

```
git clone https://ants-gitlab.inf.um.es/jmanuel/certify-op-tee
cp -r dpabc_trusted_application/security_api optee_examples/
cd optee_examples/security_api
./config.sh 64
```

Before compilation make sure to add the contents of "package/qemu_configuration/changes_to_common.txt" to the common.mk file under the build directory.

Finally cd into OP-TEE build directory and buid the entire project.

```
cd build
make -j$(nproc) check QEMU_VIRTFS_AUTOMOUNT=y BR2_PACKAGE_PYTHON3_ZLIB=y BR2_PACKAGE_PYTHON3_REQUESTS=y BR2_PACKAGE_PYTHON3=y BR2_PACKAGE_ZLIB=y BR2_PACKAGE_LIBCURL=y BR2_PACKAGE_LIBCURL_CURL=y BR2_PACKAGE_PYTHON3_PYEXPAT=y BR2_PACKAGE_PYTHON3_XML=y BR2_PACKAGE_PYTHON3_SSL=y BR2_PACKAGE_PYTHON_PIP=y
```

## Running

To run the tests in qemu get in OP-TEE's build directory and execute OP-TEE.

```
cd <optee-dir>/build
echo 'c' | make run-only QEMU_VIRTFS_AUTOMOUNT=y
```

Two additional consoles will pop-up one showing the debug information of the secure world (Secure World) and another showing the qemu emulation (Normal World), to initiate the emulation type 'c' in the qemu prompt, then logint into the normal world as root user with no password and execute: test_dpabc.

```
(qemu) c
```

```
buildroot login: root
# security_api_test
```
    
## Agents
The agents code is written in python and located under "package/scripts"
