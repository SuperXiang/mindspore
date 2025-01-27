/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include "ops/asinh.h"

namespace mindspore {
namespace ops {
namespace {
abstract::ShapePtr AsinhInferShape(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  auto prim_name = primitive->name();
  (void)CheckAndConvertUtils::CheckArgs<abstract::AbstractTensor>(prim_name, input_args, 0);
  auto x = input_args[kInputIndex0]->BuildShape();
  MS_EXCEPTION_IF_NULL(x);
  auto in_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[0]->BuildShape())[kShape];
  const int64_t MaxDim = 8;
  (void)CheckAndConvertUtils::CheckInteger("The dimension of Asinh input", SizeToLong(in_shape.size()), kLessThan,
                                           MaxDim, prim_name);
  auto shape_element = x->cast<abstract::ShapePtr>();
  MS_EXCEPTION_IF_NULL(shape_element);
  return shape_element;
}

TypePtr AsinhInferType(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  auto prim_name = primitive->name();
  const std::set<TypePtr> valid_types = {kFloat16, kFloat32, kFloat64, kComplex64, kComplex128};
  auto x_type = input_args[kInputIndex0]->BuildType();
  (void)CheckAndConvertUtils::CheckTensorTypeValid("x", x_type, valid_types, prim_name);
  return input_args[kInputIndex0]->BuildType();
}
}  // namespace

AbstractBasePtr AsinhInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                           const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto prim_name = primitive->name();
  const size_t InputNum = 1;
  (void)CheckAndConvertUtils::CheckInputArgs(input_args, kEqual, InputNum, prim_name);
  auto types = AsinhInferType(primitive, input_args);
  auto shapes = AsinhInferShape(primitive, input_args);
  return abstract::MakeAbstract(shapes, types);
}

REGISTER_PRIMITIVE_EVAL_IMPL(Asinh, prim::kPrimAsinh, AsinhInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
