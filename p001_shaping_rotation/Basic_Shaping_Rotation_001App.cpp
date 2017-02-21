#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Basic_Shape_001App : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
};

void Basic_Shape_001App::setup()
{
    //    setWindowSize(600, 600);
}


void Basic_Shape_001App::update()
{
}

void Basic_Shape_001App::draw()
{
    
    gl::clear( Color( 1, 1 , 1 ) );
    gl::pushModelMatrix();
    vec2 center = getWindowCenter();
    gl::translate(center);
    
    int num_circles = 16;
    float big_radius = getWindowHeight()/2 - 50;
    
    //returns the number of animation frames since the app has launched
    float anim = getElapsedFrames() / 30.f;
    
    for(int c = 0; c < num_circles; ++c){
        float rel_gap = c /(float) num_circles;
        float angle = rel_gap * M_PI * 2* sin(anim);
        vec2 offset(cos(angle),sin(angle));
        
        gl::pushModelMatrix();
        gl::translate(offset * big_radius * abs(sin(anim)));
        gl::color(Color(rel_gap * 0.5,.5,.53));
        //        gl::color( Color( CM_HSV, rel_gap, 1, 1 ) );
        gl::drawSolidCircle(vec2(0,0), 5);
        
        
        gl::popModelMatrix();
        
    }
    float anim_r = abs(sin(anim)*.5);
    gl::color(anim_r,.5,.53);
    gl::drawSolidEllipse(vec2(),60.,1.*sin(anim));
    
    gl::popModelMatrix();
}

CINDER_APP( Basic_Shape_001App, RendererGl )








