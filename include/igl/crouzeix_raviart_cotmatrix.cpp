// This file is part of libigl, a simple c++ geometry processing library.
// 
// Copyright (C) 2017 Alec Jacobson <alecjacobson@gmail.com>
// 
// This Source Code Form is subject to the terms of the Mozilla Public License 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.
#include "crouzeix_raviart_cotmatrix.h"
#include "unique_simplices.h"
#include "oriented_facets.h"
#include "is_edge_manifold.h"
#include "cotmatrix_entries.h"

template <typename DerivedV, typename DerivedF, typename LT, typename DerivedE, typename DerivedEMAP>
void igl::crouzeix_raviart_cotmatrix(
  const Eigen::MatrixBase<DerivedV> & V, 
  const Eigen::MatrixBase<DerivedF> & F, 
  Eigen::SparseMatrix<LT> & L,
  Eigen::PlainObjectBase<DerivedE> & E,
  Eigen::PlainObjectBase<DerivedEMAP> & EMAP)
{
  // All occurances of directed "facets"
  Eigen::MatrixXi allE;
  oriented_facets(F,allE);
  Eigen::VectorXi _1;
  unique_simplices(allE,E,_1,EMAP);
  return crouzeix_raviart_cotmatrix(V,F,E,EMAP,L);
}

template <typename DerivedV, typename DerivedF, typename DerivedE, typename DerivedEMAP, typename LT>
void igl::crouzeix_raviart_cotmatrix(
  const Eigen::MatrixBase<DerivedV> & V, 
  const Eigen::MatrixBase<DerivedF> & F, 
  const Eigen::MatrixBase<DerivedE> & E,
  const Eigen::MatrixBase<DerivedEMAP> & EMAP,
  Eigen::SparseMatrix<LT> & L)
{
  // number of rows
  const int m = F.rows();
  // Element simplex size
  const int ss = F.cols();
  // Mesh should be edge-manifold
  assert(F.cols() != 3 || is_edge_manifold(F));
  typedef Eigen::Matrix<LT,Eigen::Dynamic,Eigen::Dynamic> MatrixXS;
  MatrixXS C;
  cotmatrix_entries(V,F,C);
  Eigen::MatrixXi F2E(m,ss);
  {
    int k =0;
    for(int c = 0;c<ss;c++)
    {
      for(int f = 0;f<m;f++)
      {
        F2E(f,c) = k++;
      }
    }
  }
  // number of entries inserted per facet
  const int k = ss*(ss-1)*2;
  std::vector<Eigen::Triplet<LT> > LIJV;LIJV.reserve(k*m);
  Eigen::VectorXi LI(k),LJ(k),LV(k);
  // Compensation factor to match scales in matlab version
  double factor = 2.0;

  switch(ss)
  {
    default: assert(false && "unsupported simplex size");
    case 3:
      factor = 4.0;
      LI<<0,1,2,1,2,0,0,1,2,1,2,0;
      LJ<<1,2,0,0,1,2,0,1,2,1,2,0;
      LV<<2,0,1,2,0,1,2,0,1,2,0,1;
      break;
    case 4:
      factor *= -1.0;
      LI<<0,3,3,3,1,2,1,0,1,2,2,0,0,3,3,3,1,2,1,0,1,2,2,0;
      LJ<<1,0,1,2,2,0,0,3,3,3,1,2,0,3,3,3,1,2,1,0,1,2,2,0;
      LV<<2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1;
      break;
  }

  for(int f=0;f<m;f++)
  {
    for(int c = 0;c<k;c++)
    {
      LIJV.emplace_back(
        EMAP(F2E(f,LI(c))),
        EMAP(F2E(f,LJ(c))),
        (c<(k/2)?-1.:1.) * factor *C(f,LV(c)));
    }
  }
  L.resize(E.rows(),E.rows());
  L.setFromTriplets(LIJV.begin(),LIJV.end());
}
