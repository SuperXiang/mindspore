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

#ifndef MINDSPORE_CORE_OPS_EXPAND_DIMS_H_
#define MINDSPORE_CORE_OPS_EXPAND_DIMS_H_
#include <vector>
#include <memory>
#include "ops/primitive_c.h"
#include "abstract/abstract_value.h"
#include "utils/check_convert_utils.h"

namespace mindspore {
namespace ops {
constexpr auto kNameExpandDims = "ExpandDims";
/// \brief Adds an additional dimension to ‘input_x` at the given axis.
/// Refer to Python API @ref mindspore.ops.ExpandDims for more details.
class MS_CORE_API ExpandDims : public PrimitiveC {
 public:
  /// \brief Constructor.
  ExpandDims() : PrimitiveC(kNameExpandDims) { InitIOName({"x", "axis"}, {"output"}); }
  /// \brief Destructor.
  ~ExpandDims() = default;
  MS_DECLARE_PARENT(ExpandDims, PrimitiveC);
  /// \brief Init. Refer to the parameters of Python API @ref mindspore.ops.ExpandDims for the inputs.
  void Init() const {}
};
AbstractBasePtr ExpandDimsInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                const std::vector<AbstractBasePtr> &input_args);
using PrimExpandDims = std::shared_ptr<ExpandDims>;
}  // namespace ops
}  // namespace mindspore

#endif  // MINDSPORE_CORE_OPS_EXPAND_DIMS_H_
