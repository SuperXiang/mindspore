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
"""st for line_search."""

import pytest

import numpy as onp
from scipy.optimize.linesearch import line_search_wolfe2 as osp_line_search

import mindspore.numpy as mnp
from mindspore import context
from mindspore.scipy.optimize.line_search import line_search as msp_line_search

from .utils import match_array

context.set_context(mode=context.GRAPH_MODE)


def _scalar_func_1(np):
    def f(x):
        return -x - x ** 3 + x ** 4

    def fprime(x):
        return -1 - 3 * x ** 2 + 4 * x ** 3

    return f, fprime


def _scalar_func_2(np):
    def f(x):
        return np.exp(-4 * x) + x ** 2

    def fprime(x):
        return -4 * np.exp(-4 * x) + 2 * x

    return f, fprime


def _scalar_func_3(np):
    def f(x):
        return -np.sin(10 * x)

    def fprime(x):
        return -10 * np.cos(10 * x)

    return f, fprime


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('maxiter, func, x, p', [(10, _scalar_func_1, 0., 1.),
                                                 (10, _scalar_func_2, 0., 1.),
                                                 (10, _scalar_func_3, 0., 1.)])
def test_scalar_search(maxiter, func, x, p):
    """
    Feature: ALL TO ALL
    Description: test cases for 1-d function
    Expectation: the result match scipy
    """
    osp_f, osp_fp = func(onp)
    osp_x, osp_p = onp.array(x), onp.array(p)
    osp_res = osp_line_search(osp_f, osp_fp, osp_x, osp_p, maxiter=maxiter)

    msp_f, _ = func(mnp)
    msp_x, msp_p = mnp.array(x), mnp.array(p)
    msp_res = msp_line_search(msp_f, msp_x, msp_p, maxiter=maxiter)

    match_array(msp_res.a_k, osp_res[0], error=5)
    match_array(msp_res.f_k, osp_res[3], error=5)


def _line_func_1(np, *args):
    def f(x):
        return np.dot(x, x)

    def fprime(x):
        return 2 * x

    return f, fprime


def _line_func_2(np, *args):
    def f(x):
        A = args[0]
        return np.dot(x, np.dot(A, x)) + 1

    def fprime(x):
        A = args[0]
        return np.dot(A + A.T, x)

    return f, fprime


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('maxiter, func, x, p',
                         [(10, _line_func_1, [1.13689136, 0.09772497, 0.58295368, -0.39944903, 0.37005589],
                           [-1.30652685, 1.65813068, -0.11816405, -0.6801782, 0.66638308]),
                          (10, _line_func_1, [-0.52118931, -1.84306955, -0.477974, -0.47965581, 0.6203583],
                           [0.69845715, 0.00377089, 0.93184837, 0.33996498, -0.01568211]),
                          (10, _line_func_2, [0.15634897, 1.23029068, 1.20237985, -0.38732682, -0.30230275],
                           [-1.04855297, -1.42001794, -1.70627019, 1.9507754, -0.50965218]),
                          (10, _line_func_2, [0.42833187, 0.06651722, 0.3024719, -0.63432209, -0.36274117],
                           [-0.67246045, -0.35955316, -0.81314628, -1.7262826, 0.17742614])])
def test_line_search(maxiter, func, x, p):
    """
    Feature: ALL TO ALL
    Description: test cases for n-d function in PYNATIVE mode
    Expectation: the result match scipy
    """
    A = [[1.76405235, 0.40015721, 0.97873798, 2.2408932, 1.86755799],
         [-0.97727788, 0.95008842, -0.15135721, -0.10321885, 0.4105985],
         [0.14404357, 1.45427351, 0.76103773, 0.12167502, 0.44386323],
         [0.33367433, 1.49407907, -0.20515826, 0.3130677, -0.85409574],
         [-2.55298982, 0.6536186, 0.8644362, -0.74216502, 2.26975462]]

    osp_x, osp_p, osp_A = onp.array(x), onp.array(p), onp.array(A)
    osp_f, osp_fp = func(onp, osp_A)
    osp_res = osp_line_search(osp_f, osp_fp, osp_x, osp_p, maxiter=maxiter)

    msp_x, msp_p, msp_A = mnp.array(x), mnp.array(p), mnp.array(A)
    msp_f, _ = func(mnp, msp_A)
    msp_res = msp_line_search(msp_f, msp_x, msp_p, maxiter=maxiter)

    match_array(msp_res.a_k, osp_res[0], error=5)
    match_array(msp_res.f_k, osp_res[3], error=5)


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
@pytest.mark.parametrize('maxiter, func, x, p',
                         [(10, _line_func_1, [1.13689136, 0.09772497, 0.58295368, -0.39944903, 0.37005589],
                           [-1.30652685, 1.65813068, -0.11816405, -0.6801782, 0.66638308])])
def test_line_search_graph(maxiter, func, x, p):
    """
    Feature: ALL TO ALL
    Description: test cases for n-d function in GRAPH mode
    Expectation: the result match scipy
    """
    context.set_context(mode=context.GRAPH_MODE)
    A = [[1.76405235, 0.40015721, 0.97873798, 2.2408932, 1.86755799],
         [-0.97727788, 0.95008842, -0.15135721, -0.10321885, 0.4105985],
         [0.14404357, 1.45427351, 0.76103773, 0.12167502, 0.44386323],
         [0.33367433, 1.49407907, -0.20515826, 0.3130677, -0.85409574],
         [-2.55298982, 0.6536186, 0.8644362, -0.74216502, 2.26975462]]

    osp_x, osp_p, osp_A = onp.array(x), onp.array(p), onp.array(A)
    osp_f, osp_fp = func(onp, osp_A)
    osp_res = osp_line_search(osp_f, osp_fp, osp_x, osp_p, maxiter=maxiter)

    msp_x, msp_p, msp_A = mnp.array(x), mnp.array(p), mnp.array(A)
    msp_f, _ = func(mnp, msp_A)
    msp_res = msp_line_search(msp_f, msp_x, msp_p, maxiter=maxiter)

    match_array(msp_res.a_k, osp_res[0], error=5)
    match_array(msp_res.f_k, osp_res[3], error=5)