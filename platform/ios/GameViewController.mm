//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Game View Controller Implementation
//	Manages the OpenGL ES 3.0 context, game loop, and touch input.
//////////////////////////////////////////////////////////////////////////////////
#import "GameViewController.h"
#import "TouchInput.h"

#include "Gameplay/Stage.h"
#include "IO/UI.h"
#include "IO/Window.h"
#include "Net/Session.h"
#include "Audio/Audio.h"
#include "Character/Char.h"
#include "Gameplay/MapleMap/MapPortals.h"
#include "Graphics/DamageNumber.h"
#include "Timer.h"
#include "Constants.h"
#include "Configuration.h"

#ifdef USE_NX
#include "Util/NxFiles.h"
#else
#include "Util/WzFiles.h"
#endif

@interface GameViewController ()
{
	EAGLContext* _context;
	int64_t _accumulator;
	int64_t _timestep;
}
@end

@implementation GameViewController

- (void)viewDidLoad
{
	[super viewDidLoad];

	_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

	if (!_context)
	{
		NSLog(@"Failed to create OpenGL ES 3.0 context");
		return;
	}

	GLKView* view = (GLKView*)self.view;
	view.context = _context;
	view.drawableDepthFormat = GLKViewDrawableDepthFormatNone;
	view.multipleTouchEnabled = YES;

	[EAGLContext setCurrentContext:_context];

	self.preferredFramesPerSecond = 60;
	_timestep = ms::Constants::TIMESTEP * 1000;
	_accumulator = _timestep;

	// Initialize the game on a slight delay to let the GL context settle
	dispatch_async(dispatch_get_main_queue(), ^{
		[self initializeGame];
	});
}

- (void)initializeGame
{
	[EAGLContext setCurrentContext:_context];

	// Set screen resolution in Configuration
	CGRect screenBounds = [UIScreen mainScreen].nativeBounds;
	ms::Configuration::get().set_max_width(static_cast<int16_t>(screenBounds.size.width));
	ms::Configuration::get().set_max_height(static_cast<int16_t>(screenBounds.size.height));

	// Initialize subsystems in order
	if (ms::Error error = ms::Session::get().init())
	{
		NSLog(@"Session init failed: %s", error.get_message());
		return;
	}

#ifdef USE_NX
	if (ms::Error error = ms::NxFiles::init())
	{
		NSLog(@"NxFiles init failed: %s", error.get_message());
		return;
	}
#else
	if (ms::Error error = ms::WzFiles::init())
	{
		NSLog(@"WzFiles init failed: %s", error.get_message());
		return;
	}
#endif

	if (ms::Error error = ms::Window::get().init())
	{
		NSLog(@"Window init failed: %s", error.get_message());
		return;
	}

	if (ms::Error error = ms::Sound::init())
	{
		NSLog(@"Sound init failed: %s", error.get_message());
		return;
	}

	if (ms::Error error = ms::Music::init())
	{
		NSLog(@"Music init failed: %s", error.get_message());
		return;
	}

	ms::Char::init();
	ms::DamageNumber::init();
	ms::MapPortals::init();
	ms::Stage::get().init();
	ms::UI::get().init();

	ms::Timer::get().start();

	self.gameInitialized = YES;
}

- (void)update
{
	if (!self.gameInitialized || self.gamePaused)
		return;

	int64_t elapsed = ms::Timer::get().stop();

	for (_accumulator += elapsed; _accumulator >= _timestep; _accumulator -= _timestep)
	{
		ms::Window::get().check_events();
		ms::Window::get().update();
		ms::Stage::get().update();
		ms::UI::get().update();
		ms::Session::get().read();
	}

	// Check if we should quit
	if (!ms::Session::get().is_connected() || !ms::UI::get().not_quitted())
	{
		self.gamePaused = YES;
	}
}

- (void)glkView:(GLKView*)view drawInRect:(CGRect)rect
{
	if (!self.gameInitialized || self.gamePaused)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	float alpha = static_cast<float>(_accumulator) / _timestep;

	ms::Window::get().begin();
	ms::Stage::get().draw(alpha);
	ms::UI::get().draw(alpha);
	ms::Window::get().end();
}

#pragma mark - Touch Input

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	if (!self.gameInitialized)
		return;

	[TouchInput handleTouchesBegan:touches inView:self.view];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	if (!self.gameInitialized)
		return;

	[TouchInput handleTouchesMoved:touches inView:self.view];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	if (!self.gameInitialized)
		return;

	[TouchInput handleTouchesEnded:touches inView:self.view];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	if (!self.gameInitialized)
		return;

	[TouchInput handleTouchesEnded:touches inView:self.view];
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (void)dealloc
{
	ms::Sound::close();

	if ([EAGLContext currentContext] == _context)
		[EAGLContext setCurrentContext:nil];
}

@end
