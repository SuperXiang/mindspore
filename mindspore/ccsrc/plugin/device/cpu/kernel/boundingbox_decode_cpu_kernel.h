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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_BOUNDINGBOX_DECODE_CPU_KERNEL_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_BOUNDINGBOX_DECODE_CPU_KERNEL_H_

#include <vector>
#include <algorithm>

#include "plugin/device/cpu/kernel/cpu_kernel.h"
#include "plugin/device/cpu/kernel/cpu_kernel_factory.h"

namespace mindspore {
namespace kernel {
constexpr size_t MIN_MAX_SHAPE_SIZE = 2;
constexpr size_t INPUT_NUMS = 2;
template <typename T>
class BoundingBoxDecodeCpuKernelMod : public NativeCpuKernelMod {
 public:
  BoundingBoxDecodeCpuKernelMod() = default;
  ~BoundingBoxDecodeCpuKernelMod() override = default;

  void InitKernel(const CNodePtr &kernel_node) override;

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &,
              const std::vector<AddressPtr> &outputs) override;

 private:
  std::vector<float> means_;
  std::vector<float> stds_;
  std::vector<int> max_shape_;
  float wh_ratio_clip_{0.016};
};

MS_REG_CPU_KERNEL_T(
  BoundingBoxDecode,
  KernelAttr().AddInputAttr(kNumberTypeFloat32).AddInputAttr(kNumberTypeFloat32).AddOutputAttr(kNumberTypeFloat32),
  BoundingBoxDecodeCpuKernelMod, float);

MS_REG_CPU_KERNEL_T(
  BoundingBoxDecode,
  KernelAttr().AddInputAttr(kNumberTypeFloat16).AddInputAttr(kNumberTypeFloat16).AddOutputAttr(kNumberTypeFloat16),
  BoundingBoxDecodeCpuKernelMod, float16);

}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_BOUNDINGBOX_DECODE_CPU_KERNEL_H_
