/**
 * Copyright 2020-2022 Huawei Technologies Co., Ltd
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

#include "plugin/device/cpu/kernel/mkldnn/mkl_cpu_kernel.h"
#include <vector>
#include <string>
#include <algorithm>
#include "utils/ms_utils.h"
#include "utils/profile.h"

namespace mindspore {
namespace kernel {
void MKLCpuKernelMod::GetPadding(const CNodePtr &kernel_node, const std::string &pad_mode,
                                 const std::vector<size_t> &src_shape, const std::vector<size_t> &kernel_size,
                                 const std::vector<int> &stride, std::vector<int> *padding_l,
                                 std::vector<int> *padding_r, const std::vector<int> &dilation) const {
  MS_EXCEPTION_IF_NULL(kernel_node);
  MS_EXCEPTION_IF_NULL(padding_l);
  MS_EXCEPTION_IF_NULL(padding_r);
  auto dim = src_shape.size();
  if (dim < 2) {
    MS_LOG(EXCEPTION) << "Set pad only support src dim >= 2!";
  }
  std::vector<int> weight_height;
  for (size_t i = 2; i < dim; ++i) {
    (void)weight_height.emplace_back(src_shape[i]);
  }

  MS_LOG(INFO) << "pad mode: " << pad_mode;
  if (pad_mode == PAD_MODE_LOWER_SAME || pad_mode == PAD_MODE_UPPER_SAME) {
    for (size_t i = 0; i < weight_height.size(); ++i) {
      auto wh = weight_height[i];
      int out = (wh + stride[i] - 1) / stride[i];
      int effective_k = (SizeToInt(kernel_size[i]) - 1) * dilation[i] + 1;
      int pad_along = std::max(0, (out - 1) * stride[i] + effective_k - wh);
      int pad = pad_along / 2;
      (void)padding_l->emplace_back(pad);
      (void)padding_r->emplace_back(pad_along - pad);
    }
  } else if (pad_mode == PAD_MODE_LOWER_VALID || pad_mode == PAD_MODE_UPPER_VALID) {
    MS_LOG(INFO) << "pad valid";
    for (size_t i = 0; i < dim - 2; ++i) {
      (void)padding_l->emplace_back(0);
      (void)padding_r->emplace_back(0);
    }
  } else {
    std::vector<int> pad;
    std::vector<int64_t> pad_me = common::AnfAlgo::GetNodeAttr<std::vector<int64_t>>(kernel_node, PAD_LIST);
    (void)std::transform(pad_me.begin(), pad_me.end(), std::back_inserter(pad),
                         [](const int64_t &value) { return static_cast<int>(value); });
    for (size_t i = 0; i < dim; i += 2) {
      (void)padding_l->emplace_back(pad[i]);
      (void)padding_r->emplace_back(pad[i + 1]);
    }
  }
}

bool MKLCpuKernelMod::BinaryBroadCast(std::vector<size_t> *src0_shape, std::vector<size_t> *src1_shape,
                                      std::vector<size_t> *dst_shape) const {
  MS_EXCEPTION_IF_NULL(src0_shape);
  MS_EXCEPTION_IF_NULL(src1_shape);
  MS_EXCEPTION_IF_NULL(dst_shape);
  bool need_swap = false;
  if (dst_shape->size() == 0) {
    (void)dst_shape->emplace_back(1);
    (void)src0_shape->emplace_back(1);
    (void)src1_shape->emplace_back(1);
  }
  MS_LOG(DEBUG) << "Binary broadcast in: src0: " << *src0_shape << " src1: " << *src1_shape << " dst: " << *dst_shape;
  if (src0_shape->size() != dst_shape->size()) {
    need_swap = true;
    for (size_t i = src0_shape->size(); i < dst_shape->size(); ++i) {
      (void)src0_shape->insert(src0_shape->begin(), 1);
    }
  } else if (src1_shape->size() != dst_shape->size()) {
    for (size_t i = src1_shape->size(); i < dst_shape->size(); ++i) {
      (void)src1_shape->insert(src1_shape->begin(), 1);
    }
  }
  if (src0_shape->size() == src1_shape->size()) {
    bool visit_src0 = false;
    bool visit_src1 = false;
    for (size_t i = 0; i < src0_shape->size(); ++i) {
      if (src0_shape->at(i) != src1_shape->at(i)) {
        if (src0_shape->at(i) == 1 && !visit_src1) {
          need_swap = true;
          visit_src0 = true;
        } else if (src1_shape->at(i) == 1 && !visit_src0) {
          need_swap = false;
          visit_src1 = true;
        } else {
          MS_LOG(EXCEPTION) << "Invalid broadcast! " << *src0_shape << " vs " << *src1_shape;
        }
      }
    }
  } else {
    MS_LOG(EXCEPTION) << "Invalid broadcast! src0: " << *src0_shape << " src1: " << *src1_shape
                      << " dst: " << *dst_shape;
  }
  MS_LOG(DEBUG) << "Binary broadcast out: src0: " << *src0_shape << " src1: " << *src1_shape << " dst: " << *dst_shape;
  return need_swap;
}

dnnl::memory::format_tag MKLCpuKernelMod::GetDefaultFormatTag(const dnnl::memory::dims &dims) const {
  static const std::vector<dnnl::memory::format_tag> tag_vec = {
    dnnl::memory::format_tag::a,      dnnl::memory::format_tag::ab,    dnnl::memory::format_tag::abc,
    dnnl::memory::format_tag::abcd,   dnnl::memory::format_tag::abcde, dnnl::memory::format_tag::abcdef,
    dnnl::memory::format_tag::abcdefg};
  size_t rank = dims.size();
  if (rank > tag_vec.size()) {
    MS_LOG(EXCEPTION) << "The kernel does not support construct " << rank << "-D tensor dnnl memory format_tag.";
  }
  return tag_vec[rank - 1];
}

dnnl::memory::desc MKLCpuKernelMod::GetDefaultMemDesc(const std::vector<size_t> &shape) const {
  dnnl::memory::dims dims;
  if (shape.empty()) {
    (void)dims.insert(dims.end(), 1);
  } else {
    (void)dims.insert(dims.end(), shape.begin(), shape.end());
  }
  dnnl::memory::format_tag mem_tag = GetDefaultFormatTag(dims);
  auto mem_desc = CreateDesc<dnnl::memory::desc>(dims, dnnl::memory::data_type::f32, mem_tag);
  return mem_desc;
}

void MKLCpuKernelMod::AddArgument(int arg_key, const dnnl::memory::desc &mem_desc, bool alloc) {
  if (alloc) {
    arguments_[arg_key] = dnnl::memory(mem_desc, engine_);
  } else {
    arguments_[arg_key] = dnnl::memory(mem_desc, engine_, nullptr);
  }
}

void MKLCpuKernelMod::SetArgumentHandle(int arg_key, void *ptr) {
  auto arg_iter = arguments_.find(arg_key);
  if (arg_iter != arguments_.end()) {
    MS_LOG(DEBUG) << "begin to invoke dnnl::memory::set_data_handle";
    arg_iter->second.set_data_handle(ptr);
    MS_LOG(DEBUG) << "end to invoke dnnl::memory::set_data_handle";
  }
}

void MKLCpuKernelMod::ExecutePrimitive() {
  MS_EXCEPTION_IF_NULL(primitive_);
#ifdef USE_MS_THREADPOOL_FOR_DNNL
  // add auto search
  const size_t MAX_POW = 6;
  const size_t AVG_COUNT = 5;
  const size_t DIFF = 2;
  size_t current_pow = parallel_search_info_.search_count / AVG_COUNT;
  int current_thread_nums = static_cast<int>(std::pow(2.0f, current_pow));
  auto mkl_pool = dynamic_cast<mkl_threadpool *>(mkl_threadpool_.get());
  if (current_pow < MAX_POW) {
    if (parallel_search_info_.search_count % AVG_COUNT == 0) {
      parallel_search_info_.tmp_sum_cost_time = 0;
    }
    double start_time = GetTime();
    mkl_pool->set_num_threads(current_thread_nums);
    MS_LOG(DEBUG) << "begin to invoke primitive::execute";
    primitive_->execute(stream_, arguments_);
    MS_LOG(DEBUG) << "end to invoke primitive::execute";
    double cost_time = GetTime() - start_time;
    parallel_search_info_.tmp_sum_cost_time += cost_time;
    parallel_search_info_.search_count++;
    if (parallel_search_info_.search_count % AVG_COUNT == 0) {
      if (parallel_search_info_.min_cost_time > parallel_search_info_.tmp_sum_cost_time) {
        parallel_search_info_.min_cost_time = parallel_search_info_.tmp_sum_cost_time;
        parallel_search_info_.best_pow = current_pow;
      } else if (current_pow - parallel_search_info_.best_pow >= DIFF) {
        parallel_search_info_.search_count = AVG_COUNT * MAX_POW;
      }
    }
  } else {
    int best_thread_nums = static_cast<int>(std::pow(2.0f, parallel_search_info_.best_pow));
    mkl_pool->set_num_threads(best_thread_nums);
    MS_LOG(DEBUG) << "begin to invoke primitive::execute";
    primitive_->execute(stream_, arguments_);
    MS_LOG(DEBUG) << "end to invoke primitive::execute";
  }
#else
  MS_LOG(DEBUG) << "begin to invoke primitive::execute";
  primitive_->execute(stream_, arguments_);
  MS_LOG(DEBUG) << "end to invoke primitive::execute";
#endif
  (void)stream_.wait();
}

void MKLCpuKernelMod::SetDataHandle(dnnl::memory mem, void *ptr) {
  MS_LOG(DEBUG) << "begin to invoke dnnl::memory::set_data_handle";
  mem.set_data_handle(ptr);
  MS_LOG(DEBUG) << "end to invoke dnnl::memory::set_data_handle";
}

void *MKLCpuKernelMod::GetDataHandle(const dnnl::memory &mem) const {
  MS_LOG(DEBUG) << "begin to invoke dnnl::memory::get_data_handle";
  auto ptr = mem.get_data_handle();
  MS_LOG(DEBUG) << "end to invoke dnnl::memory::get_data_handle";
  return ptr;
}

size_t MKLCpuKernelMod::GetSize(const dnnl::memory::desc &desc) const {
  MS_LOG(DEBUG) << "begin to invoke dnnl::memory::desc::get_size()";
  auto size = desc.get_size();
  MS_LOG(DEBUG) << "end to invoke dnnl::memory::desc::get_size()";
  return size;
}

void MKLCpuKernelMod::Reorder(dnnl::memory *src_mem, dnnl::memory *dst_mem) {
  MS_LOG(DEBUG) << "begin to invoke constructor of dnnl::reorder";
  auto desc = dnnl::reorder(*src_mem, *dst_mem);
  MS_LOG(DEBUG) << "end to invoke constructor of dnnl::reorder";
  MS_LOG(DEBUG) << "begin to invoke primitive::execute";
  desc.execute(stream_, *src_mem, *dst_mem);
  MS_LOG(DEBUG) << "begin to invoke primitive::execute";
}
}  // namespace kernel
}  // namespace mindspore
