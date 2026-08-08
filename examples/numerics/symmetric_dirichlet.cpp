// Selected numerical core from the private Maya deformer.
// This sample covers the published simply-connected deformation pipeline only.
// Multiply-connected / period-closing research code is intentionally omitted.

#include <Eigen/Dense>
#include <complex>
#include <algorithm>

using Cd    = std::complex<double>;
using VecCd = Eigen::VectorXcd;
using VecD  = Eigen::VectorXd;
using MatCd = Eigen::MatrixXcd;
using MatD  = Eigen::MatrixXd;

struct Energy {
    double total;
    double symmetricDirichlet;
    double handlePenalty;
};

Energy computeTotalEnergy(
    const VecCd& phi,
    const MatCd& derivativeBasis,
    const VecD& weights,
    const MatCd& handleBasis,
    const VecCd& targetHandles,
    double lambda)
{
    const VecCd fPrime = derivativeBasis * phi;
    const VecD s = fPrime.cwiseAbs2();
    const VecD safe = s.cwiseMax(1e-30);

    const double eSD =
        ((s + safe.cwiseInverse()).array() * weights.array()).sum();

    const VecCd handleResidual = handleBasis * phi - targetHandles;
    const double eHandles = 0.5 * handleResidual.cwiseAbs2().sum();

    return {eSD + lambda * eHandles, eSD, eHandles};
}

VecCd computeTotalGradient(
    const VecCd& phi,
    const MatCd& derivativeBasis,
    const VecD& weights,
    const MatCd& handleBasis,
    const VecCd& targetHandles,
    double lambda)
{
    const VecCd fPrime = derivativeBasis * phi;
    const VecD s = fPrime.cwiseAbs2();

    VecD factor = VecD::Ones(s.size());
    for (int i = 0; i < s.size(); ++i) {
        if (s(i) > 1e-30)
            factor(i) = 1.0 - 1.0 / (s(i) * s(i));
    }

    const VecCd inner =
        (factor.array() * weights.array())
            .matrix()
            .cast<Cd>()
            .cwiseProduct(fPrime);

    const VecCd gradSD = 2.0 * derivativeBasis.adjoint() * inner;
    const VecCd gradHandles =
        handleBasis.adjoint() * (handleBasis * phi - targetHandles);

    return gradSD + lambda * gradHandles;
}

void computeSymmetricDirichletHessian(
    const VecCd& phi,
    const MatCd& derivativeBasis,
    const VecD& weights,
    MatCd& A,
    MatCd& B)
{
    const VecCd fPrime = derivativeBasis * phi;
    const VecD s = fPrime.cwiseAbs2();

    VecD wA(s.size());
    VecCd wB(s.size());

    for (int i = 0; i < s.size(); ++i) {
        double gamma = 1.0;
        double alpha2 = 0.0;

        if (s(i) > 1e-30) {
            gamma = 1.0 + 1.0 / (s(i) * s(i));
            alpha2 = 2.0 / (s(i) * s(i) * s(i));
        }

        wA(i) = 2.0 * gamma * weights(i);
        wB(i) = 2.0 * alpha2 * fPrime(i) * fPrime(i) * weights(i);
    }

    const MatCd weightedD =
        wA.cwiseSqrt().cast<Cd>().asDiagonal() * derivativeBasis;
    A = weightedD.adjoint() * weightedD;

    const MatCd weightedConjugateD =
        wB.asDiagonal() * derivativeBasis.conjugate();
    B = derivativeBasis.adjoint() * weightedConjugateD;
}

// Complex Hessian blocks are converted to a real 2n x 2n system before
// solving the Newton direction. Small / negative eigenvalues are clamped to
// keep the step numerically stable.
void buildRealHessian(
    const MatCd& A,
    const MatCd& B,
    const VecCd& gradient,
    MatD& H,
    VecD& g)
{
    const int n = static_cast<int>(A.rows());
    const MatCd ApB = A + B;
    const MatCd AmB = A - B;

    H.resize(2 * n, 2 * n);
    H.topLeftCorner(n, n)     = ApB.real();
    H.topRightCorner(n, n)    = -AmB.imag();
    H.bottomLeftCorner(n, n)  = ApB.imag();
    H.bottomRightCorner(n, n) = AmB.real();

    H = (0.5 * (H + H.transpose())).eval();

    g.resize(2 * n);
    g.head(n) = gradient.real();
    g.tail(n) = gradient.imag();
}
