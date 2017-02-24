#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"




using namespace ci;
using namespace ci::app;
using namespace std;

class Basic_3D_d002App : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
};

void Basic_3D_d002App::setup()
{
}



void Basic_3D_d002App::update()
{
}

void Basic_3D_d002App::draw()
{
	gl::clear( Color( 1 , 1, 1 ) );
    
    //turn on z-buffering(depth buffer)
    gl::enableDepthRead();
    gl::enableDepthWrite();
    
    CameraPersp cam;
    cam.lookAt(vec3(0,7,0),vec3(0,1,0));
    gl::setMatrices(cam);
    
    //create a shader(bring a built-in shader from Cinder
    auto lambert = gl::ShaderDef().lambert().color();
    //in order to make the shader we create above the active shader, we use getStockShader()
    auto active_shader = gl::getStockShader(lambert);
    //and then bind the result
    active_shader->bind();
    
    int num_spheres = 512;
    float max_angle = M_PI * 21;
    float spiral_radius = 2;
    float height = 2;
    //dividing by 30.0f is a lot different than dividing by 30
    float anim = getElapsedFrames() / 30.0f;
    
    for (int s = 0; s < num_spheres; ++s){
        float rel = s / (float)num_spheres;
        float angle = rel * max_angle;
        float y = rel * height * sin(anim * (s+10) / 30.0f);
        float r = rel * spiral_radius * spiral_radius;
        vec3 offset = vec3(r * cos(angle), y, r * sin(angle));
        
        gl::pushModelMatrix();
        gl::translate(offset);
        gl::color(Color(rel+.4,rel+.4,rel+.4));
        //draw the sphere at origin, radius one, subdivisions to 40 (default is less then 40)
        gl::drawSphere(vec3(), 0.1f,10);
        gl::popModelMatrix();
        
    }
    
   
}

CINDER_APP( Basic_3D_d002App, RendererGl )






