# Algorithm repair notes

This version repairs the algorithm implementation to match the PDF experiment protocol:

1. HotspotOnlyParity is still weakness-prioritized, but it now has hard coverage, node-use, block-use, pair-reuse and duplicate-pattern controls. This prevents the previous pathological concentration where many information variables were never protected.
2. ExtParityV2 is implemented as a weakness-coverage-reuse balanced construction, with stronger coverage and pair-reuse penalties.
3. AdaptiveStructV2 performs static structural selection between HotspotOnlyParity and ExtParityV2. It uses base-graph diagnostics plus candidate construction diagnostics, not short BER simulation.
4. FER is split into FER_info and FER_code while keeping the legacy FER column as FER_info.
5. AdaptiveSelectV8 reports its short-simulation overhead through selection_short_frames and selection_extra_decodes.
6. The threshold scan range is extended to 4.8 dB so that BER_info < 1e-3 crossing is more likely to be observable.
