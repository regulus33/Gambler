# TODO

-----


# Step selection
Currently when you select a step, the step is white
but the active step (sequence index) is just a step
with a white border, neither of these states are clear.

# Step selection Bug?
Seems to be an occasional bug where note selection
is not being properly set, something with quant?

# CC modulation
The cc modulation panel, we need buttons:
1. enable all
2. select one
3. randomize selected

# navigation 
clicking a step in overview should make that step the active step

double clicking a step should make that step the current index

remove the follow playback

make current step white!

active step should be red or white border

make the steps and the piano keys color.lighten() on current step

{} have an option where when pressing one or more keys (or toggling)
other keys that would be in key light up, always factoring the whole selected group.

saveable keyboard settings, stack them all up as rectangles atop the piano

a setting that listens for any non mapped midi input and makes the scale quantized to that momentarily

If you click another row in seqeuncer overview that isn't active row in follow step mode you get pushed back to active...

Strange thing happening with scales, I'm moving the note range sliders and then seeing piano roll component change state.
Seems like a problem with global scale. We should probably get rid of it as well as the record button. 

# Standalone 

We haven't tested standalone much in master clock mode and we also haven't 
implemented slave mode for standalone.

----- 

