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

/*! \file
    \ingroup CCLAMBDA
    \brief Enter brief description of file here 
*/
#include <cstdio>
#include <libdpd/dpd.h>
#include "MOInfo.h"
#include "Params.h"
#define EXTERN
#include "globals.h"

namespace psi { namespace cclambda {

void WabeiL1(int L_irr)
{
  dpdfile2 newL1;
  dpdbuf4 W, L2;

  dpd_file2_init(&newL1, PSIF_CC_LAMBDA, L_irr, 0, 1, "New L(I,A)");
  dpd_buf4_init(&W, PSIF_CC_HBAR, 0, 11, 7, 11, 7, 0, "W(AM,EF)");
  dpd_buf4_init(&L2, PSIF_CC_LAMBDA, L_irr, 0, 7, 2, 7, 0, "L2(IM,EF)");
  dpd_contract442(&L2, &W, &newL1, 0, 0, 1.0, 1.0);
  dpd_buf4_close(&W);
  dpd_buf4_close(&L2);
  dpd_buf4_init(&W, PSIF_CC_HBAR, 0, 11, 5, 11, 5, 0, "W(Am,Ef)");
  dpd_buf4_init(&L2, PSIF_CC_LAMBDA, L_irr, 0, 5, 0, 5, 0, "L2(Im,Ef)");
  dpd_contract442(&L2, &W, &newL1, 0, 0, 1.0, 1.0);
  dpd_buf4_close(&W);
  dpd_buf4_close(&L2);
  dpd_file2_close(&newL1);

  dpd_file2_init(&newL1, PSIF_CC_LAMBDA, L_irr, 0, 1, "New L(i,a)");
  dpd_buf4_init(&W, PSIF_CC_HBAR, 0, 11, 7, 11, 7, 0, "W(am,ef)");
  dpd_buf4_init(&L2, PSIF_CC_LAMBDA, L_irr, 0, 7, 2, 7, 0, "L2(im,ef)");
  dpd_contract442(&L2, &W, &newL1, 0, 0, 1.0, 1.0);
  dpd_buf4_close(&W);
  dpd_buf4_close(&L2);
  dpd_buf4_init(&W, PSIF_CC_HBAR, 0, 11, 5, 11, 5, 0, "W(aM,eF)");
  dpd_buf4_init(&L2, PSIF_CC_LAMBDA, L_irr, 0, 5, 0, 5, 0, "L2(iM,eF)");
  dpd_contract442(&L2, &W, &newL1, 0, 0, 1.0, 1.0);
  dpd_buf4_close(&W);
  dpd_buf4_close(&L2);
  dpd_file2_close(&newL1);
}

}} // namespace psi::cclambda
