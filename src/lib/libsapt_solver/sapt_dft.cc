/* 
 *  SAPT_DFT.CC 
 *
 */

#ifdef _MKL
#include <mkl.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif


#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
#include <utility>
#include <time.h>

#include <psifiles.h>
#include <psi4-dec.h>
#include <libciomr/libciomr.h>
#include <libpsio/psio.h>
#include <libchkpt/chkpt.hpp>
#include <libipv1/ip_lib.h>
#include <libiwl/iwl.hpp>
#include <libqt/qt.h>
#include <psifiles.h>

#include <libmints/mints.h>

#include "sapt_dft.h"

using namespace boost;
using namespace std;
using namespace psi;

namespace psi { namespace sapt {

SAPT_DFT::SAPT_DFT(Options& options, shared_ptr<PSIO> psio, shared_ptr<Chkpt> chkpt)
    : SAPT0(options, psio, chkpt)
{
    E_UCHF_disp_ = 0.0;
    E_TDDFT_disp_ = 0.0;
    E_MP2C_delta_ = E_TDDFT_disp_ - E_UCHF_disp_;
    E_MP2C_int_ = E_MP2_int_ + E_MP2C_delta_;
}

SAPT_DFT::~SAPT_DFT()
{
}

double SAPT_DFT::compute_energy()
{  
    // THe obligatory
    print_header(); 
    // Allocate space
    allocate_arrays();
    // DF integrals for use with X
    fprintf(outfile, "  Computing density-fitting MO integrals.\n");
    compute_integrals();

    // This needs to go away
    compute_amplitudes();
    elst10();
    exch10();
    exch_disp20();
    disp20();
    cphf_induction();
    ind20();
    exch_ind20();
    exit(0);
    // Yeah, all of it, sorry Ed

    // The density matrices (AO)
    fprintf(outfile, "  Computing density matrices.\n");
    compute_D(); // Confirmed
    // The auxiliary S matrix and its inverse
    fprintf(outfile, "  Computing auxiliary overlap matrix.\n");
    compute_S(); // Confirmed
    // The J matrix
    fprintf(outfile, "  Computing auxiliary fitting matrix.\n");
    compute_J(); // Confirmed
    // The bloody W matrix of Dyson's Equation
    fprintf(outfile, "  Computing interelectronic interaction matrix.\n");
    compute_W();


    // Get a quadrature object for imaginary frequencies
    shared_ptr<OmegaQuadrature> quad = shared_ptr<OmegaQuadrature> ( \
        new OmegaQuadrature(options_.get_int("N_OMEGA")));

    // Perform quadrature over imaginary frequencies
    fprintf(outfile, "\n");
    fprintf(outfile, "  -----------------------------------------------------------------------------------\n");       
    fprintf(outfile, "   =========================> CASIMIR-POLDER INTEGRATION <==========================\n");       
    fprintf(outfile, "  -----------------------------------------------------------------------------------\n");       
    fprintf(outfile, "   Point      Omega            Weight           E_UCHF [mH]          E_TDDFT [mH]\n");       
    fprintf(outfile, "  -----------------------------------------------------------------------------------\n");       
    fflush(outfile);
    int n = 0;
    for (quad->reset(); !quad->isDone(); quad->nextPoint() ) {
    
        double omega = quad->getOmega();
        double weight = quad->getWeight();

        // Uncoupled response functions
        compute_X_0(omega);
        // UCHF Dispersion
        double UCHF = compute_UCHF_disp();
        // Coupled response functions
        compute_X_coup(omega);
        // TDDFT Dispersion
        double TDDFT = compute_TDDFT_disp();
        
        n++;
        fprintf(outfile, "   %3d   %12.8E   %12.8E   %18.12f   %18.12f\n", \
           n, omega, weight, UCHF*1000.0,TDDFT*1000.0);       
        E_UCHF_disp_ += weight*UCHF;
        E_TDDFT_disp_ += weight*TDDFT;
    }
    fprintf(outfile, "  -----------------------------------------------------------------------------------\n");       
    fprintf(outfile, "    @ UCHF Dispersion Energy:  %18.12f [mH] %18.12f [kcal]\n", E_UCHF_disp_*1000.0, \
        E_UCHF_disp_*627.509);    
    fprintf(outfile, "    @ TDDFT Dispersion Energy: %18.12f [mH] %18.12f [kcal]\n", E_TDDFT_disp_*1000.0, \
        E_TDDFT_disp_*627.509);    
    fprintf(outfile, "  -----------------------------------------------------------------------------------\n");       
    fflush(outfile);
        
   
    // Free arrays
    free_arrays();
 
    // More fun
    print_results();

    return E_MP2C_int_;
}

void SAPT_DFT::print_header()
{
     fprintf(outfile,"                 SAPT DFT  \n");
     fprintf(outfile,"       Rob Parrish and Ed Hohenstein\n") ;
     fprintf(outfile,"             9 November 2009\n") ;
     fprintf(outfile,"\n");
     fprintf(outfile,"          Orbital Information\n");
     fprintf(outfile,"        -----------------------\n");
     fprintf(outfile,"          NSO     = %9d\n",calc_info_.nso);
     fprintf(outfile,"          NMO     = %9d\n",calc_info_.nmo);
     fprintf(outfile,"          NRI     = %9d\n",calc_info_.nri);
     fprintf(outfile,"          NOCC_A  = %9d\n",calc_info_.noccA);
     fprintf(outfile,"          NOCC_B  = %9d\n",calc_info_.noccB);
     fprintf(outfile,"          NVIR_A  = %9d\n",calc_info_.nvirA);
     fprintf(outfile,"          NVIR_B  = %9d\n\n",calc_info_.nvirB);
    
     #ifdef _OPENMP
     fprintf(outfile,"  Running SAPT_DFT with %d OMP threads\n\n",
       omp_get_max_threads());
     #endif 
    
     fflush(outfile);
} 
double SAPT_DFT::print_results() {
    return 0.0;
} 
void SAPT_DFT::allocate_arrays() {
    
    int naux = calc_info_.nri;
    // Allocation of UCHF response params
    X0_A_ = block_matrix(naux,naux);
    X0_B_ = block_matrix(naux,naux);
    // Allocation of UCHF response params
    XC_A_ = block_matrix(naux,naux);
    XC_B_ = block_matrix(naux,naux);
}
void SAPT_DFT::free_arrays() {
    free_block(X0_A_);
    free_block(X0_B_);
    free_block(XC_A_);
    free_block(XC_B_);
}
void SAPT_DFT::compute_S() {
    
    // Get my bearings
    int naux = calc_info_.nri; 
    IntegralFactory rifactory_JS(ribasis_, ribasis_, zero_, zero_);
    shared_ptr<OneBodyInt> S = shared_ptr<OneBodyInt>(rifactory_JS.overlap());
    MatrixFactory matJS;
    matJS.init_with(1,&naux,&naux);

    //Put the integrals in a good old double**
    SharedMatrix S_J = shared_ptr<Matrix>(matJS.create_matrix("S_J"));
    //Compute those integrals
    S->compute(S_J);
    //S_J->print(outfile);
    S_ = S_J->to_block_matrix();
   
}
void SAPT_DFT::compute_D() {
    
    //Some parameters
    int norbs = calc_info_.nso;
    int noccA = calc_info_.noccA;
    int noccB = calc_info_.noccB;
    int nvirA = calc_info_.nvirA;
    int nvirB = calc_info_.nvirB;
    int nmoA = noccA+nvirA;
    int nmoB = noccB+nvirB;
    double **CA = calc_info_.CA;
    double **CB = calc_info_.CB;

    //fprintf(outfile,"CA:\n");
    //print_mat(CA,norbs,nmoA,outfile);
    //fprintf(outfile,"CB:\n");
    //print_mat(CB,norbs,nmoB,outfile);

    D_A_ = block_matrix(norbs,norbs);
    C_DGEMM('N','T',norbs,norbs,noccA,1.0,&CA[0][0],nmoA,&CA[0][0],nmoA,0.0,&D_A_[0][0],norbs);
    //fprintf(outfile,"DA:\n");
    //print_mat(D_A_,norbs,norbs,outfile);
    D_B_ = block_matrix(norbs,norbs);
    C_DGEMM('N','T',norbs,norbs,noccB,1.0,&CB[0][0],nmoB,&CB[0][0],nmoB,0.0,&D_B_[0][0],norbs);
    //fprintf(outfile,"DB:\n");
    //print_mat(D_B_,norbs,norbs,outfile);
}
void SAPT_DFT::compute_J() {
    
    int naux = calc_info_.nri;
    IntegralFactory rifactory_J(ribasis_, zero_, ribasis_, zero_);

    TwoBodyInt* Jint = rifactory_J.eri();
    //double **calc_info_.J = block_matrix(ribasis_->nbf(), ribasis_->nbf());
    J_ = block_matrix(ribasis_->nbf(), ribasis_->nbf());
    const double *Jbuffer = Jint->buffer();

    int index = 0;

     for (int MU=0; MU < ribasis_->nshell(); ++MU) {
        int nummu = ribasis_->shell(MU)->nfunction();

        for (int NU=0; NU <= MU; ++NU) {
            int numnu = ribasis_->shell(NU)->nfunction();

            Jint->compute_shell(MU, 0, NU, 0);

            index = 0;
            for (int mu=0; mu < nummu; ++mu) {
                int omu = ribasis_->shell(MU)->function_index() + mu;

                for (int nu=0; nu < numnu; ++nu, ++index) {
                    int onu = ribasis_->shell(NU)->function_index() + nu;


                    J_[omu][onu] = Jbuffer[index];
                    J_[onu][omu] = Jbuffer[index];
                }
            }
        }   
    }

    

    Jinv_ = block_matrix(naux,naux);
    C_DCOPY(naux*(ULI)naux, J_[0], 1, Jinv_[0], 1);

    // Cholesky factorization
    int stat = C_DPOTRF('U',naux,Jinv_[0],naux);
    // Inverse 
    stat = C_DPOTRI('U',naux,Jinv_[0],naux);

    // Rexpand J^-1 (Upper triangle)
    for (int P = 0; P < naux; P++)
        for (int Q = P+1; Q < naux; Q++)
            Jinv_[P][Q] = Jinv_[Q][P];

    //fprintf(outfile,"J:\n");
    //print_mat(J_,naux,naux,outfile);
    //fprintf(outfile,"J^-1:\n");
    //print_mat(Jinv_,naux,naux,outfile);
}
void SAPT_DFT::compute_W() {
   
    // Get my bearings
    int naux = calc_info_.nri; 
    int norbs = calc_info_.nso;
    int noccA = calc_info_.noccA;
    int noccB = calc_info_.noccB;
    int nvirA = calc_info_.nvirA;
    int nvirB = calc_info_.nvirB;

    W_A_ = block_matrix(naux,naux);
    W_B_ = block_matrix(naux,naux);

    double* c_A = init_array(naux);
    double* c_B = init_array(naux);
    double* d_A = init_array(naux);
    double* d_B = init_array(naux);

    // =============== Density Coefficients ================//

    IntegralFactory rifactory(basisset_, basisset_, ribasis_, zero_);

    const double *buffer;
    shared_ptr<TwoBodyInt> eri = shared_ptr<TwoBodyInt>(rifactory.eri());
    buffer = eri->buffer();

    // Indexing for three-index AO tensors
    int numP,Pshell,MU,NU,P,PHI,mu,nu,nummu,numnu,omu,onu;
    int index;

    int max_P_shell = 0;
    for (Pshell=0; Pshell < ribasis_->nshell(); ++Pshell)
        if (ribasis_->shell(Pshell)->nfunction() > max_P_shell)
            max_P_shell = ribasis_->shell(Pshell)->nfunction();

    double** Amn = block_matrix(max_P_shell,norbs*(ULI)norbs);
    
    // A bit naive at the moment (no sieves or threading)
    for (Pshell=0; Pshell < ribasis_->nshell(); ++Pshell) {
        numP = ribasis_->shell(Pshell)->nfunction();
        for (MU=0; MU < basisset_->nshell(); ++MU) {
            nummu = basisset_->shell(MU)->nfunction();
            for (NU=0; NU < basisset_->nshell(); ++NU) {
                numnu = basisset_->shell(NU)->nfunction();
                eri->compute_shell(MU, NU, Pshell, 0);
                for (mu=0 ; mu < nummu; ++mu) {
                    omu = basisset_->shell(MU)->function_index() + mu;
                    for (nu=0; nu < numnu; ++nu) {
                        onu = basisset_->shell(NU)->function_index() + nu;
                        for (P=0; P < numP; ++P) {
                            Amn[P][omu*norbs+onu] = buffer[mu*numnu*numP+nu*numP+P];
                        }
                    }
                }
            }
        }
        for (P = 0; P < numP; P++) {
            c_A[P] = C_DDOT(norbs*(ULI)norbs, Amn[P], 1, D_A_[0], 1);
            c_B[P] = C_DDOT(norbs*(ULI)norbs, Amn[P], 1, D_B_[0], 1);
        }
    }
   
    free_block(Amn);

    // d_A = S^-1 c_A
    C_DGEMV('N', naux, naux, 1.0, Jinv_[0], naux, c_A, 1, 0.0, d_A, 1);
    // d_B = S^-1 c_B
    C_DGEMV('N', naux, naux, 1.0, Jinv_[0], naux, c_B, 1, 0.0, d_B, 1);

    free(c_A);     
    free(c_B);     
    
    // =============== Heavy Three-Index Tensor ================//
    
    IntegralFactory PQRfactory(ribasis_, ribasis_, ribasis_, zero_);
    double** PQR = block_matrix(max_P_shell*max_P_shell,naux);
    double* TempA = init_array(max_P_shell*max_P_shell);
    double* TempB = init_array(max_P_shell*max_P_shell);

    eri = shared_ptr<TwoBodyInt>(PQRfactory.eri());
    buffer = eri->buffer();

    int Q, numQ, R, numR, q, p, r, op, oq;
    // A bit naive at the moment (no sieves or threading)
    for (P=0; P < ribasis_->nshell(); ++P) {
        numP = ribasis_->shell(P)->nfunction();
        for (Q=0; Q< basisset_->nshell(); ++Q) {
            numQ = ribasis_->shell(Q)->nfunction();
            for (R=0; R < ribasis_->nshell();  ++R) {
                numR = ribasis_->shell(R)->nfunction();
                eri->compute_shell(P, Q, R, 0); // TODO Should be overlap type
                for (p=0 ; p < numP; ++p) {
                    for (q=0; q < numQ; ++q) {
                        for (r=0; r < numR; ++r) {
                            PQR[p*numQ + q][r + ribasis_->shell(R)->function_index()] = \
                                buffer[p*numQ*numR+q*numR+r];
                        }
                    }
                }
            }
            // Multiply
            C_DGEMV('N', numP*numQ, naux, 1.0, PQR[0], naux, d_A, 1, 0.0, TempA, 1);
            C_DGEMV('N', numP*numQ, naux, 1.0, PQR[0], naux, d_B, 1, 0.0, TempB, 1);
            for (p=0 ; p < numP; ++p) {
                op = ribasis_->shell(P)->function_index() + p;
                for (q=0; q < numQ; ++q) {
                    oq = ribasis_->shell(Q)->function_index() + q;
                    W_A_[op][oq] = TempA[p*numQ + q];
                    W_B_[op][oq] = TempB[p*numQ + q];
                }
            }
        }
    }
    
    free_block(PQR);

    free(TempA);     
    free(TempB);     
    free(d_A);     
    free(d_B);     

    // =============== M -> Mprime ================//
    
    double **V = block_matrix(naux, naux);
    C_DCOPY(naux*(ULI)naux,S_[0],1,V[0],1);
    // Form V : V'SV = 1 
    // First, diagonalize V
    // the C_DSYEV call replaces the original matrix J with its eigenvectors
    double* eigval = init_array(naux);
    int lwork = naux * 3;
    double* work = init_array(lwork);
    int stat = C_DSYEV('v','u',naux,V[0],naux,eigval, work,lwork);
    if (stat != 0) {
        fprintf(outfile, "C_DSYEV failed\n");
        exit(PSI_RETURN_FAILURE);
    }
    free(eigval);

    double** Temp = block_matrix(naux,naux);
   
    // TODO Verify fortran ordering 
    C_DGEMM('T','N', naux, naux, naux, 1.0, V[0], naux, W_A_[0], naux, \
        0.0, Temp[0], naux);
    C_DGEMM('N','N', naux, naux, naux, 1.0, Temp[0], naux, V[0], naux, \
        0.0, W_A_[0], naux);
    
    C_DGEMM('T','N', naux, naux, naux, 1.0, V[0], naux, W_B_[0], naux, \
        0.0, Temp[0], naux);
    C_DGEMM('N','N', naux, naux, naux, 1.0, Temp[0], naux, V[0], naux, \
        0.0, W_B_[0], naux);

    // =============== Apply functional kernel ================//
   
    double C_x = 3.0/8.0*pow(3.0,1.0/3.0)*pow(4.0,2.0/3.0)*pow(M_PI,-1.0/3.0); 
    double** U = block_matrix(naux,naux);

    // Monomer A
    C_DCOPY(naux*(ULI)naux,W_A_[0],1,U[0],1);
    // Form V : V'SV = 1 
    // First, diagonalize V
    // the C_DSYEV call replaces the original matrix J with its eigenvectors
    double* lambda = init_array(naux);
    stat = C_DSYEV('v','u',naux,U[0],naux,lambda, work,lwork);
    if (stat != 0) {
        fprintf(outfile, "C_DSYEV failed\n");
        exit(PSI_RETURN_FAILURE);
    }

    // Here's the functional
    C_DCOPY(naux*(ULI)naux,U[0],1,Temp[0],1);
    for (int k = 0; k < naux; k++) {
        lambda[k] = -8.0/9.0 * C_x * pow(lambda[k], -2.0/3.0);
        C_DSCAL(naux, lambda[k], Temp[k], 1);
    }

    // Now revert to M
    C_DGEMM('T','N',naux, naux, naux, 1.0, U[0], naux, Temp[0], naux, \
        0.0, W_A_[0], naux);
    C_DGEMM('N','N',naux, naux, naux, 1.0, V[0], naux, W_A_[0], naux, \
        0.0, Temp[0], naux);
    C_DGEMM('N','N',naux, naux, naux, 1.0, S_[0], naux, Temp[0], naux, \
        0.0, W_A_[0], naux);
    C_DGEMM('N','T',naux, naux, naux, 1.0, W_A_[0], naux, V[0], naux, \
        0.0, Temp[0], naux);
    C_DGEMM('N','T',naux, naux, naux, 1.0, Temp[0], naux, S_[0], naux, \
        0.0, W_A_[0], naux);
    
    // Monomer B
    C_DCOPY(naux*(ULI)naux,W_B_[0],1,U[0],1);
    // Form V : V'SV = 1 
    // First, diagonalize V
    // the C_DSYEV call replaces the original matrix J with its eigenvectors
    stat = C_DSYEV('v','u',naux,U[0],naux,lambda, work,lwork);
    if (stat != 0) {
        fprintf(outfile, "C_DSYEV failed\n");
        exit(PSI_RETURN_FAILURE);
    }

    // Here's the functional
    C_DCOPY(naux*(ULI)naux,U[0],1,Temp[0],1);
    for (int k = 0; k < naux; k++) {
        lambda[k] = -8.0/9.0 * C_x * pow(lambda[k], -2.0/3.0);
        C_DSCAL(naux, lambda[k], Temp[k], 1);
    }

    // Now revert to M
    C_DGEMM('T','N',naux, naux, naux, 1.0, U[0], naux, Temp[0], naux, \
        0.0, W_B_[0], naux);
    C_DGEMM('N','N',naux, naux, naux, 1.0, V[0], naux, W_B_[0], naux, \
        0.0, Temp[0], naux);
    C_DGEMM('N','N',naux, naux, naux, 1.0, S_[0], naux, Temp[0], naux, \
        0.0, W_B_[0], naux);
    C_DGEMM('N','T',naux, naux, naux, 1.0, W_B_[0], naux, V[0], naux, \
        0.0, Temp[0], naux);
    C_DGEMM('N','T',naux, naux, naux, 1.0, Temp[0], naux, S_[0], naux, \
        0.0, W_B_[0], naux);
    
    // =============== Release Memory ================//

    free(lambda);
    free(work);
       
    free_block(Temp);
    free_block(U); 
    free_block(V); 

    for (P = 0; P < naux; P++) {
        for (Q = 0; Q < naux; Q++) {
            W_A_[P][Q] += J_[P][Q];
            W_B_[P][Q] += J_[P][Q];
        }
    }
}
void SAPT_DFT::compute_X_0(double omega) { 
    
    // Get my bearings
    int naux = calc_info_.nri; 
    int norbs = calc_info_.nso;
    int noccA = calc_info_.noccA;
    int noccB = calc_info_.noccB;
    int nvirA = calc_info_.nvirA;
    int nvirB = calc_info_.nvirB;

    double lambda, eps_ia;
    double omega2 = omega*omega;
    double *eps_i, *eps_a; 

    // =============== MONOMER A ================//
    //fprintf(outfile, "  Monomer A X_0:\n");
    double** A_ints = block_matrix(naux, nvirA*(ULI)noccA);
    eps_i = calc_info_.evalsA;
    eps_a = calc_info_.evalsA + noccA;

    //fprintf(outfile,"  Epsilon occ:\n");
    //for (int i = 0; i < noccA; i++)
    //    fprintf(outfile, "   i = %d: eps = %20.14f\n", i, eps_i[i]);
    //fprintf(outfile,"  Epsilon virt:\n");
    //for (int i = 0; i < nvirA; i++)
    //    fprintf(outfile, "   a = %d: eps = %20.14f\n", i, eps_a[i]);

    // Read the AR DF integrals 
    psio_address next_DF_AA = PSIO_ZERO;
    psio_->read(PSIF_SAPT_AA_DF_INTS, "AR Bare RI Integrals", (char*) \
        &A_ints[0][0], naux*nvirA*noccA*(ULI)sizeof(double), next_DF_AA, \
        &next_DF_AA);

    //fprintf(outfile, "  AR Bare RI Integrals (A|ia)\n");
    //print_mat(A_ints, naux, nvirA*(ULI)noccA, outfile);

    // Scale the products ia by \sqrt(lambda_ia)
    //fprintf(outfile, "  Applying response, omega = %20.14f\n",omega);
    for (int i = 0; i < noccA; i++) {
        for (int a = 0; a < nvirA; a++) {
            eps_ia = eps_a[a] - eps_i[i]; 
            lambda = 4.0 * eps_ia / (eps_ia*eps_ia + omega2);
            //fprintf(outfile, "   i = %d, a = %d, lambda = %20.14f\n", i, a, lambda);
            C_DSCAL(noccA*(ULI)nvirA, sqrt(lambda), &A_ints[0][i*nvirA + a], 1); 
        }
    }

    //fprintf(outfile, "  Symmetric Response AR RI Integrals (A|ia)\n");
    //print_mat(A_ints, naux, nvirA*(ULI)noccA, outfile);

    // The only O(N^4) DGEMM in the whole thing
    C_DGEMM('N','T', naux, naux, noccA*(ULI)nvirA, 1.0, A_ints[0], noccA*(ULI)nvirA, \
        A_ints[0], noccA*(ULI)nvirA, 0.0, X0_A_[0], naux);

    //fprintf(outfile, "X_0^A:\n");
    //print_mat(X0_A_, naux, naux, outfile);

    free_block(A_ints);     

    // =============== MONOMER B ================//
    //fprintf(outfile, "  Monomer B X_0:\n");
    double** B_ints = block_matrix(naux, nvirB*(ULI)noccB);
    eps_i = calc_info_.evalsB;
    eps_a = calc_info_.evalsB + noccB;

    //fprintf(outfile,"  Epsilon occ:\n");
    //for (int i = 0; i < noccB; i++)
    //    fprintf(outfile, "   i = %d: eps = %20.14f\n", i, eps_i[i]);
    //fprintf(outfile,"  Epsilon virt:\n");
    //for (int i = 0; i < nvirB; i++)
    //    fprintf(outfile, "   a = %d: eps = %20.14f\n", i, eps_a[i]);

    // Read the BS DF integrals 
    psio_address next_DF_BB = PSIO_ZERO;
    psio_->read(PSIF_SAPT_BB_DF_INTS, "BS Bare RI Integrals", (char*) \
        &B_ints[0][0], naux*nvirB*noccB*(ULI)sizeof(double), next_DF_BB, \
        &next_DF_BB);

    //fprintf(outfile, "  BS Bare RI Integrals (A|ia)\n");
    //print_mat(A_ints, naux, nvirB*(ULI)noccB, outfile);

    // Scale the products ia by \sqrt(lambda_ia)
    //fprintf(outfile, "  Applying response, omega = %20.14f\n",omega);
    for (int i = 0; i < noccB; i++) {
        for (int a = 0; a < nvirB; a++) {
            eps_ia = eps_a[a] - eps_i[i]; 
            lambda = 4.0 * eps_ia / (eps_ia*eps_ia + omega2);
            //fprintf(outfile, "   i = %d, a = %d, lambda = %20.14f\n", i, a, lambda);
            C_DSCAL(noccB*(ULI)nvirB, sqrt(lambda), &B_ints[0][i*nvirB + a], 1); 
        }
    }

    //fprintf(outfile, "  Symmetric Response BS RI Integrals (A|ia)\n");
    //print_mat(B_ints, naux, nvirB*(ULI)noccB, outfile);

    // OK, I lied
    C_DGEMM('N','T', naux, naux, noccB*(ULI)nvirB, 1.0, B_ints[0], noccB*(ULI)nvirB, \
        B_ints[0], noccB*(ULI)nvirB, 0.0, X0_B_[0], naux);

    //fprintf(outfile, "X_0^B:\n");
    //print_mat(X0_B_, naux, naux, outfile);

    free_block(B_ints);     
}
void SAPT_DFT::compute_X_coup(double omega) { 
    
    // Get my bearings
    int naux = calc_info_.nri; 

    double ** Temp1 = block_matrix(naux,naux);
    double ** Temp2 = block_matrix(naux,naux);
    double ** Temp3 = block_matrix(naux,naux);

    // ========= XC_A ========== //
    // Form S - X_0 S^-1 W
    C_DGEMM('N','N', naux, naux, naux, 1.0, X0_A_[0], naux, Jinv_[0], naux, \
        0.0, Temp1[0], naux);
    C_DGEMM('N','N', naux, naux, naux, 1.0, Temp1[0], naux, W_A_[0], naux, \
        0.0, Temp2[0], naux);
    for (ULI k = 0; k < naux*(ULI)naux; k++)
        Temp1[0][k] = S_[0][k] - Temp2[0][k];

    // Invert S - X_0 S^-1 W 
    // Cholesky factorization (possibly unstable)
    int stat = C_DPOTRF('U',naux,Temp1[0],naux);
    // Inverse 
    stat = C_DPOTRI('U',naux,Temp1[0],naux);

    // Form X_0 S^1 W (S - X_0 S^-1 W) ^ -1
    C_DGEMM('N','N', naux, naux, naux, 1.0, Temp2[0], naux, Temp2[0], naux, \
        0.0, Temp3[0], naux);
    
    // Form XC = X_0 S^1 W (S - X_0 S^-1 W) ^ -1 X_0
    C_DGEMM('N','N', naux, naux, naux, 1.0, Temp3[0], naux, X0_A_[0], naux, \
        0.0, XC_A_[0], naux);
    
    // ========= XC_B ========== //
    // Form S - X_0 S^-1 W
    C_DGEMM('N','N', naux, naux, naux, 1.0, X0_B_[0], naux, Jinv_[0], naux, \
        0.0, Temp1[0], naux);
    C_DGEMM('N','N', naux, naux, naux, 1.0, Temp1[0], naux, W_B_[0], naux, \
        0.0, Temp2[0], naux);
    for (ULI k = 0; k < naux*(ULI)naux; k++)
        Temp1[0][k] = S_[0][k] - Temp2[0][k];

    // Invert S - X_0 S^-1 W 
    // Cholesky factorization (possibly unstable)
    stat = C_DPOTRF('U',naux,Temp1[0],naux);
    // Inverse 
    stat = C_DPOTRI('U',naux,Temp1[0],naux);

    // Form X_0 S^1 W (S - X_0 S^-1 W) ^ -1
    C_DGEMM('N','N', naux, naux, naux, 1.0, Temp2[0], naux, Temp2[0], naux, \
        0.0, Temp3[0], naux);
    
    // Form XC = X_0 S^1 W (S - X_0 S^-1 W) ^ -1 X_0
    C_DGEMM('N','N', naux, naux, naux, 1.0, Temp3[0], naux, X0_A_[0], naux, \
        0.0, XC_B_[0], naux);
    
    free_block(Temp1);    
    free_block(Temp2);    
    free_block(Temp3);    
}
double SAPT_DFT::compute_UCHF_disp() {

    int naux = calc_info_.nri;

    double ** Temp1 = block_matrix(naux,naux);
    double ** Temp2 = block_matrix(naux,naux);

    // Form C_A
    C_DGEMM('N','N', naux, naux, naux, 1.0, Jinv_[0], naux, X0_A_[0], naux, \
        0.0, Temp1[0], naux);

    // Form C_B
    C_DGEMM('N','N', naux, naux, naux, 1.0, Jinv_[0], naux, X0_B_[0], naux, \
        0.0, Temp2[0], naux);

    double contribution = -1.0/(2.0*M_PI)*C_DDOT(naux*(ULI)naux, Temp1[0], 1, Temp2[0],1); 

    free_block(Temp1);    
    free_block(Temp2);    

    return contribution;
}
double SAPT_DFT::compute_TDDFT_disp() {

    int naux = calc_info_.nri;

    double ** Temp1 = block_matrix(naux,naux);
    double ** Temp2 = block_matrix(naux,naux);

    // Form C_A
    C_DGEMM('N','N', naux, naux, naux, 1.0, Jinv_[0], naux, XC_A_[0], naux, \
        0.0, Temp1[0], naux);

    // Form C_B
    C_DGEMM('N','N', naux, naux, naux, 1.0, Jinv_[0], naux, XC_B_[0], naux, \
        0.0, Temp2[0], naux);

    double contribution = -1.0/(2.0*M_PI)*C_DDOT(naux*(ULI)naux, Temp1[0], 1, Temp2[0],1); 

    free_block(Temp1);    
    free_block(Temp2);    

    return contribution;
}
OmegaQuadrature::OmegaQuadrature(int npoints) {

    npoints_ = npoints;
    index_ = 0;
    w_ = init_array(npoints);
    omega_ = init_array(npoints);

    // Compute Treutler-style mapping 
    double x,temp;
    double xi = 1.0; // By default
    double alpha = 0.6;
    double INVLN2 = 1.0/log(2.0);
    for (int tau = 1; tau<=npoints; tau++) {
    	//$x = \cos\left(\frac{\tau}{n_\tau+1}\pi\right)$
    	//$r = \frac{\xi}{\ln(2)}(1+x)^{\alpha}\ln\left(\frac{2.0}{1-x}\right)$
        x = cos(tau/(npoints+1.0)*M_PI);
    	omega_[tau-1] = xi*INVLN2*pow(1.0+x,alpha)*log(2.0/(1.0-x));
    	//$w = \frac{\pi}{n_tau+1.0}\sin^2\left(\frac{\tau}{n_\tau+1}\pi\right)$
    	//$w *= \frac{2\xi}{(1+x)^2}$ Accounts for change of variable
    	//$w *= \frac{1}{\sqrt{1-x^2}}$ Accounts for integral type
        temp = sin(tau/(npoints+1.0)*M_PI);
        w_[tau-1] = M_PI/(npoints+1.0)*temp*temp;
    	w_[tau-1] *= xi*INVLN2*(alpha*pow(1.0+x,alpha-1.0)*log(2.0/(1.0-x))+pow(1.0+x,alpha)/(1.0-x));
    	w_[tau-1] *= 1.0/sqrt(1.0-x*x);
    }
}
OmegaQuadrature::~OmegaQuadrature() {
    free(w_);
    free(omega_);
}


}}
