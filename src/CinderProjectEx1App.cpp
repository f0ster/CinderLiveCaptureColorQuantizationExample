#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"

#include "cinder/Capture.h"

#include "CinderOpenCV.h"

#include "cinder/ip/Fill.h"

using namespace ci;
using namespace ci::app;
using namespace std;

static const int WIDTH = 640, HEIGHT = 480;
static const int INITIAL_COLORS_SIZE_LIMIT = 256;

class CinderProjectEx1App : public AppBasic {
  public:
	void setup();
	void keyDown( KeyEvent Event );
	void update();
	void draw();

	void updateColorPalette();
	void drawBlendedMask();
	void updateImage();


  private:
	Capture		mCapture;	
	gl::Texture	mTexture;
	Color8u*	mClusterColors;
	unsigned int mColorCount;

	int	mClusterColorsSize;

	cv::Mat		mColorSamples;
	
};

void CinderProjectEx1App::setup()
{
	// list out the devices, then try to use first one (for dev purposes)
	vector<Capture::DeviceRef> devices( Capture::getDevices() );
	if(!devices.empty()) {
		Capture::DeviceRef device = *devices.begin();
		console() << "Found Device " << device->getName() << " ID: " << device->getUniqueId() << std::endl;
		try {
			if( device->checkAvailable() ) {
				mCapture = Capture( WIDTH, HEIGHT, device ) ;
				mCapture.start();
				mTexture =  gl::Texture();
			}
			else
				console() << "device is NOT available" << std::endl;
		}
		catch( CaptureExc & ) {
			console() << "Unable to initialize device: " << device->getName() << endl;
		}
	}

	mClusterColorsSize = INITIAL_COLORS_SIZE_LIMIT;
	mClusterColors = new Color8u[mClusterColorsSize];

	mColorCount = 4;

	const int sampleCount = mCapture.getHeight() * mCapture.getWidth();
	mColorSamples = cv::Mat( sampleCount, 1, CV_32FC3 );
	
	updateColorPalette();

	gl::enableAlphaBlending();


}

void CinderProjectEx1App::keyDown( KeyEvent event ) {
	bool changed = false;
    if( event.getChar() == '-' ){
        changed = true;
		mColorCount /= 2;
		if(mColorCount <=0 )
			mColorCount = 1;
    } else if( event.getChar() == '=' ){
        changed = true;
		mColorCount *= 2;
		if(mColorCount > mClusterColorsSize) {
			//resize color space
			delete mClusterColors;
			mClusterColors = new Color8u[mClusterColorsSize * 2];
			mClusterColorsSize *= 2;
		}
    }
	
	if(changed) {
		updateColorPalette();
	}
}

void CinderProjectEx1App::update()
{	
	updateImage();

	
}

void CinderProjectEx1App::draw()
{
	gl::enableAlphaBlending();
	gl::clear( Color::black() );
	
	float width = getWindowWidth();	
	float height = getWindowHeight();
	float x = 0, y = 0;

	gl::color( Color::white() );
	//check texture is initialized
	if( mTexture.getHeight() != 0 )
		gl::draw( mTexture, Rectf( x, y, width, height ) );
	
	drawBlendedMask();
}

void CinderProjectEx1App::updateColorPalette() {
	// build our matrix of samples
	Surface::ConstIter imageIt = mCapture.getSurface().getIter();
	cv::MatIterator_<cv::Vec3f> sampleIt = mColorSamples.begin<cv::Vec3f>();
	while( imageIt.line() )
		while( imageIt.pixel() )
			*sampleIt++ = cv::Vec3f( imageIt.r(), imageIt.g(), imageIt.b() );
}

void CinderProjectEx1App::drawBlendedMask() {
	float r = sin( getElapsedSeconds() ) * 0.5f + 0.5f;
	float g = sin( getElapsedSeconds() + 45) * 0.5f + 0.5f;
	float b = sin( getElapsedSeconds() + 90) * 0.5f + 0.5f;

	gl::color( r, g, b, 0.05f );
	Vec3f center = Vec3f( getWindowWidth()/2 , getWindowHeight()/2, 0.0f); 
	gl::drawCube( center, Vec3f(getWindowWidth(), getWindowHeight(), 0.0f));
}

void CinderProjectEx1App::updateImage()
{
	updateColorPalette();
	// call kmeans	
	cv::Mat labels, clusters;
	cv::kmeans( mColorSamples, mColorCount, labels, cv::TermCriteria( cv::TermCriteria::COUNT, 8, 0 ), 1, cv::KMEANS_RANDOM_CENTERS, clusters );

	for( int i = 0; i < mColorCount; ++i )
		mClusterColors[i] = Color8u( clusters.at<cv::Vec3f>(i,0)[0], clusters.at<cv::Vec3f>(i,0)[1], clusters.at<cv::Vec3f>(i,0)[2] );

	Surface result( mCapture.getWidth(), mCapture.getHeight(), false );
	Surface::Iter resultIt = result.getIter();
	cv::MatIterator_<int> labelIt = labels.begin<int>();
	while( resultIt.line() ) {
		while( resultIt.pixel() ) {
			resultIt.r() = mClusterColors[*labelIt].r;
			resultIt.g() = mClusterColors[*labelIt].g;
			resultIt.b() = mClusterColors[*labelIt].b;
			++labelIt;
		}
	}
	
	// draw the color palette across the bottom of the image
	const int swatchSize = 12;
	for( int i = 0; i < mColorCount; ++i ) {
		ip::fill( &result, mClusterColors[i], Area( i * swatchSize, result.getHeight() - swatchSize, ( i + 1 ) * swatchSize, result.getHeight() ) );
	}
	
	mTexture = gl::Texture( result );
}



CINDER_APP_BASIC( CinderProjectEx1App, RendererGl )
