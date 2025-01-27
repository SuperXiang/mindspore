#!/bin/bash
# Copyright 2022 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

# Prepare and Install mindspore gpu by pip on Ubuntu 18.04.
#
# This file will:
#   - change deb source to huaweicloud mirror
#   - install mindspore dependencies via apt like gcc, libgmp
#   - install python3 & pip3 via apt and set it to default
#   - install CUDA by run file and cudnn via apt.
#   - install mindspore-cpu within new installed python by pip
#   - compile and install Open MPI if OPENMPI is set to on.
#
# Augments:
#   - PYTHON_VERSION: python version to install. [3.7(default), 3.9]
#   - MINDSPORE_VERSION: mindspore version to install, default 1.6.0
#   - CUDA_VERSION: CUDA version to install. [10.1(default), 11.1]
#   - OPENMPI: whether to install optional package Open MPI for distributed training. [on, off(default)]
#
# Usage:
#   Need root permission to run, like `sudo bash -i ./ubuntu-gpu-pip.sh`.
#   To set augments, run it as `sudo PYTHON_VERSION=3.9 CUDA_VERSION=11.1 OPENMPI=on bash -i ./ubuntu-gpu-pip.sh`.

set -e

PYTHON_VERSION=${PYTHON_VERSION:-3.7}
MINDSPORE_VERSION=${MINDSPORE_VERSION:-1.6.0}
CUDA_VERSION=${CUDA_VERSION:-10.1}
OPENMPI=${OPENMPI:-off}

available_py_version=(3.7 3.9)
if [[ " ${available_py_version[*]} " != *" $PYTHON_VERSION "* ]]; then
    echo "PYTHON_VERSION is '$PYTHON_VERSION', but available versions are [${available_py_version[*]}]."
    exit 1
fi
available_cuda_version=(10.1 11.1)
if [[ " ${available_cuda_version[*]} " != *" $CUDA_VERSION "* ]]; then
    echo "CUDA_VERSION is '$CUDA_VERSION', but available versions are [${available_cuda_version[*]}]."
    exit 1
fi
declare -A minimum_driver_version_map=()
minimum_driver_version_map["10.1"]="418.39"
minimum_driver_version_map["11.1"]="450.80.02"
driver_version=$(modinfo nvidia | grep ^version | awk '{printf $2}')
if [[ $driver_version < ${minimum_driver_version_map[$CUDA_VERSION]} ]]; then
    echo "CUDA $CUDA_VERSION minimum required driver version is ${minimum_driver_version_map[$CUDA_VERSION]}, \
        but current nvidia driver version is $driver_version, please upgrade your driver manually."
    exit 1
fi
cuda_name="cuda-$CUDA_VERSION"

declare -A version_map=()
version_map["3.7"]="${MINDSPORE_VERSION}-cp37-cp37m"
version_map["3.9"]="${MINDSPORE_VERSION}-cp39-cp39"

# add value to environment variable if value is not in it
add_env() {
    local name=$1
    if [[ ":${!name}:" != *":$2:"* ]]; then
        echo -e "export $1=$2:\$$1" >> ~/.bashrc
    fi
}

# use huaweicloud mirror in China
sed -i "s@http://.*archive.ubuntu.com@http://repo.huaweicloud.com@g" /etc/apt/sources.list
sed -i "s@http://.*security.ubuntu.com@http://repo.huaweicloud.com@g" /etc/apt/sources.list
apt-get update

apt-get install curl make gcc-7 libgmp-dev linux-headers-"$(uname -r)" -y

# python
add-apt-repository -y ppa:deadsnakes/ppa
apt-get install python$PYTHON_VERSION python$PYTHON_VERSION-distutils python3-pip -y
update-alternatives --install /usr/bin/python python /usr/bin/python$PYTHON_VERSION 100
# pip
sudo -u $SUDO_USER python -m pip install pip -i https://pypi.tuna.tsinghua.edu.cn/simple
update-alternatives --install /usr/bin/pip pip ~/.local/bin/pip$PYTHON_VERSION 100
update-alternatives --install /usr/local/bin/pip pip ~/.local/bin/pip$PYTHON_VERSION 100
sudo -u $SUDO_USER pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple

# install cuda/cudnn
cd /tmp
declare -A cuda_url_map=()
cuda_url_map["10.1"]=https://developer.download.nvidia.com/compute/cuda/10.1/Prod/local_installers/cuda_10.1.243_418.87.00_linux.run
cuda_url_map["11.1"]=https://developer.download.nvidia.com/compute/cuda/11.1.1/local_installers/cuda_11.1.1_455.32.00_linux.run
cuda_url=${cuda_url_map[$CUDA_VERSION]}
wget $cuda_url
sh ${cuda_url##*/} --silent --toolkit
cd -
apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub
add-apt-repository "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/ /"
add-apt-repository "deb https://developer.download.nvidia.cn/compute/machine-learning/repos/ubuntu1804/x86_64/ /"
apt-get update
declare -A cudnn_name_map=()
cudnn_name_map["10.1"]="libcudnn7=7.6.5.32-1+cuda10.1 libcudnn7-dev=7.6.5.32-1+cuda10.1"
cudnn_name_map["11.1"]="libcudnn8=8.0.4.30-1+cuda11.1 libcudnn8-dev=8.0.4.30-1+cuda11.1"
apt-get install --no-install-recommends ${cudnn_name_map[$CUDA_VERSION]} -y

# add cuda to path
set +e && source ~/.bashrc
set -e
add_env PATH /usr/local/cuda/bin
add_env LD_LIBRARY_PATH /usr/local/cuda/lib64
add_env LD_LIBRARY_PATH /usr/lib/x86_64-linux-gnu
set +e && source ~/.bashrc
set -e

# optional openmpi for distributed training
if [[ X"$OPENMPI" == "Xon" ]]; then
    cd /tmp
    sudo -u $SUDO_USER curl -O https://download.open-mpi.org/release/open-mpi/v4.0/openmpi-4.0.3.tar.gz
    sudo -u $SUDO_USER tar xzf openmpi-4.0.3.tar.gz
    cd openmpi-4.0.3
    sudo -u $SUDO_USER ./configure --prefix=/usr/local/openmpi-4.0.3
    sudo -u $SUDO_USER make
    make install
    add_env PATH /usr/local/openmpi-4.0.3/bin
    add_env LD_LIBRARY_PATH /usr/local/openmpi-4.0.3/lib
fi

arch=`uname -m`
sudo -u $SUDO_USER pip install https://ms-release.obs.cn-north-4.myhuaweicloud.com/${MINDSPORE_VERSION}/MindSpore/gpu/${arch}/${cuda_name}/mindspore_gpu-${version_map["$PYTHON_VERSION"]}-linux_${arch}.whl --trusted-host ms-release.obs.cn-north-4.myhuaweicloud.com -i https://pypi.tuna.tsinghua.edu.cn/simple

# check mindspore installation
python -c "import mindspore;mindspore.run_check()"

# check if it can be run with GPU
cd /tmp
cat > example.py <<END
import numpy as np
from mindspore import Tensor
import mindspore.ops as ops
import mindspore.context as context

context.set_context(device_target="GPU")
x = Tensor(np.ones([1,3,3,4]).astype(np.float32))
y = Tensor(np.ones([1,3,3,4]).astype(np.float32))
print(ops.add(x, y))
END
python example.py
cd -
