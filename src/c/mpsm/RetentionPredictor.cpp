#include "RetentionPredictor.h"
#include "PalmbaldRetentionPredictor.h"
#include "KrokhinRetentionPredictor.h"
#include "AKlammerRetentionPredictor.h"
#include "AKlammerStaticRetentionPredictor.h"
#include "NullRetentionPredictor.h"

#include "parameter.h"


using namespace std;

RetentionPredictor::RetentionPredictor() {
}

RetentionPredictor::~RetentionPredictor() {
}


FLOAT_T RetentionPredictor::predictRTime(Match* match) {
  /*override this function*/
  return 0.0;
}

double RetentionPredictor::calcMaxDiff(const MPSM_Match& mpsm_match) {
  if (mpsm_match.numMatches() <= 1) return 0.0;

  vector<FLOAT_T> rtimes;

  for (int match_idx=0;
    match_idx < mpsm_match.numMatches();
    match_idx++) {
      rtimes.push_back(predictRTime(mpsm_match.getMatch(match_idx)));
    }
  

  double max_diff = 0;
  
  for (int idx1 = 0;idx1 < rtimes.size()-1;idx1++) {
    for (int idx2 = idx1 + 1;idx2 < rtimes.size();idx2++) {

      double current_diff = rtimes[idx2] - rtimes[idx1];

      if (fabs(current_diff) > fabs(max_diff)) {
        max_diff = current_diff;
      }
    }
  }

  return max_diff;

    
}


RetentionPredictor* RetentionPredictor::predictor_ = NULL;

void RetentionPredictor::createRetentionPredictor() {

  if (predictor_) {
    delete predictor_;
  } else {

    RTP_TYPE_T rtp_type = get_rtp_type_parameter("rtime-predictor");

    switch(rtp_type) {
      case RTP_KROKHIN:
        carp(CARP_DEBUG,"creating krokhin retention predictor");
        predictor_ = new KrokhinRetentionPredictor();
        break;
      case RTP_PALMBALD:
        carp(CARP_DEBUG,"creating palmbald retention predictor");
        predictor_ = new PalmbaldRetentionPredictor();
        break;
      case RTP_AKLAMMER:
      carp(CARP_DEBUG,"creating aklammer retention predictor");
      predictor_ = new AKlammerStaticRetentionPredictor();
      break;
      case RTP_INVALID:
      default:
        carp(CARP_WARNING,"Invalid retention time predictor: returning null");
        predictor_ = new NullRetentionPredictor();
        
    }
  }
}

RetentionPredictor* RetentionPredictor::getStaticRetentionPredictor() {
  if (predictor_ == NULL) {
    carp(CARP_WARNING, "Retention predictor not created!");
    createRetentionPredictor();
  }
  return predictor_;
}
