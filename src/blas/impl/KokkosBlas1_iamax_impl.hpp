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
#ifndef KOKKOSBLAS1_IAMAX_IMPL_HPP_
#define KOKKOSBLAS1_IAMAX_IMPL_HPP_

#include <KokkosKernels_config.h>
#include <Kokkos_Core.hpp>
#include <KokkosBlas1_iamax_spec.hpp>

namespace KokkosBlas {
namespace Impl {

/// \brief Iamax functor for single vectors.
///
/// \tparam RV 0-D output View
/// \tparam XV 1-D input View
/// \tparam MagType Magnitude type
/// \tparam SizeType Index type.  Use int (32 bits) if possible.
template <class RV, class XV, class MagType,
          class SizeType = typename XV::size_type>
struct V_Iamax_Functor {
  using size_type   = SizeType;
  using mag_type    = MagType;
  using xvalue_type = typename XV::non_const_value_type;
  using IPT         = Kokkos::Details::InnerProductSpaceTraits<xvalue_type>;
  using value_type  = typename RV::value_type;

  typename XV::const_type m_x;

  V_Iamax_Functor(const XV& x) : m_x(x) {
    static_assert(Kokkos::is_view<RV>::value,
                  "KokkosBlas::Impl::V_Iamax_Functor: "
                  "R is not a Kokkos::View.");
    static_assert(Kokkos::is_view<XV>::value,
                  "KokkosBlas::Impl::V_Iamax_Functor: "
                  "X is not a Kokkos::View.");
    static_assert(std::is_same<typename RV::value_type,
                               typename RV::non_const_value_type>::value,
                  "KokkosBlas::Impl::V_Iamax_Functor: R is const.  "
                  "It must be nonconst, because it is an output argument "
                  "(we have to be able to write to its entries).");
    static_assert(RV::rank == 0 && XV::rank == 1,
                  "KokkosBlas::Impl::V_Iamax_Functor: "
                  "RV must have rank 0 and XV must have rank 1.");
  }

  KOKKOS_INLINE_FUNCTION void operator()(const size_type i,
                                         value_type& lmaxloc) const {
    mag_type val    = IPT::norm(m_x(i - 1));
    mag_type maxval = IPT::norm(m_x(lmaxloc - 1));
    if (val > maxval) lmaxloc = i;
  }

  KOKKOS_INLINE_FUNCTION void init(value_type& update) const {
    update = Kokkos::reduction_identity<typename RV::value_type>::max() + 1;
  }

  KOKKOS_INLINE_FUNCTION void join(value_type& update,
                                   const value_type& source) const {
    mag_type source_val = IPT::norm(m_x(source - 1));
    mag_type update_val = IPT::norm(m_x(update - 1));
    if (update_val < source_val) update = source;
  }
};

/// \brief Find the index of the element with the maximum magnitude of the
/// single vector (1-D
///   View) X, and store the result in the 0-D View r.
template <class RV, class XV, class SizeType>
void V_Iamax_Invoke(const RV& r, const XV& X) {
  using execution_space = typename XV::execution_space;
  using AT = Kokkos::Details::ArithTraits<typename XV::non_const_value_type>;
  using mag_type = typename AT::mag_type;

  const SizeType numRows = static_cast<SizeType>(X.extent(0));

  // Avoid MaxLoc Reduction if this is a zero length view
  if (numRows == 0) {
    Kokkos::deep_copy(r, 0);
    return;
  }

  Kokkos::RangePolicy<execution_space, SizeType> policy(1, numRows + 1);

  using functor_type = V_Iamax_Functor<RV, XV, mag_type, SizeType>;
  functor_type op(X);
  Kokkos::parallel_reduce("KokkosBlas::Iamax::S0", policy, op, r);
}

/// \brief Find the index of the element with the maximum magnitude of the
/// columns of the
///   multivector (2-D View) X, and store result(s) in the 1-D View r.
template <class RV, class XMV, class SizeType>
void MV_Iamax_Invoke(const RV& r, const XMV& X) {
  for (size_t i = 0; i < X.extent(1); i++) {
    auto ri = Kokkos::subview(r, i);
    auto Xi = Kokkos::subview(X, Kokkos::ALL(), i);
    V_Iamax_Invoke<decltype(ri), decltype(Xi), SizeType>(ri, Xi);
  }
}

}  // namespace Impl
}  // namespace KokkosBlas

#endif  // KOKKOSBLAS1_IAMAX_IMPL_HPP_
