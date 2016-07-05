#ifndef APPLYALGORITHMFILTER_HXX
#define APPLYALGORITHMFILTER_HXX

#include "itkObjectFactory.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"

namespace itk {

template<typename TI, typename TO, typename TC>
ApplyAlgorithmFilter<TI, TO, TC>::ApplyAlgorithmFilter() {
    //std::cout <<  __PRETTY_FUNCTION__ << endl;
}

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::SetAlgorithm(const std::shared_ptr<Algorithm> &a) {
    //std::cout <<  __PRETTY_FUNCTION__ << endl;
    m_algorithm = a;
    // Inputs go: Data 0, Data 1, ..., Mask, Const 0, Const 1, ...
    // Only the data inputs are required, the others are optional
    this->SetNumberOfRequiredInputs(a->numInputs());
    // Outputs go: AllResiduals, Residual, Iterations, Parameter 0, Parameter 1, ...
    size_t totalOutputs = StartOutputs + m_algorithm->numOutputs();
    this->SetNumberOfRequiredOutputs(totalOutputs);
    for (size_t i = 0; i < totalOutputs; i++) {
        this->SetNthOutput(i, this->MakeOutput(i));
    }
}

template<typename TI, typename TO, typename TC>
auto ApplyAlgorithmFilter<TI, TO, TC>::GetAlgorithm() const -> std::shared_ptr<const Algorithm>{ return m_algorithm; }

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::SetPoolsize(const size_t n) { m_poolsize = n; }

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::SetSubregion(const TRegion &sr) { m_subregion = sr; m_hasSubregion = true; }

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::SetVerbose(const bool v) { m_verbose = v; }

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::SetOutputAllResiduals(const bool r) { m_allResiduals = r; }

template<typename TI, typename TO, typename TC>
RealTimeClock::TimeStampType ApplyAlgorithmFilter<TI, TO, TC>::GetTotalTime() const { return m_elapsedTime; }

template<typename TI, typename TO, typename TC>
RealTimeClock::TimeStampType ApplyAlgorithmFilter<TI, TO, TC>::GetMeanTime() const { return m_elapsedTime / m_unmaskedVoxels; }

template<typename TI, typename TO, typename TC>
SizeValueType ApplyAlgorithmFilter<TI, TO, TC>::GetEvaluations() const { return m_unmaskedVoxels; }

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::SetInput(const size_t i, const TInputImage *image) {
    if (i < m_algorithm->numInputs()) {
        this->SetNthInput(i, const_cast<TInputImage*>(image));
    } else {
        itkExceptionMacro("Requested input " << i << " does not exist (" << m_algorithm->numInputs() << " inputs)");
    }
}

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::SetConst(const size_t i, const TConstImage *image) {
    if (i < m_algorithm->numConsts()) {
        this->SetNthInput(m_algorithm->numInputs() + 1 + i, const_cast<TConstImage*>(image));
    } else {
        itkExceptionMacro("Requested const input " << i << " does not exist (" << m_algorithm->numConsts() << " const inputs)");
    }
}

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::SetMask(const TConstImage *image) {
    this->SetNthInput(m_algorithm->numInputs(), const_cast<TConstImage*>(image));
}

template<typename TI, typename TO, typename TC>
auto ApplyAlgorithmFilter<TI, TO, TC>::GetInput(const size_t i) const -> typename TInputImage::ConstPointer {
    if (i < m_algorithm->numInputs()) {
        return static_cast<const TInputImage *> (this->ProcessObject::GetInput(i));
    } else {
        itkExceptionMacro("Requested input " << i << " does not exist (" << m_algorithm->numInputs() << " inputs)");
    }
}

template<typename TI, typename TO, typename TC>
auto ApplyAlgorithmFilter<TI, TO, TC>::GetConst(const size_t i) const -> typename TConstImage::ConstPointer {
    if (i < m_algorithm->numConsts()) {
        size_t index = m_algorithm->numInputs() + 1 + i;
        return static_cast<const TConstImage *> (this->ProcessObject::GetInput(index));
    } else {
        itkExceptionMacro("Requested const input " << i << " does not exist (" << m_algorithm->numConsts() << " const inputs)");
    }
}

template<typename TI, typename TO, typename TC>
auto ApplyAlgorithmFilter<TI, TO, TC>::GetMask() const -> typename TConstImage::ConstPointer {
    return static_cast<const TConstImage *>(this->ProcessObject::GetInput(m_algorithm->numInputs()));
}

template<typename TI, typename TO, typename TC>
DataObject::Pointer ApplyAlgorithmFilter<TI, TO, TC>::MakeOutput(unsigned int idx) {
    DataObject::Pointer output;
    if (idx == AllResidualsOutput) {
        auto img = TInputImage::New();
        output = img;
    } else if (idx == ResidualOutput) {
        auto img = TConstImage::New();
        output = img;
    } else if (idx == IterationsOutput) {
        auto img = TIterationsImage::New();
        output = img;
    } else if (idx < (m_algorithm->numOutputs() + StartOutputs)) {
        output = (TOutputImage::New()).GetPointer();
    } else {
        itkExceptionMacro("Attempted to create output " << idx << ", this algorithm only has " << m_algorithm->numOutputs() << "+" << StartOutputs << " outputs.");
    }
    return output.GetPointer();
}

template<typename TI, typename TO, typename TC>
auto ApplyAlgorithmFilter<TI, TO, TC>::GetOutput(const size_t i) -> TOutputImage *{
    if (i < m_algorithm->numOutputs()) {
        return dynamic_cast<TConstImage *>(this->ProcessObject::GetOutput(i+StartOutputs));
    } else {
        itkExceptionMacro("Requested output " << std::to_string(i) << " is past maximum (" << std::to_string(m_algorithm->numOutputs()) << ")");
    }
}

template<typename TI, typename TO, typename TC>
auto ApplyAlgorithmFilter<TI, TO, TC>::GetAllResidualsOutput() -> TInputImage *{
    return dynamic_cast<TInputImage *>(this->ProcessObject::GetOutput(AllResidualsOutput));
}

template<typename TI, typename TO, typename TC>
auto ApplyAlgorithmFilter<TI, TO, TC>::GetResidualOutput() -> TConstImage *{
    return dynamic_cast<TConstImage *>(this->ProcessObject::GetOutput(ResidualOutput));
}

template<typename TI, typename TO, typename TC>
auto ApplyAlgorithmFilter<TI, TO, TC>::GetIterationsOutput() -> TIterationsImage *{
    return dynamic_cast<TIterationsImage *>(this->ProcessObject::GetOutput(IterationsOutput));
}

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::GenerateOutputInformation() {
    Superclass::GenerateOutputInformation();
    size_t size = 0;
    for (size_t i = 0; i < m_algorithm->numInputs(); i++) {
        size += this->GetInput(i)->GetNumberOfComponentsPerPixel();
    }
    if (m_algorithm->dataSize() != size) {
        itkExceptionMacro("Sequence size (" << m_algorithm->dataSize() << ") does not match input size (" << size << ")");
    }
    if (size == 0) {
        itkExceptionMacro("Total input size cannot be 0");
    }

    auto input     = this->GetInput(0);
    auto region    = input->GetLargestPossibleRegion();
    auto spacing   = input->GetSpacing();
    auto origin    = input->GetOrigin();
    auto direction = input->GetDirection();
    if (m_verbose) std::cout << "Allocating output memory" << std::endl;
    for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
        auto op = this->GetOutput(i);
        op->SetRegions(region);
        op->SetSpacing(spacing);
        op->SetOrigin(origin);
        op->SetDirection(direction);
        op->Allocate(true);
    }
    if (m_allResiduals) {
        if (m_verbose) std::cout << "Allocating residuals memory" << std::endl;
        auto r = this->GetAllResidualsOutput();
        r->SetRegions(region);
        r->SetSpacing(spacing);
        r->SetOrigin(origin);
        r->SetDirection(direction);
        r->SetNumberOfComponentsPerPixel(size);
        r->Allocate(true);
    }
    auto r = this->GetResidualOutput();
    r->SetRegions(region);
    r->SetSpacing(spacing);
    r->SetOrigin(origin);
    r->SetDirection(direction);
    r->Allocate(true);
    auto i = this->GetIterationsOutput();
    i->SetRegions(region);
    i->SetSpacing(spacing);
    i->SetOrigin(origin);
    i->SetDirection(direction);
    i->Allocate(true);
}

template<typename TI, typename TO, typename TC>
void ApplyAlgorithmFilter<TI, TO, TC>::GenerateData() {
    const unsigned int LastDim = TInputImage::ImageDimension - 1;
    TRegion region = this->GetInput(0)->GetLargestPossibleRegion();
    if (m_hasSubregion) {
        if (region.IsInside(m_subregion)) {
            region = m_subregion;
        } else {
            itkExceptionMacro("Specified subregion is not entirely inside image.");
        }
    }
    ImageRegionConstIterator<TConstImage> maskIter;
    const auto mask = this->GetMask();
    if (mask) {
        if (m_verbose) std::cout << "Counting voxels in mask..." << std::endl;
        m_unmaskedVoxels = 0;
        maskIter = ImageRegionConstIterator<TConstImage>(mask, region);
        maskIter.GoToBegin();
        while (!maskIter.IsAtEnd()) {
            if (maskIter.Get())
                ++m_unmaskedVoxels;
            ++maskIter;
        }
        maskIter.GoToBegin(); // Reset
        if (m_verbose) std::cout << "Found " << m_unmaskedVoxels << " unmasked voxels." << std::endl;
    } else {
        m_unmaskedVoxels = region.GetNumberOfPixels();
    }
    ProgressReporter progress(this, 0, m_unmaskedVoxels, 10);

    std::vector<ImageRegionConstIterator<TInputImage>> dataIters(m_algorithm->numInputs());
    for (size_t i = 0; i < m_algorithm->numInputs(); i++) {
        dataIters[i] = ImageRegionConstIterator<TInputImage>(this->GetInput(i), region);
    }

    std::vector<ImageRegionConstIterator<TConstImage>> constIters(m_algorithm->numConsts());
    for (size_t i = 0; i < m_algorithm->numConsts(); i++) {
        typename TConstImage::ConstPointer c = this->GetConst(i);
        if (c) {
            constIters[i] = ImageRegionConstIterator<TConstImage>(c, region);
        }
    }
    std::vector<ImageRegionIterator<TConstImage>> outputIters(m_algorithm->numOutputs());
    for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
        outputIters[i] = ImageRegionIterator<TConstImage>(this->GetOutput(i), region);
    }
    ImageRegionIterator<TInputImage> allResidualsIter;
    if (m_allResiduals) {
        allResidualsIter = ImageRegionIterator<TInputImage>(this->GetAllResidualsOutput(), region);
    }
    ImageRegionIterator<TConstImage> residualIter(this->GetResidualOutput(), region);
    ImageRegionIterator<TIterationsImage> iterationsIter(this->GetIterationsOutput(), region);
    if (m_verbose) std::cout << "Starting processing" << std::endl;
    QI::ThreadPool threadPool(m_poolsize);
    TimeProbe clock;
    clock.Start();
    while(!dataIters[0].IsAtEnd()) {
        if (!mask || maskIter.Get()) {
            auto task = [=] {
                std::vector<TInput> inputs(m_algorithm->numInputs());
                std::vector<TOutput> outputs(m_algorithm->numOutputs());
                std::vector<TConst> constants = m_algorithm->defaultConsts();
                for (size_t i = 0; i < constIters.size(); i++) {
                    if (this->GetConst(i)) {
                        constants[i] = constIters[i].Get();
                    }
                }
                TConst residual;
                TInput resids;
                TIterations iterations{0};

                for (size_t i = 0; i < m_algorithm->numInputs(); i++) {
                    inputs[i] = dataIters[i].Get();
                }
                m_algorithm->apply(inputs, constants, outputs, residual, resids, iterations);
                for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
                    outputIters[i].Set(outputs[i]);
                }
                residualIter.Set(residual);
                if (m_allResiduals) {
                    allResidualsIter.Set(resids);
                }
                iterationsIter.Set(iterations);
            };
            threadPool.enqueue(task);
            progress.CompletedPixel(); // We can get away with this because enqueue blocks if the queue is full
        } else {
            for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
                outputIters[i].Set(0);
            }
            VariableLengthVector<float> residZeros(m_algorithm->dataSize()); residZeros.Fill(0.);
            allResidualsIter.Set(residZeros);
            residualIter.Set(0);
            iterationsIter.Set(0);
        }
        
        if (this->GetMask())
            ++maskIter;
        for (size_t i = 0; i < m_algorithm->numInputs(); i++) {
            ++dataIters[i];
        }
        for (size_t i = 0; i < m_algorithm->numConsts(); i++) {
            if (this->GetConst(i))
                ++constIters[i];
        }
        for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
            ++outputIters[i];
        }
        if (m_allResiduals)
            ++allResidualsIter;
        ++residualIter;
        ++iterationsIter;
    }
    clock.Stop();
    m_elapsedTime = clock.GetTotal();
}
} // namespace ITK

#endif // APPLYALGORITHMFILTER_HXX
