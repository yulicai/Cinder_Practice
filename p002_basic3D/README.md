

<img src = "https://github.com/yulicai/Cinder_Practice/raw/master/images/depth_dance.gif" >

# What I leaned from it

 <br />
Cinder, C++
 <br />
Feb 23th 2017

####Basic 3D concept with Cinder in comparison with three.js
* Model Matrix
Model matrix is used to position an object in the world
* View Matrix
View matrix is used to position our virtual "eye" in the world
* Projection Matrix
<br />
<img src = "https://github.com/yulicai/Cinder_Practice/raw/master/images/camera_persp.svg" width = "320">

image credit: [mindofmatthew](http://content.mindofmatthew.com/how_3d_works/#just_the_code_please)
<br />
These three things are common concept when presenting 3D object in a 2D screen.
<br />
How 3D is projected onto the 2D image plane or screen. These include the aspect ratio, field-of-view and clipping planes.
<br />
In Cinder, it controls the view matrix by creating an camera object ( same in three.js ), but using the method CameraPersp::lookAt( vec3(point of view), vec3(where to look) )
<br />
By calling gl::setMatrices() and passing a CameraPersp, Cinder's OpenGL View and Projection matrices are set.
<pre><code>
CameraPersp cam;
cam.lookAt( vec3( 3, 3, 3 ), vec3( 0 ) );
gl::setMatrices( cam );
</code></pre>

<br />
In three.js, it is by initializing a camera object
<pre><code>
//PerspectiveCamera( fov, aspect, near, far )
var camera = new THREE.PerspectiveCamera( 45, width / height, 1, 1000 );
</code></pre>

####Depth Buffer(z-index artifacts)
Z-Buffering is the standard technique for preventing fragments from drawing on top of each other incorrectly. It uses a per-pixel depth value to ensure that no pixel draws on top of a pixel it is "behind."
<br />
In Cinder, we have to add manually add two lines to enable depth read and write, because by default it is false.
<br />
At the beginning of the draw loop
<pre><code>
gl::enableDepthRead();
gl::enableDepthWrite();
</code></pre>
But in three.js, it is associated with the [Material object](https://threejs.org/docs/api/materials/Material.html)
<br />
Using the prooerty ".depthWrite"
<br />
Whether rendering this material has any effect on the depth buffer. Default is true.
<br />
When drawing 2D overlays it can be useful to disable the depth writing in order to layer several things together without creating z-index artifacts.
