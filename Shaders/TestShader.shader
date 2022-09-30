Stupid slop outside of a valid block
that shouldn't be included
in the output of the function.

Again, none of this stuff
before the first #DEFINE should appear. Also, a #DEFINE should
only appear at the start of the line. The rest of the line
will be used as the section name. 

In the end, this shader should load in as six sections, NOT seven.

#DEFINE VERTEX_SHADER
This is a dumb
test of the
vertex shader

#DEFINE FRAGMENT_SHADER
Now here comes a 
dumb test of
the fragment shader

#DEFINE OTHER_CONTENT
Here's an additional block, in case I want
to support other shader types
such as geometry shaders.

#DEFINE EMPTY_SECTION_1
#DEFINE EMPTY_SECTION_2
#DEFINE EMPTY_SECTION_3