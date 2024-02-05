Meshes and blendshapes taken from Faceware Technologies

My vertex shader only supports 3 separate blendshapes loaded in at a time per emotion.


Incompatible Blendshapes
When I ran the animation with both the Victor_smile.obj and Victor_puff.obj, the animation looked very unnatural. It looked to me that the bottom half of his face was growing. A part of the reason it looked unnatural might be because of the sinusoidal weight functions making the movement too even, but the resulting face also looked off as the shape of the cheeks look unnatural. When I try to smile while having my cheecks puffed, I have more air in the bottom of my cheecks which the blended shapes lack. The model's face gets turned into a sharp V shape. I think because each blendshape is separately adding transformations to push the cheeks up the resulting face has double the amount of tranformation and the cheeks are raised to an unnatural degree.

To see the incompatible blend shapes haves these two lines uncommented in input.txt and all other DELTA and EMOTION lines commented
DELTA 1 Victor_headGEO.obj Victor_smile.obj
DELTA 3 Victor_headGEO.obj Victor_puff.obj


When multiple Emotion lines are uncommented at a time, press 's' to switch between them.

To my knowledge I completed all parts of the assignment, excluding the extra credit.