/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CORE_OPS_SQUARE_H_
#define MINDSPORE_CORE_OPS_SQUARE_H_
#include <memory>
#include <vector>
#include <set>

#include "abstract/abstract_value.h"
#include "ops/primitive_c.h"
#include "utils/check_convert_utils.h"

namespace mindspore {
namespace ops {
/// \brief Returns square of a tensor element-wise. Refer to Python API @ref mindspore.ops.Square for more details.
class MS_CORE_API Square : public PrimitiveC {
 public:
  /// \brief Constructor.
  Square() : PrimitiveC(prim::kPrimSquare->name()) { InitIOName({"input_x"}, {"output"}); }
  /// \brief Destructor.
  ~Square() = default;
  MS_DECLARE_PARENT(Square, PrimitiveC);
  /// \brief Init.
  void Init() const {}
};
}  // namespace ops
}  // namespace mindspore

#endif  // MINDSPORE_CORE_OPS_SQUARE_H_
