#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;
using namespace std;

struct Particle
{
	vec3 		home;
	vec3 		ppos;
	vec3		pos;
	vec3		mVel;
	vec3		mAcc;
	ColorA		color;
	float		damping;
};

// Home many particles to create. (200k)
const int NUM_PARTICLES = 500;

class Particle_turbulanceApp : public App {
public:
	void setup() override;
	void update() override;

	void draw() override;
	gl::TextureRef mTexture;
	float time;

	// Push particles away from a point.
	void disturbParticles(const vec3 &center, float force);

private:
	gl::GlslProgRef		mGlsl;
	// Particle data on CPU.
	vector<Particle>	mParticles;
	// Buffer holding raw particle data on GPU, written to every update().
	gl::VboRef			mParticleVbo;
	// Batch for rendering particles with default shader.
	gl::BatchRef		mParticleBatch;
};

void Particle_turbulanceApp::setup()
{
	setWindowSize(1080, 480); //testing ratio
	mTexture = gl::Texture::create(loadImage(loadAsset("summerSmall.jpg")));

	// Create initial particle layout.
	mParticles.assign(NUM_PARTICLES, Particle());
	const float azimuth = 128.0f * M_PI / mParticles.size();
	const float inclination = M_PI / mParticles.size();
	const float radius = 80.0f;
	vec3 center = vec3(getWindowCenter(), 0.0f);
	for (int i = 0; i < mParticles.size(); ++i)
	{	// assign starting values to particles.
		float x = radius * sin(inclination * i) * cos(azimuth * i);
		float y = radius * cos(inclination * i)/8.0;
		//float z = radius * sin(inclination * i) * sin(azimuth * i);
		float z = 0.0f;

		auto &p = mParticles.at(i);
		p.pos = center + vec3(x, y, z);
		p.pos.y -= 50.0f;
		//p.pos.z += 100.0f;
		p.home = p.pos;
		//p.mVel = Rand::randVec3()*10.0f;
		p.mVel = vec3(Rand::randVec2()*10.0f, 0.0f);
		p.mAcc = vec3(0.0f);
		p.ppos = p.home + p.mVel; // random initial velocity
		p.damping = Rand::randFloat(0.965f, 0.985f);
		p.color = Color(CM_HSV, lmap<float>(i, 0.0f, mParticles.size(), 0.1f, 0.2f), 1.0f, lmap<float>(i, 0.0f, mParticles.size(), 0.4f, 0.68f));
	}

	// Create particle buffer on GPU and copy over data.
	// Mark as streaming, since we will copy new data every frame.
	mParticleVbo = gl::Vbo::create(GL_ARRAY_BUFFER, mParticles, GL_STREAM_DRAW);

	// Describe particle semantics for GPU.
	geom::BufferLayout particleLayout;
	particleLayout.append(geom::Attrib::POSITION, 3, sizeof(Particle), offsetof(Particle, pos));
	particleLayout.append(geom::Attrib::COLOR, 4, sizeof(Particle), offsetof(Particle, color));

	// Create a customized shader
	 mGlsl = gl::GlslProg::create(gl::GlslProg::Format()
		.vertex(CI_GLSL(150,
			uniform mat4	ciModelViewProjection;
	in vec4			ciPosition;
	in vec4			ciColor;
	out vec4		vColor;

	void main(void) {
		gl_Position = ciModelViewProjection * ciPosition;
		vColor = ciColor;
		//glEnable(GL_POINT_SPRITE);
		//glEnable(GL_PROGRAM_POINT_SIZE);
		//vec3 vertex = vec3(ciModelViewProjection * gl_Vertex);
		float distance = length(gl_Position);
		gl_PointSize = mix(10.0, 100.0, gl_Position.x);
	}
	)).fragment(CI_GLSL(150,
		out vec4			oColor;
	in vec4				vColor;
	void main(void) {
		oColor = vColor;
	}
	)));


	// Create mesh by pairing our particle layout with our particle Vbo.
	// A VboMesh is an array of layout + vbo pairs
	auto mesh = gl::VboMesh::create(mParticles.size(), GL_POINTS, { { particleLayout, mParticleVbo } });
	mParticleBatch = gl::Batch::create(mesh, mGlsl);
	//mParticleBatch = gl::Batch::create(mesh, gl::getStockShader(gl::ShaderDef().color()));
	gl::begin(GL_POINTS);
	gl::pointSize(4);


	getWindow()->getSignalMouseDown().connect([this](MouseEvent event) {
		vec3 mouse(event.getPos(), 0.0f);
		disturbParticles(mouse, 20.0f);
	});
	// Disturb particle a little on mouse drag.
	getWindow()->getSignalMouseDrag().connect([this](MouseEvent event) {
		vec3 mouse(event.getPos(), 0.0f);
		disturbParticles(mouse, 20.0f);
	});

}



void Particle_turbulanceApp::disturbParticles(const vec3 &center, float force)
{
	for (auto &p : mParticles) {
		vec3 dir = center - p.pos;
		float l = length(dir);
		if (l < 80.0f) {
			vec3 steer = dir - p.mVel;
			steer = normalize(steer);
			steer *= 0.05f;
			p.mAcc += steer;
		}
		float d2 = length2(dir);
		p.pos += force * dir / d2;
	}
}

void Particle_turbulanceApp::update()
{
	time = getElapsedSeconds();
	// Run Verlet integration on all particles on the CPU.
	float dt2 =1.0f / (100.0f * 100.0f);

	const float azimuth = 128.0f * M_PI / mParticles.size();
	const float inclination = M_PI / mParticles.size();
	const float radius = 160.0f;
	vec3 center = vec3(getWindowCenter(), 0.0f);
	for (int i = 0; i < mParticles.size(); ++i)
	{	// assign starting values to particles.
		float x = radius * sin(inclination * i) * cos(azimuth * i);
		float y = radius * cos(inclination * i) / 2.0;
		//float z = radius * sin(inclination * i) * sin(azimuth * i);
		float z = 0.0f;
		auto &p = mParticles.at(i);
		vec3 vel = (p.pos - p.ppos) * p.damping + p.mAcc;
		p.ppos = p.pos;
		vec3 acc = (p.home - p.pos) * 32.0f;

		vec3 dir = p.pos - vec3(Rand::randFloat(getWindowWidth()), 0.0, 0.0);
		float dist = length2(dir);
		p.pos += 100.0f*dir / dist;
	    p.pos.x += (sin(time*M_PI /10.0));
		//p.pos.z += 0.3*(cos(time*M_PI / 1.0));
		p.pos += vel + acc * dt2;
		
	}


	// Copy particle data onto the GPU.
	// Map the GPU memory and write over it.
	void *gpuMem = mParticleVbo->mapReplace();
	memcpy(gpuMem, mParticles.data(), mParticles.size() * sizeof(Particle));
	mParticleVbo->unmap();
}

void Particle_turbulanceApp::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::draw(mTexture);
	gl::ScopedGlslProg		glslScope(mGlsl);
	gl::setMatricesWindowPersp(getWindowSize(), 60.0f, 1.0f, 10000.0f);
	gl::enableDepthRead(false);
	gl::enableDepthWrite(false);

	mParticleBatch->draw();
}

CINDER_APP( Particle_turbulanceApp, RendererGl )
