#include "operators/vision/roi_align_op.h"
#include "core/workspace.h"
#include "utils/math_functions.h"
#include "utils/op_kernel.h"

namespace dragon {

template <class Context> template <typename T>
void ROIAlignOp<Context>::RunWithType() {
    kernel::ROIAlign<T, Context>(spatial_scale, 
                                pool_h, pool_w,
                                     &input(0),
                                     &input(1),
                                mask_h, mask_w,
                                    output(0));
}

template <class Context>
void ROIAlignOp<Context>::RunOnDevice() {
    vector<TIndex> dims({input(1).dim(0), input(0).dim(1), pool_h, pool_w});
    output(0)->Reshape(dims);

    mask_h = ws()->CreateTensor("_t_" + anchor() + "_roi_align_mask_h");
    mask_h->Reshape(dims);
    mask_w = ws()->CreateTensor("_t_" + anchor() + "_roi_align_mask_w");
    mask_w->Reshape(dims);

    if (input(0).template IsType<float>()) return RunWithType<float>();
    else LOG(FATAL) << "unsupported input types.";
}

DEPLOY_CPU(ROIAlign);
#ifdef WITH_CUDA
DEPLOY_CUDA(ROIAlign);
#endif
OPERATOR_SCHEMA(ROIAlign).NumInputs(2).NumOutputs(1);

template <class Context> template <typename T>
void ROIAlignGradientOp<Context>::RunWithType() {
    kernel::ROIAlignGrad<T, Context>(spatial_scale, 
                                    pool_h, pool_w,
                                        &input(-1),
                                         &input(1),
                                    mask_h, mask_w,
                                        output(0));
}

template <class Context>
void ROIAlignGradientOp<Context>::RunOnDevice() {
    output(0)->ReshapeLike(input(0));

    mask_h = ws()->GetTensor("_t_" + anchor() + "_roi_align_mask_h");
    mask_w = ws()->GetTensor("_t_" + anchor() + "_roi_align_mask_w");

    if (input(0).template IsType<float>()) return RunWithType<float>();
    else LOG(FATAL) << "unsupported input types.";
}

template <class Context>
void ROIAlignGradientOp<Context>::ShareBeforeRun() {
    Tensor* dX = ws()->GetBuffer();
    if (dX != nullptr) output(0)->Replace(*dX);
}

template <class Context>
void ROIAlignGradientOp<Context>::ClearAfterRun() {
    Tensor* dY = &input(-1);
    ws()->ReleaseBuffer(dY);
    ws()->ReleaseBuffer(mask_h);
    ws()->ReleaseBuffer(mask_w);
}

DEPLOY_CPU(ROIAlignGradient);
#ifdef WITH_CUDA
DEPLOY_CUDA(ROIAlignGradient);
#endif
OPERATOR_SCHEMA(ROIAlignGradient).NumInputs(3).NumOutputs(1);

class GetROIAlignGradient final : public GradientMakerBase {
public:
    GRADIENT_MAKER_CTOR(GetROIAlignGradient);
    vector<OperatorDef> MakeDefs() override {
        return SingleDef(def.type() + "Gradient", "",
            vector<string> {I(0), I(1), GO(0)},
            vector<string> {GI(0)});
    }
};
REGISTER_GRADIENT(ROIAlign, GetROIAlignGradient);

}    // namespace dragon