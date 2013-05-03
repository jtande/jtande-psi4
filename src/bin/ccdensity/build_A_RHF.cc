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
    \ingroup CCDENSITY
    \brief Enter brief description of file here 
*/
#include <libdpd/dpd.h>
#include "MOInfo.h"
#include "Params.h"
#include "Frozen.h"
#define EXTERN
#include "globals.h"

namespace psi { namespace ccdensity {

/* BUILD_A_RHF(): Construct the molecular orbital Hessian, A, for
** RHF orbitals. At the moment we're actually building all symmetry
** blocks of A, though for the orbital Z-vector equations we really
** only need the totally symmetric components.
**
** In spatial orbitals:
**
** A(em,ai) = 4 <mi|ea> - <im|ea> - <me|ia> + (m==i) fea - (e==a) fmi
**
** */

void build_A_RHF(void)
{
  int h, nirreps, e, m, a, i, em, ai, E, M, A, I;
  int Esym, Msym, Asym, Isym;
  dpdfile2 fIJ, fAB;
  dpdbuf4 Amat, D, C;

  nirreps = moinfo.nirreps;

  /* Two-electron integral contributions */
  dpd_buf4_init(&D, PSIF_CC_DINTS, 0, 0, 5, 0, 5, 0, "D <ij|ab>");
  dpd_buf4_sort(&D, PSIF_CC_MISC, rpsq, 11, 11, "A(EM,AI)");
  dpd_buf4_close(&D);
  dpd_buf4_init(&Amat, PSIF_CC_MISC, 0, 11, 11, 11, 11, 0, "A(EM,AI)");
  dpd_buf4_scm(&Amat, 4.0);
  dpd_buf4_close(&Amat);
  dpd_buf4_init(&D, PSIF_CC_DINTS, 0, 0, 5, 0, 5, 0, "D <ij|ab>");
  dpd_buf4_sort_axpy(&D, PSIF_CC_MISC, rqsp, 11, 11, "A(EM,AI)", -1);
  dpd_buf4_close(&D);
  dpd_buf4_init(&C, PSIF_CC_CINTS, 0, 10, 10, 10, 10, 0, "C <ia|jb>");
  dpd_buf4_sort_axpy(&C, PSIF_CC_MISC, qpsr, 11, 11, "A(EM,AI)", -1);
  dpd_buf4_close(&C);

  /* Fock matrix contributions */
  dpd_file2_init(&fIJ, PSIF_CC_OEI, 0, 0, 0, "fIJ");
  dpd_file2_mat_init(&fIJ);
  dpd_file2_mat_rd(&fIJ);
  dpd_file2_init(&fAB, PSIF_CC_OEI, 0, 1, 1, "fAB");
  dpd_file2_mat_init(&fAB);
  dpd_file2_mat_rd(&fAB);

  dpd_buf4_init(&Amat, PSIF_CC_MISC, 0, 11, 11, 11, 11, 0, "A(EM,AI)");
  
  for(h=0; h < nirreps; h++) {
    dpd_buf4_mat_irrep_init(&Amat, h);
    dpd_buf4_mat_irrep_rd(&Amat, h);

    for(em=0; em < Amat.params->rowtot[h]; em++) {
      e = Amat.params->roworb[h][em][0];
      m = Amat.params->roworb[h][em][1];
      E = fAB.params->rowidx[e];
      M = fIJ.params->rowidx[m];
      Esym = fAB.params->psym[e];
      Msym = fIJ.params->psym[m];
      for(ai=0; ai < Amat.params->coltot[h]; ai++) {
	a = Amat.params->colorb[h][ai][0];
	i = Amat.params->colorb[h][ai][1];
	A = fAB.params->colidx[a];
	I = fIJ.params->colidx[i];
	Asym = fAB.params->qsym[a];
	Isym = fIJ.params->qsym[i];

	if((M==I) && (Esym==Asym))
	  Amat.matrix[h][em][ai] += fAB.matrix[Esym][E][A];
	if((E==A) && (Msym==Isym))
	  Amat.matrix[h][em][ai] -= fIJ.matrix[Msym][M][I];
      }
    }

    dpd_buf4_mat_irrep_wrt(&Amat, h);
    dpd_buf4_mat_irrep_close(&Amat, h);
  }

  dpd_buf4_close(&Amat);

  dpd_file2_mat_close(&fIJ);
  dpd_file2_close(&fIJ);
  dpd_file2_mat_close(&fAB);
  dpd_file2_close(&fAB);
}


}} // namespace psi::ccdensity
