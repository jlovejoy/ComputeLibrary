/*
 * Copyright (c) 2016, 2017 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "arm_compute/core/CL/ICLKernel.h"

#include "arm_compute/core/CL/ICLTensor.h"
#include "arm_compute/core/Error.h"
#include "arm_compute/core/Helpers.h"
#include "arm_compute/core/TensorInfo.h"
#include "arm_compute/core/Utils.h"
#include "arm_compute/core/Validate.h"
#include "arm_compute/core/Window.h"

#include <cstddef>

using namespace arm_compute;

void arm_compute::enqueue(cl::CommandQueue &queue, ICLKernel &kernel, const Window &window, const cl::NDRange &lws_hint)
{
    if(kernel.kernel()() == nullptr)
    {
        return;
    }

    ARM_COMPUTE_ERROR_ON((0 == (window.x().end() - window.x().start())) || (0 == (window.y().end() - window.y().start())));

    cl::NDRange gws((window.x().end() - window.x().start()) / window.x().step(),
                    (window.y().end() - window.y().start()) / window.y().step(),
                    (window.z().end() - window.z().start()) / window.z().step());

    cl::NDRange lws = cl::NullRange;

    if((lws_hint[0] <= gws[0]) && (lws_hint[1] <= gws[1]) && (lws_hint[2] <= gws[2]))
    {
        lws = lws_hint;
    }

    queue.enqueueNDRangeKernel(kernel.kernel(), cl::NullRange, gws, lws);
}

ICLKernel::ICLKernel()
    : _kernel(nullptr), _lws_hint(cl::Range_128_1)
{
}

cl::Kernel &ICLKernel::kernel()
{
    return _kernel;
}

template <unsigned int dimension_size>
unsigned int           ICLKernel::num_arguments_per_tensor() const
{
    return 2 + 2 * dimension_size;
}

template <unsigned int dimension_size>
void ICLKernel::add_tensor_argument(unsigned &idx, const ICLTensor *tensor, const Window &window)
{
    ARM_COMPUTE_ERROR_ON(tensor == nullptr);
    ARM_COMPUTE_ERROR_ON_WINDOW_DIMENSIONS_GTE(window, tensor->info()->num_dimensions());

    const TensorInfo *info    = tensor->info();
    const Strides    &strides = info->strides_in_bytes();

    // Calculate offset to the start of the window
    unsigned int offset_first_element = info->offset_first_element_in_bytes();

    for(unsigned int n = 0; n < info->num_dimensions(); ++n)
    {
        offset_first_element += window[n].start() * strides[n];
    }

    unsigned int idx_start = idx;
    _kernel.setArg(idx++, tensor->cl_buffer());

    for(unsigned int dimension = 0; dimension < dimension_size; dimension++)
    {
        _kernel.setArg<cl_uint>(idx++, strides[dimension]);
        _kernel.setArg<cl_uint>(idx++, strides[dimension] * window[dimension].step());
    }

    _kernel.setArg<cl_uint>(idx++, offset_first_element);

    ARM_COMPUTE_ERROR_ON_MSG(idx_start + num_arguments_per_tensor<dimension_size>() != idx,
                             "add_%dD_tensor_argument() is supposed to add exactly %d arguments to the kernel", dimension_size, num_arguments_per_tensor<dimension_size>());
    ARM_COMPUTE_UNUSED(idx_start);
}

void ICLKernel::add_1D_tensor_argument(unsigned int &idx, const ICLTensor *tensor, const Window &window)
{
    add_tensor_argument<1>(idx, tensor, window);
}

void ICLKernel::add_2D_tensor_argument(unsigned int &idx, const ICLTensor *tensor, const Window &window)
{
    add_tensor_argument<2>(idx, tensor, window);
}

void ICLKernel::add_3D_tensor_argument(unsigned int &idx, const ICLTensor *tensor, const Window &window)
{
    add_tensor_argument<3>(idx, tensor, window);
}

unsigned int ICLKernel::num_arguments_per_1D_tensor() const
{
    return num_arguments_per_tensor<1>();
}

unsigned int ICLKernel::num_arguments_per_2D_tensor() const
{
    return num_arguments_per_tensor<2>();
}

unsigned int ICLKernel::num_arguments_per_3D_tensor() const
{
    return num_arguments_per_tensor<3>();
}
