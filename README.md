# sand-pendulum
### A simple simulation of a lissajous-style biperiodic pendulum, with drag

Once someone took me into a new-agey store in a beach town and I saw a sand pendulum —
a weighted bob on a string which drew lines in a flattened area of fine sand.
The key element that made the patterns interesting is that the string's pivot point
for one axis was lower than for the other.
There are various ways to achieve this mechanically:
tie the string to a metal piece which can pivot on one axis,
use a triangular yoke of string hung from two points,
or (as in the case I saw) constrain the string with a pair of smooth rods.
The result is that the pendulum draws a class of shapes known as Lissajous figures,
but because of drag, they get a little smaller each time the pendulum goes around.

Seeing the interesting shapes that resulted, I promptly went home and wrote a mathematical simulation of the motion.
There are two parameters: the ratio between the pendulum's length (or period) on one axis vs the other axis,
and the amount of drag slowing the pendulum down.
Higher drag lets you see the individual lines more easily;
lower drag makes the lines seem more like a wireframe outline of an abstract 3D shape.

As for ratios, random numbers tend to create messy shapes,
but interesting and pleasing shapes can arise when the ratio is a simple rational number, like 3/1 or 4/3 or 6/5.
Sometimes it looks better if it's close to such a number, but slightly off from it.

To use the Windows exe, adjust the two numeric sliders (or edit the numbers directly),
then just click on the surface to set the starting position of the pendulum bob.
Clicking near a corner will give you the largest view.
It draws simple black lines to trace the path of the bob's tip, and stops after a fixed interval.
There's a Print button, but there's no capture-as-image button in this version.

The code was written quickly in the first GUI framework that came to hand, which at the time was Borland OWL.
In its day it was a better alternative to MFC for making Windows apps in C++.
If I were doing this now I'd use something more portable.

No license, do what you want with it.
