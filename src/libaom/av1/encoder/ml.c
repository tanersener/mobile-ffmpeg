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

#include <assert.h>
#include <math.h>

#include "aom_dsp/aom_dsp_common.h"
#include "av1/encoder/ml.h"

// Calculate prediction based on the given input features and neural net config.
// Assume there are no more than NN_MAX_NODES_PER_LAYER nodes in each hidden
// layer.
void av1_nn_predict_c(const float *input_nodes,
                      const NN_CONFIG *const nn_config, float *const output) {
  int num_input_nodes = nn_config->num_inputs;
  int buf_index = 0;
  float buf[2][NN_MAX_NODES_PER_LAYER];

  // Propagate hidden layers.
  const int num_layers = nn_config->num_hidden_layers;
  assert(num_layers <= NN_MAX_HIDDEN_LAYERS);
  for (int layer = 0; layer < num_layers; ++layer) {
    const float *layer_weights = nn_config->weights[layer];
    const float *layer_bias = nn_config->bias[layer];
    float *output_nodes = buf[buf_index];
    const int num_output_nodes = nn_config->num_hidden_nodes[layer];
    assert(num_output_nodes < NN_MAX_NODES_PER_LAYER);
    for (int node = 0; node < num_output_nodes; ++node) {
      float val = layer_bias[node];
      for (int i = 0; i < num_input_nodes; ++i)
        val += layer_weights[node * num_input_nodes + i] * input_nodes[i];
      // ReLU as activation function.
      val = val > 0.0f ? val : 0.0f;  // Could use AOMMAX().
      output_nodes[node] = val;
    }
    num_input_nodes = num_output_nodes;
    input_nodes = output_nodes;
    buf_index = 1 - buf_index;
  }

  // Final output layer.
  const float *layer_weights = nn_config->weights[num_layers];
  const float *layer_bias = nn_config->bias[num_layers];
  for (int node = 0; node < nn_config->num_outputs; ++node) {
    float val = layer_bias[node];
    for (int i = 0; i < num_input_nodes; ++i)
      val += layer_weights[node * num_input_nodes + i] * input_nodes[i];
    output[node] = val;
  }
}

void av1_nn_softmax(const float *input, float *output, int n) {
  // Softmax function is invariant to adding the same constant
  // to all input values, so we subtract the maximum input to avoid
  // possible overflow.
  float max_inp = input[0];
  for (int i = 1; i < n; i++) max_inp = AOMMAX(max_inp, input[i]);
  float sum_out = 0.0f;
  for (int i = 0; i < n; i++) {
    output[i] = (float)exp(input[i] - max_inp);
    sum_out += output[i];
  }
  for (int i = 0; i < n; i++) output[i] /= sum_out;
}
