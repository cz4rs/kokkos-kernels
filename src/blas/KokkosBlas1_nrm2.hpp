/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Siva Rajamanickam (srajama@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOSBLAS1_NRM2_HPP_
#define KOKKOSBLAS1_NRM2_HPP_

#include <KokkosBlas1_nrm2_spec.hpp>
#include <KokkosBlas_serial_nrm2.hpp>
#include <KokkosKernels_helpers.hpp>
#include <KokkosKernels_Error.hpp>

namespace KokkosBlas {

/// \brief Return the nrm2 of the vector x.
///
/// \tparam XVector Type of the first vector x; a 1-D Kokkos::View.
///
/// \param x [in] Input 1-D View.
///
/// \return The nrm2 product result; a single value.
template <class XVector>
typename Kokkos::Details::InnerProductSpaceTraits<
    typename XVector::non_const_value_type>::mag_type
nrm2(const XVector& x) {
  static_assert(Kokkos::is_view<XVector>::value,
                "KokkosBlas::nrm2: XVector must be a Kokkos::View.");
  static_assert(XVector::rank == 1,
                "KokkosBlas::nrm2: "
                "XVector must have rank 1.");
  typedef typename Kokkos::Details::InnerProductSpaceTraits<
      typename XVector::non_const_value_type>::mag_type mag_type;

  typedef Kokkos::View<
      typename XVector::const_value_type*,
      typename KokkosKernels::Impl::GetUnifiedLayout<XVector>::array_layout,
      typename XVector::device_type, Kokkos::MemoryTraits<Kokkos::Unmanaged> >
      XVector_Internal;

  typedef Kokkos::View<mag_type, typename XVector_Internal::array_layout,
                       Kokkos::HostSpace,
                       Kokkos::MemoryTraits<Kokkos::Unmanaged> >
      RVector_Internal;

  mag_type result;
  RVector_Internal R = RVector_Internal(&result);
  XVector_Internal X = x;

  Impl::Nrm2<RVector_Internal, XVector_Internal>::nrm2(R, X, true);
  Kokkos::fence();
  return result;
}

/// \brief R(i,j) = nrm2(X(i,j))
///
/// Replace each entry in R with the nrm2olute value (magnitude) of the
/// corresponding entry in X.
///
/// \tparam RMV 1-D or 2-D Kokkos::View specialization.
/// \tparam XMV 1-D or 2-D Kokkos::View specialization.  It must have
///   the same rank as RMV, and its entries must be assignable to
///   those of RMV.
template <class RV, class XMV>
void nrm2(const RV& R, const XMV& X,
          typename std::enable_if<Kokkos::is_view<RV>::value, int>::type = 0) {
  static_assert(Kokkos::is_view<RV>::value,
                "KokkosBlas::nrm2: "
                "R is not a Kokkos::View.");
  static_assert(Kokkos::is_view<XMV>::value,
                "KokkosBlas::nrm2: "
                "X is not a Kokkos::View.");
  static_assert(std::is_same<typename RV::value_type,
                             typename RV::non_const_value_type>::value,
                "KokkosBlas::nrm2: R is const.  "
                "It must be nonconst, because it is an output argument "
                "(we have to be able to write to its entries).");
  static_assert(((RV::rank == 0) && (XMV::rank == 1)) ||
                    ((RV::rank == 1) && (XMV::rank == 2)),
                "KokkosBlas::nrm2: "
                "RV and XMV must either have rank 0 and 1 or rank 1 and 2.");

  typedef typename Kokkos::Details::InnerProductSpaceTraits<
      typename XMV::non_const_value_type>::mag_type mag_type;
  static_assert(std::is_same<typename RV::value_type, mag_type>::value,
                "KokkosBlas::nrm2: R must have the magnitude type of"
                "the xvectors value_type it is an output argument "
                "(we have to be able to write to its entries).");

  // Check compatibility of dimensions at run time.
  if (X.extent(1) != R.extent(0)) {
    std::ostringstream os;
    os << "KokkosBlas::nrm2 (MV): Dimensions of R and X do not match: "
       << "R: " << R.extent(0) << ", X: " << X.extent(0) << " x "
       << X.extent(1);
    KokkosKernels::Impl::throw_runtime_exception(os.str());
  }

  using UnifiedXLayout =
      typename KokkosKernels::Impl::GetUnifiedLayout<XMV>::array_layout;
  using UnifiedRVLayout =
      typename KokkosKernels::Impl::GetUnifiedLayoutPreferring<
          RV, UnifiedXLayout>::array_layout;

  // Create unmanaged versions of the input Views.  RV and XMV may be
  // rank 1 or rank 2.
  typedef Kokkos::View<typename RV::non_const_data_type, UnifiedRVLayout,
                       typename RV::device_type,
                       Kokkos::MemoryTraits<Kokkos::Unmanaged> >
      RV_Internal;
  typedef Kokkos::View<typename XMV::const_data_type, UnifiedXLayout,
                       typename XMV::device_type,
                       Kokkos::MemoryTraits<Kokkos::Unmanaged> >
      XMV_Internal;

  RV_Internal R_internal  = R;
  XMV_Internal X_internal = X;

  Impl::Nrm2<RV_Internal, XMV_Internal>::nrm2(R_internal, X_internal, true);
}

///
/// Serial nrm2
///
template <class XMV>
KOKKOS_INLINE_FUNCTION typename Kokkos::Details::InnerProductSpaceTraits<
    typename XMV::non_const_value_type>::mag_type
serial_nrm2(const XMV X) {
#if (KOKKOSKERNELS_DEBUG_LEVEL > 0)
  static_assert(Kokkos::is_view<XMV>::value,
                "KokkosBlas::serial_nrm2: XMV is not a Kokkos::View");
  static_assert(XMV::Rank == 1,
                "KokkosBlas::serial_nrm2: XMV must have rank 1");
#endif  // KOKKOSKERNELS_DEBUG_LEVEL

  return Impl::serial_nrm2(X.extent(0), X.data(), X.stride_0());
}

template <class RV, class XMV>
KOKKOS_INLINE_FUNCTION int serial_nrm2(const XMV X, const RV& R) {
// Do some compile time check when debug is enabled
#if (KOKKOSKERNELS_DEBUG_LEVEL > 0)
  static_assert(Kokkos::is_view<XMV>::value,
                "KokkosBlas::serial_nrm2: XMV is not a Kokkos::View");
  static_assert(Kokkos::is_view<RV>::value,
                "KokkosBlas::serial_nrm2: RV is not a Kokkos::View");
  static_assert(std::is_same<typename RV::value_type,
                             typename RV::non_const_value_type>::value,
                "KokkosBlas::serial_nrm2: R is const.  "
                "It must be nonconst, because it is an output argument "
                "(we have to be able to write to its entries).");
  static_assert(((RV::rank == 0) && (XMV::rank == 1)) ||
                    ((RV::rank == 1) && (XMV::rank == 2)),
                "KokkosBlas::serial_nrm2: "
                "RV and XMV must either have rank 0 and 1 or rank 1 and 2.");

  using norm_type = typename Kokkos::Details::InnerProductSpaceTraits<
      typename XMV::non_const_value_type>::mag_type;
  static_assert(
      std::is_same<typename RV::non_const_value_type, norm_type>::value,
      "KokkosBlas::serial_nrm2: RV must have same value_type as"
      " Kokkos::ArithTraits<XMV::value_type>::mag_type");

  if (R.extent(0) != X.extent(1)) {
    KOKKOS_IMPL_DO_NOT_USE_PRINTF(
        "KokkosBlas::serial_nrm2 (MV): Dimensions of R and X do not match,"
        " R: %d and X: %d x %d.\n",
        R.extent_int(0), X.extent_int(0), X.extent_int(1));
    return 1;
  }
#endif  // KOKKOSKERNELS_DEBUG_LEVEL

  Impl::serial_nrm2(X.extent(0), X.extent(1), X.data(), X.stride_0(),
                    X.stride_1(), R.data(), R.stride_0());
  return 0;
}

}  // namespace KokkosBlas

#endif  // KOKKOSBLAS1_NRM2_HPP_
