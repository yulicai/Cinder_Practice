#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Serial.h"
#include "cinder/Text.h"
#include "cinder/Log.h"

#include <sstream>

using namespace ci;
using namespace ci::app;
using namespace std;

#define BUFSIZE 80
#define READ_INTERVAL 0.25

class Arduino_Communication101App : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	bool serialInitiallized();


	SerialRef mSerial;
	uint8_t mCounter;
	string mLastString;
	char contact;
	
	gl::TextureRef mTexture;

	double mLastRead, mLastUpdate;


};

void Arduino_Communication101App::setup()
{
	mCounter = 0;
	mLastRead = 0;
	mLastUpdate = 0;


	// print the devices
	for (const auto &dev : Serial::getDevices())
		console() << "Device: " << dev.getName() << endl;

	try {
		//Serial::Device dev = Serial::findDeviceByNameContains("tty.usbmodem");
		Serial::Device dev = Serial::findDeviceByNameContains("COM");
		mSerial = Serial::create(dev, 9600);
	}
	catch (SerialExc &exc) {
		CI_LOG_EXCEPTION("coult not initialize the serial device", exc);
		exit(-1);
	}


	contact = 0;
	mSerial->flush();
}

void Arduino_Communication101App::mouseDown( MouseEvent event )
{
}


bool Arduino_Communication101App::serialInitiallized() {
	//SERIAL HANDSHAKE WITH ARDUINO

	if (contact != '*') {
		contact = (char)mSerial->readByte();
		return false;
	}
	else {
		mSerial->writeByte('0');
		return true;
	}

}

void Arduino_Communication101App::update()
{
	if (serialInitiallized()) {
		if (mSerial->getNumBytesAvailable() > 0) {
			console() << "Bytes available: " << mSerial->getNumBytesAvailable() << std::endl;
			try {
				mLastString = mSerial->readStringUntil('\n', BUFSIZE);
			}
			catch (SerialTimeoutExc &exc) {
				CI_LOG_EXCEPTION("timeout", exc);
			}
			console() << "last string: " << mLastString << endl;

		}
	}


		TextLayout simple;
		simple.setFont(Font("Arial Black", 24));
		simple.setColor(Color(.7, .7, .2));
		simple.addLine(mLastString);
		simple.setLeadingOffset(0);
		mTexture = gl::Texture::create(simple.render(true, false));

		mSerial->flush();
	}


void Arduino_Communication101App::draw()
{
	gl::clear();

	if (mTexture)
		gl::draw(mTexture, vec2(10, 10));
}

CINDER_APP( Arduino_Communication101App, RendererGl )
