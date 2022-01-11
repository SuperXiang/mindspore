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
"""Controlling dump behavior."""
from warnings import warn

import mindspore.context as context
from mindspore._c_expression import security


def set_dump(target, enabled=True):
    """
    Enable or disable dump for the target and its contents.

    Target should be an instance of Cell or Primitive. The default enabled
    status for a cell or primitive is False. Please note that this API takes
    effect only when the dump_mode field in dump config file is 2. See the
    `dump document <https://mindspore.cn/docs/programming_guide/zh-CN/r1.6/dump_in_graph_mode.html>`_
    for details.

    .. warning::
        This is an experimental prototype that is subject to change or deletion.

    Note:
        1. This API is only effective for GRAPH_MODE with Ascend backend.
        2. When target is a cell and enabled is True, this API will the enable
           dump for the primitive operator members of the cell instance and
           its child cell instances recursively. If an operator is not a
           member of the cell instance, the dump flag will not be set for
           this operator (e.g. functional operators used directly in
           construct method). To make this API effective, please use
           self.some_op = SomeOp() in your cell's __init__ method.
        3. After using set_dump(cell, True), operators in forward computation
           of the cell will be dumped. Most backward computation (computation
           generated by the grad operations) will not be dumped by design.
           However, due to the graph optimization, a few backward computation
           data will still be dumped. You can ignore the backward computation
           data which contains "Gradients" in their filenames.
        4. This API is not designed to use in the middle of training process.
           If you call this API in the middle of training, it only takes effect
           for the later compiled graphs. If there is no new graph compilation,
           you will see no effect.
        5. For operator SparseSoftmaxCrossEntropyWithLogits, the forward
           computation and backward computation use the same set of
           operators. So you can only see dump data from backward computation.
           Please note that operator SoftmaxCrossEntropyWithLogits will also use
           the above operator internally when initialized with sparse=True and
           reduction="mean".

    Args:
        target (Union[Cell, Primitive]): The Cell instance or Primitive instance
            to which the dump flag is set.
        enabled (bool): True means enable dump, False means disable dump.
            Default: True.

    Supported Platforms:
        ``Ascend``

    Examples:
        >>> # Please set the dump config file and environment variable before
        >>> # running this example to actually get the dump data.
        >>> # See the document of this API for details.
        >>> import numpy as np
        >>>
        >>> import mindspore.nn as nn
        >>> import mindspore.context as context
        >>> from mindspore import Tensor, set_dump
        >>>
        >>> context.set_context(device_target="Ascend", mode=context.GRAPH_MODE)
        >>>
        >>> class MyNet(nn.Cell):
        ...     def __init__(self):
        ...         super().__init__()
        ...         self.conv1 = nn.Conv2d(5, 6, 5, pad_mode='valid')
        ...         self.relu1 = nn.ReLU()
        ...
        ...     def construct(self, x):
        ...         x = self.conv1(x)
        ...         x = self.relu1(x)
        ...         return x
        >>>
        >>> net = MyNet()
        >>> set_dump(net.conv1)
        >>> input_tensor = Tensor(np.ones([1, 5, 10, 10], dtype=np.float32))
        >>> net(input_tensor)
    """
    if security.enable_security():
        raise ValueError('The set_dump API is not supported, please recompile '
                         'source without "-s on".')

    import mindspore.nn as nn  # avoid circular import
    from mindspore.ops import Primitive
    if not isinstance(target, nn.Cell) and not isinstance(target, Primitive):
        raise ValueError(f"The \"target\" parameter must be an instance of "
                         f"Cell or Primitive, "
                         f"but got an instance of {type(target)}.")

    if not isinstance(enabled, bool):
        raise ValueError("The \"enabled\" parameter must be bool.")

    # Checking for device target and mode.
    current_target = context.get_context("device_target")
    if current_target != "Ascend":
        # We will not return here in case user changed device_target later.
        warn("Current device_target is {}, which is not supported by set_dump. "
             "Only Ascend device target is supported currently. "
             "If you have Ascend device, consider set device_target to Ascend "
             "before calling set_dump.".format(current_target))

    current_mode = context.get_context("mode")
    if current_mode != context.GRAPH_MODE:
        # We will not return here in case user changed mode later.
        warn(
            "Current mode is PYNATIVE_MODE, which is not supported by set_dump. "
            "Only GRAPH_MODE is supported currently. "
            "Consider set mode to GRAPH_MODE "
            "before calling set_dump.")

    # The actual set dump logic.
    mode = "true" if enabled else "false"
    if isinstance(target, nn.Cell):
        primitives = getattr(target, "_primitives", {})
        for value in primitives.values():
            if value:
                value.add_prim_attr("dump", mode)
        for cell in target.cells():
            set_dump(cell, enabled)
        return

    if isinstance(target, Primitive):
        target.add_prim_attr("dump", mode)
        return
