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

#ifndef MINDSPORE_CORE_OPS_TILE_H_
#define MINDSPORE_CORE_OPS_TILE_H_
#include <set>
#include <string>
#include <vector>
#include <memory>

#include "ops/primitive_c.h"
#include "abstract/abstract_value.h"
#include "utils/check_convert_utils.h"

namespace mindspore {
namespace ops {
constexpr auto kNameTile = prim::kTile;
/// \brief Replicates a tensor with given multiples times. Refer to Python API @ref mindspore.ops.Tile for more details.
class MS_CORE_API Tile : public PrimitiveC {
 public:
  /// \brief Constructor.
  Tile() : PrimitiveC(kNameTile) { InitIOName({"x", "multiples"}, {"output"}); }
  explicit Tile(const std::string k_name) : PrimitiveC(k_name) { InitIOName({"x", "multiples"}, {"output"}); }
  /// \brief Destructor.
  ~Tile() = default;
  MS_DECLARE_PARENT(Tile, PrimitiveC);
  /// \brief Init.
  void Init() const {}
};
AbstractBasePtr TileInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                          const std::vector<AbstractBasePtr> &input_args);
}  // namespace ops
}  // namespace mindspore

#endif  // MINDSPORE_CORE_OPS_TILE_H_
