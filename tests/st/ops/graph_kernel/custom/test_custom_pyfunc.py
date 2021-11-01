# Copyright 2021 Huawei Technologies Co., Ltd
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
import numpy as np
import pytest

import mindspore as ms
import mindspore.nn as nn
import mindspore.context as context
from mindspore import Tensor
from mindspore.ops import operations as P
from mindspore.ops.op_info_register import DataType
from mindspore.ops.operations.custom_ops import Custom, CustomRegOp, custom_op_info_register

single_output_op_info = CustomRegOp() \
    .input(0, "x1") \
    .input(1, "x2") \
    .output(0, "y") \
    .dtype_format(DataType.F16_Default, DataType.F16_Default, DataType.F16_Default) \
    .dtype_format(DataType.F32_Default, DataType.F32_Default, DataType.F32_Default) \
    .dtype_format(DataType.F64_Default, DataType.F64_Default, DataType.F64_Default) \
    .dtype_format(DataType.I32_Default, DataType.I32_Default, DataType.I32_Default) \
    .dtype_format(DataType.I64_Default, DataType.I64_Default, DataType.I64_Default) \
    .get_op_info()


@custom_op_info_register(single_output_op_info)
def func_single_output(x1, x2):
    return x1 - x2


multi_output_op_info = CustomRegOp() \
    .input(0, "x1") \
    .input(1, "x2") \
    .output(0, "y1") \
    .output(1, "y2") \
    .dtype_format(DataType.F32_Default, DataType.F32_Default, DataType.F32_Default, DataType.F32_Default) \
    .get_op_info()


@custom_op_info_register(multi_output_op_info)
def func_multi_output(x1, x2):
    return (x1 + x2), (x1 - x2)


no_output_op_info = CustomRegOp() \
    .input(0, "x1") \
    .input(1, "x2") \
    .dtype_format(DataType.F32_Default, DataType.F32_Default)\
    .get_op_info()

output = 0


@custom_op_info_register(no_output_op_info)
def func_no_output(x1, x2):
    global output
    output = x1 + x2


class PyFuncNet(nn.Cell):
    def __init__(self, fn, out_shapes, out_types,):
        super().__init__()
        self.func = Custom(fn, out_shapes, out_types, "pyfunc")
        self.relu = P.ReLU()

    def construct(self, x1, x2):
        x = self.func(x1, x2)
        return self.relu(x[0])


def func_with_dtype(ms_dtype, np_dtype):
    shape = (40, 40)
    np.random.seed(42)
    x1 = np.random.randint(-5, 5, size=shape).astype(np_dtype)
    x2 = np.random.randint(-5, 5, size=shape).astype(np_dtype)

    expect = func_single_output(x1, x2)
    expect = P.ReLU()(Tensor(expect))

    net = PyFuncNet(func_single_output, (shape,), (ms_dtype,))
    x = net(Tensor(x1), Tensor(x2))
    assert np.allclose(x.asnumpy(), expect.asnumpy())


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.env_onecard
def test_pyfunc_single_output():
    """
    Feature: test case for Custom op with func_type="pyfunc"
    Description: the net runs on GPU while custom pyfunc operator on CPU; GRAPH_MODE; single output
    Expectation: the result match with numpy result
    """
    context.set_context(mode=context.GRAPH_MODE, device_target='GPU')
    func_with_dtype(ms.float16, np.float16)
    func_with_dtype(ms.float32, np.float32)
    func_with_dtype(ms.float64, np.float64)
    func_with_dtype(ms.int32, np.int32)
    func_with_dtype(ms.int64, np.int64)


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.env_onecard
def test_pyfunc_multi_output():
    """
    Feature: test case for Custom op with func_type="pyfunc"
    Description: the net runs on GPU while custom pyfunc operator on CPU; GRAPH_MODE; multiple output
    Expectation: the result match with numpy result
    """
    context.set_context(mode=context.GRAPH_MODE, device_target='GPU')
    shape = (40, 40)
    dtype = ms.float32

    np.random.seed(42)
    x1 = np.random.randint(-5, 5, size=shape).astype(np.float32)
    x2 = np.random.randint(-5, 5, size=shape).astype(np.float32)
    expect, _ = func_multi_output(x1, x2)
    expect = P.ReLU()(Tensor(expect))

    net = PyFuncNet(func_multi_output, (shape, shape), (dtype, dtype))
    x = net(Tensor(x1), Tensor(x2))

    assert np.allclose(x.asnumpy(), expect.asnumpy())


class PyFuncGraph(nn.Cell):
    def __init__(self, fn, out_shapes, out_types):
        super().__init__()
        self.func = Custom(fn, out_shapes, out_types, "pyfunc")

    def construct(self, x1, x2):
        return self.func(x1, x2)


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.env_onecard
def test_pyfunc_no_output():
    """
    Feature: test case for Custom op with func_type="pyfunc"
    Description:  the net runs on GPU while custom pyfunc operator on CPU; GRAPH_MODE; no output
    Expectation: the result match with numpy result
    """
    context.set_context(mode=context.GRAPH_MODE, device_target='GPU')
    shape = (40, 40)

    np.random.seed(42)
    x1 = np.random.randint(-5, 5, size=shape).astype(np.float32)
    x2 = np.random.randint(-5, 5, size=shape).astype(np.float32)
    func_no_output(x1, x2)
    global output
    expect = output

    net = PyFuncGraph(func_no_output, (), ())
    net(Tensor(x1), Tensor(x2))
    net_output = output

    assert np.allclose(net_output, expect)


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.env_onecard
def test_pyfunc_scalar():
    """
    Feature: test case for Custom op with func_type="pyfunc"
    Description:  the net runs on GPU while custom pyfunc operator on CPU; GRAPH_MODE; scalar output
    Expectation: the result match with numpy result
    """
    context.set_context(mode=context.GRAPH_MODE, device_target='GPU')
    shape = ()
    ms_dtype = ms.int32

    x1 = int(10)
    x2 = int(5)
    expect = func_single_output(x1, x2)

    net = PyFuncGraph(func_single_output, shape, ms_dtype)
    x = net(Tensor(x1), Tensor(x2))
    assert np.allclose(x.asnumpy(), expect)