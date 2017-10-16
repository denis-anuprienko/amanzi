/*
  Functions

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include "DistanceFunction.hh"
#include "errors.hh"
#include <cmath>

namespace Amanzi {

DistanceFunction::DistanceFunction(const std::vector<double>& x0, const std::vector<double>& metric)
{
  if (x0.size() != metric.size()) {
    Errors::Message m;
    m << "Mismatch of metric and point dimensions.";
    Exceptions::amanzi_throw(m);
  }
  x0_ = x0;
  metric_ = metric;
}


double DistanceFunction::operator()(const std::vector<double>& x) const
{
  double tmp(0.0), y(0.0);
  if (x.size() < x0_.size()) {
    Errors::Message m;
    m << "DistanceFunction expects higher-dimensional argument.";
    Exceptions::amanzi_throw(m);
  }    
  for (int j = 0; j < x0_.size(); ++j) {
    tmp = x[j] - x0_[j];
    y += metric_[j] * tmp * tmp;
  }
  y = std::sqrt(y);

  return y;
}

}  // namespace Amanzi
