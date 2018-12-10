/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AV1_ENCODER_ML_H_
#define AOM_AV1_ENCODER_ML_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config/av1_rtcd.h"

#define NN_MAX_HIDDEN_LAYERS 10
#define NN_MAX_NODES_PER_LAYER 128

struct NN_CONFIG {
  int num_inputs;         // Number of input nodes, i.e. features.
  int num_outputs;        // Number of output nodes.
  int num_hidden_layers;  // Number of hidden layers, maximum 10.
  // Number of nodes for each hidden layer.
  int num_hidden_nodes[NN_MAX_HIDDEN_LAYERS];
  // Weight parameters, indexed by layer.
  const float *weights[NN_MAX_HIDDEN_LAYERS + 1];
  // Bias parameters, indexed by layer.
  const float *bias[NN_MAX_HIDDEN_LAYERS + 1];
};
// Typedef from struct NN_CONFIG to NN_CONFIG is in rtcd_defs

// Applies the softmax normalization function to the input
// to get a valid probability distribution in the output:
// output[i] = exp(input[i]) / sum_{k \in [0,n)}(exp(input[k]))
void av1_nn_softmax(const float *input, float *output, int n);

// Applies the softmax normalization function to the input
// to get a valid probability distribution in the output:
// output[i] = exp(input[i]) / sum_{k \in [0,n)}(exp(input[k]))
void av1_nn_softmax(const float *input, float *output, int n);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_ML_H_
