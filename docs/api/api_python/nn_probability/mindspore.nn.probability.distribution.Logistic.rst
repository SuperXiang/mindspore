mindspore.nn.probability.distribution.Logistic
================================================

.. py:class:: mindspore.nn.probability.distribution.Logistic(loc=None, scale=None, seed=None, dtype=mstype.float32, name='Logistic')

    Logistic分布（Logistic distribution）。
    连续随机分布，取值范围为 :math:`(0, \inf)` ，概率密度函数为

    .. math::
        f(x, a, b) = 1 / b \exp(\exp(-(x - a) / b) - x).

    其中 :math:`a, b` 为分别为Logistic分布的位置参数和比例参数。

    **参数：**

    - **loc** (int, float, list, numpy.ndarray, Tensor) - Logistic分布的位置。默认值：None。
    - **scale** (int, float, list, numpy.ndarray, Tensor) - Logistic分布的尺度。默认值：None。
    - **seed** (int) - 采样时使用的种子。如果为None，则使用全局种子。默认值：None。
    - **dtype** (mindspore.dtype) - 事件样例的类型。默认值：mindspore.float32。
    - **name** (str) - 分布的名称。默认值：'Logistic'。

    .. note:: 
        - `scale` 必须大于零。
        - `dtype` 必须是float，因为Logistic分布是连续的。

    **异常：**

    - **ValueError** - `scale` 中元素小于0。
    - **TypeError** - `dtype` 不是float的子类。

    .. py:method:: loc
        :property:

        返回分布位置。

        **返回：**

        Tensor, 分布的位置值。

    .. py:method:: scale
        :property:

        返回分布比例。

        **返回：**

        Tensor, 分布的比例值。

