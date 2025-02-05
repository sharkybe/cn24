/*
 * This file is part of the CN24 semantic segmentation software,
 * copyright (C) 2015 Clemens-Alexander Brust (ikosa dot de at gmail dot com).
 *
 * For licensing information, see the LICENSE file included with this project.
 */  

#include "Log.h"
#include "GradientTester.h"

namespace Conv {
  
void GradientTester::TestGradient ( NetGraph& graph, unsigned int skip_weights, bool fatal_fail ) {
  const double epsilon = 0.005;
  LOGDEBUG << "Testing gradient. FeedForward...";
	graph.FeedForward();
  LOGDEBUG << "Testing gradient. BackPropagate...";
  graph.BackPropagate();
  
	const datum initial_loss = graph.AggregateLoss();
	unsigned int global_okay = 0;
	unsigned int global_tolerable = 0;
	unsigned int global_failed = 0;
	unsigned int global_weights = 0;

  LOGDEBUG << "Initial loss: " << initial_loss;
  LOGDEBUG << "Using epsilon: " << epsilon;
  for(unsigned int l = 0; l < graph.GetNodes().size(); l++) {
		NetGraphNode* node = graph.GetNodes()[l];
		Layer* layer = node->layer;
    for(unsigned int p = 0; p < layer->parameters().size(); p++) {
			CombinedTensor* const param = layer->parameters()[p];
      LOGDEBUG << "Testing layer " << l << " (" << layer->GetLayerDescription() << "), parameter set " << p;
      LOGDEBUG << param->data;
      bool passed = true;
      unsigned int okay = 0;
      unsigned int tolerable = 0;
      unsigned int failed = 0;
      unsigned int total = 0;
      for(unsigned int e = 0; e < param->data.elements(); e+=(skip_weights + 1))
      {
        total++;
#ifdef BUILD_OPENCL
	param->data.MoveToCPU();
	param->delta.MoveToCPU();
#endif
	const datum old_param = param->data(e);
	
	param->data[e] = old_param + epsilon;
	graph.FeedForward();
	const double plus_loss = graph.AggregateLoss();
	
#ifdef BUILD_OPENCL
	param->data.MoveToCPU();
#endif
	param->data[e] = old_param - epsilon;
graph.FeedForward();
	const double minus_loss = graph.AggregateLoss();
	
	const double delta = param->delta[e];
	const double actual_delta = (plus_loss - minus_loss) / (2.0 * epsilon);
	
	const double ratio = actual_delta / delta;
	if(ratio > 1.02 || ratio < 0.98) {
	  if(ratio > 1.2 || ratio < 0.8) {
	    if(passed)
	      LOGWARN << "delta analytic: " << delta << ", numeric: " << actual_delta << ", ratio: " << ratio;
	    passed = false;
	    // std::cout << "!" << std::flush;
			failed++;
	  } else {
	  // std::cout << "#" << std::flush;
		tolerable++;
	  }
	}
	else {
	  // std::cout << "." << std::flush;
	  okay++;
	}
#ifdef BUILD_OPENCL
	param->data.MoveToCPU();
#endif
	param->data[e] = old_param;
      }
      // std::cout << "\n";
      if(passed) {
	LOGDEBUG << "Okay!";
      } else {
	LOGERROR << "Failed!";
      }
			LOGDEBUG << okay << " of " << total << " gradients okay (delta < 2%)";
			LOGDEBUG << tolerable << " of " << total << " gradients tolerable (delta < 20%)";
			LOGDEBUG << failed << " of " << total << " gradients failed (delta >= 20%)";
			global_okay += okay;
			global_tolerable += tolerable;
			global_failed += failed;
			global_weights += total;
    }
  }

	LOGRESULT << global_okay << " of " << global_weights << " tested gradients okay (delta < 2%)" << LOGRESULTEND;
	LOGRESULT << global_tolerable << " of " << global_weights << " tested gradients tolerable (delta < 20%)" << LOGRESULTEND;
	LOGRESULT << global_failed << " of " << global_weights << " tested gradients failed (delta >= 20%)" << LOGRESULTEND;
  
  if (global_failed > 0 && fatal_fail) {
    FATAL("Failed gradient check!");
  }

}

  
}
