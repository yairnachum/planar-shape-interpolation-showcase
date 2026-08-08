// Cauchy-coordinate basis evaluation for a single polygonal cage loop.
// This is the simply-connected case only. Logarithmic hole bases and all
// period-closing extensions are intentionally excluded from the public sample.

#include <Eigen/Dense>
#include <complex>
#include <cmath>

using Cd    = std::complex<double>;
using VecCd = Eigen::VectorXcd;
using MatCd = Eigen::MatrixXcd;

void computeCauchyBasisSimplyConnected(
    const VecCd& queryPoints,
    const VecCd& cageVertices,
    MatCd& C,
    MatCd& D)
{
    const int N = static_cast<int>(queryPoints.size());
    const int m = static_cast<int>(cageVertices.size());

    C = MatCd::Zero(N, m);
    D = MatCd::Zero(N, m);

    for (int j = 0; j < m; ++j) {
        const int jm1 = (j - 1 + m) % m;
        const int jp1 = (j + 1) % m;

        const Cd zj   = cageVertices(j);
        const Cd zjm1 = cageVertices(jm1);
        const Cd zjp1 = cageVertices(jp1);

        const Cd Aj   = zj - zjm1;
        const Cd Ajp1 = zjp1 - zj;

        const VecCd Bj   = VecCd::Constant(N, zj)   - queryPoints;
        const VecCd Bjm1 = VecCd::Constant(N, zjm1) - queryPoints;
        const VecCd Bjp1 = VecCd::Constant(N, zjp1) - queryPoints;

        const VecCd term1 =
            (Bjp1 / Ajp1).cwiseProduct(
                (Bjp1.cwiseQuotient(Bj)).unaryExpr(
                    [](Cd x) { return std::log(x); }));

        const VecCd term2 =
            (Bjm1 / Aj).cwiseProduct(
                (Bj.cwiseQuotient(Bjm1)).unaryExpr(
                    [](Cd x) { return std::log(x); }));

        C.col(j) = (term1 - term2) / (2.0 * M_PI * Cd(0, 1));

        const VecCd d1 =
            (1.0 / Aj) *
            (Bj.cwiseQuotient(Bjm1)).unaryExpr(
                [](Cd x) { return std::log(x); });

        const VecCd d2 =
            (1.0 / Ajp1) *
            (Bj.cwiseQuotient(Bjp1)).unaryExpr(
                [](Cd x) { return std::log(x); });

        D.col(j) = (d1 + d2) / (2.0 * M_PI * Cd(0, 1));

        // Stable limiting values at cage vertices.
        for (int i = 0; i < N; ++i) {
            if (std::abs(Bj(i)) < 1e-12) {
                C(i, j) = Cd(1.0, 0.0);
                D(i, j) = Cd(0.0, 0.0);
            } else if (
                std::abs(Bjm1(i)) < 1e-12 ||
                std::abs(Bjp1(i)) < 1e-12)
            {
                C(i, j) = Cd(0.0, 0.0);
                D(i, j) = Cd(0.0, 0.0);
            }
        }
    }
}
