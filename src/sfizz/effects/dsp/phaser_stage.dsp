/*
Dual-notch Phaser, with linear cutoff control by LFO

Note:
  `phaser_depth`: manipulates the 2 ranges of cutoff for the notches
  `phaser_feedback`: TODO, implemented in model below but not measured
  `phaser_phase[_oncc]`: TODO, assumed to be the phase 0-1 of LFO?
*/

import("stdfaust.lib");

phaser(lfo, phaser_depth, phaser_feedback) =
  (+:notch1:notch2) ~ *(feedback)
with {
  // TODO determine the relation according to `phaser_feedback`
  feedback = phaser_feedback : *(0.01) : min(0.999);

  cutoffDepth1 = 31*phaser_depth;
  cutoffMax1 = 3100.0;
  cutoffCenter1 = 1600.0;

  cutoffDepth2 = 155*phaser_depth;
  cutoffMax2 = 13400.0;
  cutoffCenter2 = 8300.0;

  // TODO adjustment of filter bandwidth
  widthAdjust = 0.5;

  notchWidth1 = widthAdjust*cutoffDepth1;
  notchWidth2 = widthAdjust*cutoffDepth2;

  notchFreq1 = max(0,min(cutoffMax1,cutoffCenter1+lfo*cutoffDepth1));
  notchFreq2 = max(0,min(cutoffMax2,cutoffCenter2+lfo*cutoffDepth2));

  // filter pick arbitrary, could use also 1-pole allpass
  notch1 = fi.notchw(notchWidth1, notchFreq1);
  notch2 = fi.notchw(notchWidth2, notchFreq2);
};

process(lfo, depth, feedback) = phaser(lfo, depth, feedback);
