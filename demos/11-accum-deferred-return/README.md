# 11 · The deferred accumulation buffer

PR11 keeps a batch that starts with `GL_LOAD` as snapshots, summed in one
shader pass at `GL_RETURN`, instead of blending every pass into the
half-float buffer. A is PR10's tip. The scene is demo 10's.

In Chrome on an M2, B and A both differ from Apple's GL by 0, 1, 0 and
813 px per panel. B differs from A only in the two panels that start with
`GL_LOAD` (JITTER AA 8,366 px, SOFT SHADOW 20,090 px), by at most 1 level of
255. MOTION BLUR and TRAIL start with a clear, take the old path, and are
identical. The page measures B about 8% faster (0.53 ms against 0.58).
