/*
 *@BEGIN LICENSE
 *
 * PSI4: an ab initio quantum chemistry software package
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *@END LICENSE
 */

#ifndef _psi_src_lib_libmints_electrostatic_h_
#define _psi_src_lib_libmints_electrostatic_h_

namespace boost {
template<class T> class shared_ptr;
}
namespace psi {

class BasisSet;
class GaussianShell;
class ObaraSaikaTwoCenterRecursion;
class OneBodyAOInt;
class PotentialInt;
class IntegralFactory;
class SphericalTransform;
class Vector3;

/*! \ingroup MINTS
 *  \class PotentialInt
 *  \brief Computes potential integrals.
 * Use an IntegralFactory to create this object.
 */
class ElectrostaticInt : public PotentialInt
{
    void compute_pair(const GaussianShell&, const GaussianShell&)
    {}

public:
    /// Constructor
    ElectrostaticInt(std::vector<SphericalTransform>&, boost::shared_ptr<BasisSet>, boost::shared_ptr<BasisSet>, int deriv=0);
    ~ElectrostaticInt();

    // Intel C++ 12 thinks we're trying to overload the "void compute_shell(int, int)" and warns us about it.
    // The following line is to shut it up.
    #pragma warning disable 1125
    /// Computes integrals between two shells.
    void compute_shell(int, int, const Vector3&);
    /// Computes integrals between two shells.
    void compute_pair(const GaussianShell&, const GaussianShell&, const Vector3&);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
    /// Computes integrals and stores in result.
    void compute(SharedMatrix& result, const Vector3&);
#pragma clang diagnostic pop

    /// Does the method provide first derivatives?
    bool has_deriv1() { return false; }

    static SharedVector nuclear_contribution(boost::shared_ptr<Molecule> mol);
};

}

#endif
