#ifndef CTC_BEAM_SEARCH_DECODER_H_
#define CTC_BEAM_SEARCH_DECODER_H_

#include <string>
#include <vector>

#include "LM.h"
#include "output.h"
#include "alphabet.h"
#include "decoder_state.h"

/* Initialize CTC beam search decoder

 * Parameters:
 *     alphabet: The alphabet.
 *     class_dim: Alphabet length (plus 1 for space character).
 *     lm: External scorer to evaluate a prefix, which consists of
 *                 n-gram language model scoring and word insertion term.
 *                 Default null, decoding the input sample without scorer.
 * Return:
 *     A struct containing prefixes and state variables.
*/
DecoderState *decoder_init(int blank_id,
                           int class_dim,
                           const LMPtr &lm);

/* Send data to the decoder

 * Parameters:
 *     probs: 2-D vector where each element is a vector of probabilities
 *               over alphabet of one time step.
 *     alphabet: The alphabet.
 *     state: The state structure previously obtained from decoder_init().
 *     time_dim: Number of timesteps.
 *     class_dim: Alphabet length (plus 1 for space character).
 *     cutoff_prob: Cutoff probability for pruning.
 *     cutoff_top_n: Cutoff number for pruning.
 *     beam_size: The width of beam search.
 *     lm: External scorer to evaluate a prefix, which consists of
 *                 n-gram language model scoring and word insertion term.
 *                 Default null, decoding the input sample without scorer.
*/
void decoder_next(const float *log_probs,
                  DecoderState *state,
                  int time_dim,
                  int class_dim,
                  double cutoff_prob,
                  size_t cutoff_top_n,
                  size_t beam_size,
                  const LMPtr &lm);

/* Get transcription for the data you sent via decoder_next()

 * Parameters:
 *     state: The state structure previously obtained from decoder_init().
 *     alphabet: The alphabet.
 *     beam_size: The width of beam search.
 *     lm: External scorer to evaluate a prefix, which consists of
 *                 n-gram language model scoring and word insertion term.
 *                 Default null, decoding the input sample without scorer.
 * Return:
 *     A vector where each element is a pair of score and decoding result,
 *     in descending order.
*/
std::vector<Output> decoder_decode(DecoderState *state,
                                   size_t beam_size,
                                   LMPtr &lm);

/* CTC Beam Search Decoder
 * Parameters:
 *     probs: 2-D vector where each element is a vector of probabilities
 *            over alphabet of one time step.
 *     time_dim: Number of timesteps.
 *     class_dim: Alphabet length (plus 1 for blank character).
 *     beam_size: The width of beam search.
 *     cutoff_prob: Cutoff probability for pruning.
 *     cutoff_top_n: Cutoff number for pruning.
 *     lm: External scorer to evaluate a prefix, which consists of
 *                 n-gram language model scoring and word insertion term.
 *                 Default null, decoding the input sample without scorer.
 * Return:
 *     A vector where each element is a pair of score and decoding result,
 *     in descending order.
*/

std::vector<Output> ctc_beam_search_decoder(
    const float *log_probs,
    int time_dim,
    int class_dim,
    int blank_id,
    size_t beam_size,
    double cutoff_prob,
    size_t cutoff_top_n,
    const LMPtr &lm);

/* CTC Beam Search Decoder for batch data
 * Parameters:
 *     log_probs: 3-D vector where each element is a 2-D vector that can be used
 *                by ctc_beam_search_decoder().
 *     beam_size: The width of beam search.
 *     num_processes: Number of threads for beam search.
 *     cutoff_prob: Cutoff probability for pruning.
 *     cutoff_top_n: Cutoff number for pruning.
 *     lm: External scorer to evaluate a prefix, which consists of
 *                 n-gram language model scoring and word insertion term.
 *                 Default null, decoding the input sample without scorer.
 * Return:
 *     A 2-D vector where each element is a vector of beam search decoding
 *     result for one audio sample.
*/
std::vector<std::vector<Output>>
ctc_beam_search_decoder_batch(
    const float *log_probs,
    int batch_size,
    int time_dim,
    int class_dim,
    const int *seq_lengths,
    int seq_lengths_size,
    int blank_id,
    size_t beam_size,
    size_t num_processes,
    double cutoff_prob,
    size_t cutoff_top_n,
    const LMPtr &lm);

#endif // CTC_BEAM_SEARCH_DECODER_H_
// CTC_BEAM_SEARCH_DECODER_H_
// CTC_BEAM_SEARCH_DECODER_H_
