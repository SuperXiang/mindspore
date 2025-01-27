/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_RUNTIME_DEVICE_CPU_KERNEL_SELECT_CPU_H_
#define MINDSPORE_CCSRC_RUNTIME_DEVICE_CPU_KERNEL_SELECT_CPU_H_

#include <utility>
#include <string>
#include <vector>

#include "ir/anf.h"
#include "ir/dtype/type.h"
#include "include/common/utils/utils.h"

namespace mindspore {
namespace device {
namespace cpu {
using DataType = std::pair<TypeId, std::string>;

void SetKernelInfo(const CNodePtr &apply_kernel_ptr);
// Indicate whether the kernel input/output number are variable.
bool IsDynamicParamKernel(const std::string &op_name);

class KernelAttr {
 public:
  KernelAttr() : all_same_(0) {}
  ~KernelAttr() = default;

  KernelAttr &AddInputAttr(const TypeId &ms_type, const std::string &format = kOpFormat_DEFAULT) {
    input_type_.emplace_back(ms_type, format);
    return *this;
  }

  KernelAttr &AddOutputAttr(const TypeId &ms_type, const std::string &format = kOpFormat_DEFAULT) {
    output_type_.emplace_back(ms_type, format);
    return *this;
  }

  KernelAttr &SetAllSameAttr(bool all_same) {
    all_same_ = all_same;
    return *this;
  }

  KernelAttr &AddOutInRef(size_t output_index, size_t input_index) {
    out_in_ref_map_[output_index] = input_index;
    return *this;
  }

  const DataType &GetInputAttr(const size_t index) const { return input_type_[index]; }
  const DataType &GetOutputAttr(const size_t index) const { return output_type_[index]; }
  bool GetAllSame() const { return all_same_; }
  void SetInputAttrList(const std::vector<DataType> &addr_list) {
    input_type_.assign(addr_list.begin(), addr_list.end());
  }

  size_t GetInputSize() const { return input_type_.size(); }
  size_t GetOutputSize() const { return output_type_.size(); }
  const OutputInputRefMap &GetOutInRefMap() const { return out_in_ref_map_; }

 private:
  std::vector<DataType> input_type_;
  std::vector<DataType> output_type_;
  // The map between kernel's output and input ref relationship.
  OutputInputRefMap out_in_ref_map_;
  bool all_same_;
};
}  // namespace cpu
}  // namespace device
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_RUNTIME_DEVICE_CPU_KERNEL_SELECT_CPU_H_
