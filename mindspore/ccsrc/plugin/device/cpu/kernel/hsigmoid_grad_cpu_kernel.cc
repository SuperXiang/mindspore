/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "plugin/device/cpu/kernel/hsigmoid_grad_cpu_kernel.h"
#include <algorithm>
#include "plugin/device/cpu/hal/device/cpu_device_address.h"

namespace mindspore {
namespace kernel {
namespace {
constexpr size_t kHSigmoidGradInputsNum = 2;
constexpr size_t kHSigmoidGradOutputsNum = 1;
}  // namespace

template <typename T>
void HSigmoidGradCpuKernelMod<T>::InitKernel(const CNodePtr &kernel_node) {
  MS_EXCEPTION_IF_NULL(kernel_node);
  kernel_name_ = common::AnfAlgo::GetCNodeName(kernel_node);
  x_shape_ = common::AnfAlgo::GetPrevNodeOutputInferShape(kernel_node, 1);
  for (const uint64_t &d : x_shape_) {
    tensor_size_ *= d;
  }
}

template <typename T>
bool HSigmoidGradCpuKernelMod<T>::Launch(const std::vector<kernel::AddressPtr> &inputs,
                                         const std::vector<kernel::AddressPtr> &,
                                         const std::vector<kernel::AddressPtr> &outputs) {
  CHECK_KERNEL_INPUTS_NUM(inputs.size(), kHSigmoidGradInputsNum, kernel_name_);
  CHECK_KERNEL_OUTPUTS_NUM(outputs.size(), kHSigmoidGradOutputsNum, kernel_name_);
  const auto *dy = reinterpret_cast<T *>(inputs[0]->addr);
  const auto *x = reinterpret_cast<T *>(inputs[1]->addr);
  auto *out = reinterpret_cast<T *>(outputs[0]->addr);

  auto zero = static_cast<T>(0);
  auto three = static_cast<T>(3);
  auto six = static_cast<T>(6);

  auto task = [&](size_t start, size_t end) {
    for (uint64_t i = start; i < end; ++i) {
      if (x[i] + three <= zero || x[i] >= three) {
        out[i] = zero;
      } else {
        out[i] = dy[i] / six;
      }
    }
  };
  ParallelLaunchAutoSearch(task, tensor_size_, this, &parallel_search_info_);
  return true;
}
}  // namespace kernel
}  // namespace mindspore
