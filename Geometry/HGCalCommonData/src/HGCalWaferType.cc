#include "Geometry/HGCalCommonData/interface/HGCalWaferType.h"
#include "Geometry/HGCalCommonData/interface/HGCalParameters.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include<cmath>

//#define EDM_ML_DEBUG

HGCalWaferType::HGCalWaferType(const std::vector<double>& rad100, 
			       const std::vector<double>& rad200,
			       double waferSize, double zMin, 
			       int choice, unsigned int cornerCut, 
			       double cutArea) : rad100_(rad100),
						 rad200_(rad200),
						 waferSize_(waferSize),
						 zMin_(zMin), choice_(choice),
						 cutValue_(cornerCut),
						 cutFracArea_(cutArea) {
  r_ = 0.5*waferSize_;
  R_ = sqrt3_*waferSize_;
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalWaferType: initialized with waferR's "
				<< waferSize_ << ":" << r_ << ":" << R_
				<< " Choice " << choice_ << " Cuts " 
				<< cutValue_ << ":" << cutFracArea_ << " zMin "
				<< zMin_ << " with " << rad100_.size() << ":"
				<< rad200_.size() << " parameters for R:";
  for (unsigned k=0; k<rad100_.size(); ++k) 
    edm::LogVerbatim("HGCalGeom") << "[" << k << "] 100:200 " << rad100_[k]
				  << " 200:300 " << rad200_[k];
#endif
}

HGCalWaferType::~HGCalWaferType() { }

int HGCalWaferType::getType(double xpos, double ypos, double zpos) {
  std::vector<double> xc(HGCalParameters::k_CornerSize,0);
  std::vector<double> yc(HGCalParameters::k_CornerSize,0);
  xc[0] = xpos+r_;  yc[0] = ypos+0.5*R_;
  xc[1] = xpos;     yc[1] = ypos+R_;
  xc[2] = xpos-r_;  yc[2] = ypos+0.5*R_;
  xc[3] = xpos-r_;  yc[3] = ypos-0.5*R_;
  xc[4] = xpos;     yc[4] = ypos-R_;
  xc[5] = xpos+r_;  yc[5] = ypos-0.5*R_;
  std::pair<double,double> rv = rLimits(zpos);
  std::vector<int>         fine, coarse;
  for (unsigned int k=0; k<HGCalParameters::k_CornerSize; ++k) {
    double rpos = std::sqrt(xc[k]*xc[k]+yc[k]*yc[k]);
    if      (rpos <= rv.first)  fine.push_back(k);
    else if (rpos <= rv.second) coarse.push_back(k);
  }
  int    type(-2);
  double fracArea(0);
  if (choice_ == 1) {
    if      (fine.size()   >= cutValue_) type = 0;
    else if (coarse.size() >= cutValue_) type = 1;
    else                                 type = 2;
  } else {
    if      (fine.size()   >= 4)                     type = 0;
    else if (coarse.size() >= 4 && fine.size() <= 1) type = 1;
    else if (coarse.size() <  2 && fine.empty())     type = 2;
    else if (!fine.empty())                          type =-1;
    if (type < 0) {
      unsigned int kmax = (type == -1) ? fine.size() : coarse.size();
      std::vector<double> xcn, ycn;
      for (unsigned int k=0; k<kmax; ++k) {
	unsigned int k1 = (type == -1) ? fine[k] : coarse[k];
	unsigned int k2 = (k1 == xc.size()-1) ? 0 : k1+1;
	bool ok = ((type == -1) ?
		   (std::find(fine.begin(),fine.end(),k2) != fine.end()) :
		   (std::find(coarse.begin(),coarse.end(),k2) != coarse.end()));
	if (!ok) {
	  xcn.emplace_back(xc[k1]); ycn.emplace_back(yc[k1]);
	  double rr = (type == -1) ? rv.first : rv.second;
	  std::pair<double,double> xy = intersection(k1, k2, xc, yc, xpos, 
						     ypos, rr);
	  xcn.emplace_back(xy.first); ycn.emplace_back(xy.second);
	} else {
	  xcn.emplace_back(xc[k1]); ycn.emplace_back(yc[k1]);
	}
      }
      fracArea = areaPolygon(xcn,ycn)/areaPolygon(xc,yc);
      type     = (fracArea > cutFracArea_) ?  -(1+type) : -type;
    }
  }
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalWaferType: position " << xpos << ":"
				<< ypos << ":" << zpos << " R " << ":" 
				<< rv.first << ":" << rv.second
				<< " corners|type " << fine.size() << ":" 
				<< coarse.size() << ":" << fracArea
				<< ":" << type ;
#endif
  return type;
}

std::pair<double,double> HGCalWaferType::rLimits(double zpos) {
  double zz = std::abs(zpos);
  if (zz < zMin_) zz = zMin_;
  zz *= HGCalParameters::k_ScaleFromDDD;
  double rfine   = rad100_[0];
  double rcoarse = rad200_[0];
  for (int i=1; i<5; ++i) {
    rfine   *= zz; rfine   += rad100_[i];
    rcoarse *= zz; rcoarse += rad200_[i];
  }
  return std::pair<double,double>(rfine*HGCalParameters::k_ScaleToDDD,rcoarse*HGCalParameters::k_ScaleToDDD);
}

double HGCalWaferType::areaPolygon(std::vector<double> const& x,
				   std::vector<double> const& y) {
  double area = 0.0;
  for (unsigned int k1=0; k1<x.size(); ++k1) {
    unsigned int k2 = (k1 == x.size()-1) ? 0 : k1+1;
    area += 0.5*(x[k1]*y[k2]-x[k2]*y[k1]);
  }
  return area;
}

std::pair<double,double> HGCalWaferType::intersection(int k1, int k2, 
						      std::vector<double> const& x,
						      std::vector<double> const& y,
						      double xpos, double ypos,
						      double rr) {
  double slope  = (x[k1]-x[k2])/(y[k1]-y[k2]);
  double interc = x[k1]-slope*y[k1];
  double xx[2], yy[2], dist[2];
  double v1     = std::sqrt((slope*slope+1)*rr*rr-(interc*interc));
  yy[0]         = (-slope*interc+v1)/(1+slope*slope);
  yy[1]         = (-slope*interc-v1)/(1+slope*slope);
  for (int i=0; i<2; ++i) {
    xx[i]   = (slope*yy[i] + interc);
    dist[i] = ((xx[i]-xpos)*(xx[i]-xpos)) + ((yy[i]-ypos)*(yy[i]-ypos));
  }
  if (dist[0] > dist[1]) return std::pair<double,double>(xx[1],yy[1]);
  else                   return std::pair<double,double>(xx[0],yy[0]);
}
