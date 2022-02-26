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
#ifndef MINDSPORE_CCSRC_BACKEND_OPTIMIZER_GPU_APPLY_MOMENTUM_SCALE_FUSION_H_
#define MINDSPORE_CCSRC_BACKEND_OPTIMIZER_GPU_APPLY_MOMENTUM_SCALE_FUSION_H_

#include <memory>
#include "backend/common/optimizer/optimizer.h"
#include "backend/common/session/anf_runtime_algorithm.h"
#include "include/common/utils/anfalgo.h"

namespace mindspore {
namespace opt {
class ApplyMomentumScaleFusion : public PatternProcessPass {
 public:
  explicit ApplyMomentumScaleFusion(bool multigraph = true) : PatternProcessPass("momentum_scale_fusion", multigraph) {
    scale_ = std::make_shared<CondVar>(IsScalar);
    variable_ = std::make_shared<Var>();
    accumulation_ = std::make_shared<Var>();
    learning_rate_ = std::make_shared<Var>();
    gradient_ = std::make_shared<Var>();
    momentum_ = std::make_shared<Var>();
    monad_state_ = std::make_shared<Var>();
  }
  ~ApplyMomentumScaleFusion() override = default;
  const BaseRef DefinePattern() const override;
  const AnfNodePtr Process(const FuncGraphPtr &, const AnfNodePtr &, const EquivPtr &) const override;

 private:
  static bool IsScalar(const BaseRef &n);

  VarPtr scale_;
  VarPtr variable_;
  VarPtr accumulation_;
  VarPtr learning_rate_;
  VarPtr gradient_;
  VarPtr momentum_;
  VarPtr monad_state_;
};
}  // namespace opt
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_BACKEND_OPTIMIZER_GPU_APPLY_MOMENTUM_SCALE_FUSION_H_
