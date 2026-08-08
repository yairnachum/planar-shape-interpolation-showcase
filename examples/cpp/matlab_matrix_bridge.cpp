// Sanitized engineering extract from the private Maya/C++ research project.
// The numerical algorithm itself is intentionally not included here.
//
// This example shows the matrix-transfer layer used to move dense real and
// complex matrices from the MATLAB Engine into native C++ buffers while
// preserving column-major layout when requested.

#include <complex>
#include <cstddef>
#include <mutex>
#include <vector>

// MATLAB Engine/Data API headers omitted from this standalone showcase snippet.

class MatlabMatrixBridge {
public:
    template <typename T>
    int copyRealMatrix(
        const char* name,
        unsigned rows,
        unsigned cols,
        T* destination,
        bool columnMajor)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // Production code fetches the MATLAB variable once, validates its
        // dimensions and type, then copies it directly into the destination.
        auto array = getVariable(name);
        const auto dims = array.dimensions();

        if (dims.rows != rows || dims.cols != cols) {
            return 1;
        }

        std::size_t k = 0;
        for (double value : array.realValues()) {
            const unsigned i = static_cast<unsigned>(k % rows);
            const unsigned j = static_cast<unsigned>(k / rows);
            const std::size_t dst = columnMajor ? k : (static_cast<std::size_t>(i) * cols + j);
            destination[dst] = static_cast<T>(value);
            ++k;
        }
        return 0;
    }

    template <typename T>
    int copyComplexMatrix(
        const char* name,
        unsigned rows,
        unsigned cols,
        std::complex<T>* destination,
        bool columnMajor)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto array = getVariable(name);
        const auto dims = array.dimensions();
        if (dims.rows != rows || dims.cols != cols) {
            return 1;
        }

        std::size_t k = 0;
        for (std::complex<double> value : array.complexValues()) {
            const unsigned i = static_cast<unsigned>(k % rows);
            const unsigned j = static_cast<unsigned>(k / rows);
            const std::size_t dst = columnMajor ? k : (static_cast<std::size_t>(i) * cols + j);
            destination[dst] = {
                static_cast<T>(value.real()),
                static_cast<T>(value.imag())
            };
            ++k;
        }
        return 0;
    }

private:
    // Placeholder facade used only so the public snippet stays independent of
    // the private project and MATLAB SDK-specific implementation details.
    struct Dimensions { unsigned rows; unsigned cols; };
    struct ArrayFacade {
        Dimensions dimensions() const;
        std::vector<double> realValues() const;
        std::vector<std::complex<double>> complexValues() const;
    };

    ArrayFacade getVariable(const char* name);
    std::recursive_mutex mutex_;
};
