#include "NaviDemo.h"
#include "NaviUtilities.h"

using namespace Ogre;
using namespace NaviLibrary;
using namespace NaviLibrary::NaviUtilities;

NaviDemo::NaviDemo()
{
	menubar, status, chat, equip = 0;
	shouldQuit = false;
	renderWin = 0;
	sceneMgr = 0;

	Root *root = new Root();

	shouldQuit = !root->showConfigDialog();
	if(shouldQuit) return;

	renderWin = root->initialise(true, "NaviDemo");
	sceneMgr = root->createSceneManager("TerrainSceneManager");
	WindowEventUtilities::addWindowEventListener(renderWin, this);

	createScene();

	setupNavis();

	loadInputSystem();
}

NaviDemo::~NaviDemo()
{
	delete NaviManager::GetPointer();
	Root::getSingleton().shutdown();
}

void NaviDemo::createScene()
{
	sceneMgr->setAmbientLight(ColourValue::White);
	sceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE);

	Camera* camera = sceneMgr->createCamera("MainCam");
	Viewport* viewport = renderWin->addViewport(camera);
	viewport->setBackgroundColour(ColourValue(0.17, 0.14, 0.1));

	camera->setAspectRatio(viewport->getActualWidth() / (Real)viewport->getActualHeight());

	parseResources();
}

void NaviDemo::setupNavis()
{
	// Create the NaviManager make our Navis
	naviMgr = new NaviManager(renderWin, "..\\Media");

	menubar = naviMgr->createNavi("menubar", "local://menubar.html", NaviPosition(BottomCenter), 1024, 128);
	
	// We can do fancy tricks because most of the Navi setter methods return a pointer to the Navi itself
	menubar->
		bind("hpChange", NaviDelegate(this, &NaviDemo::hpChange), Strings("direction"))->
		bind("turnOff", NaviDelegate(this, &NaviDemo::turnOff), Strings("name"))->
		bind("turnOn", NaviDelegate(this, &NaviDemo::turnOn), Strings("name"))->
		setMask("navimenu_bg.png")->
		setMovable(false);

	status = naviMgr->createNavi("status", "local://status.html", NaviPosition(0, 0), 512, 256)->
		bind("levelChanged", NaviDelegate(this, &NaviDemo::levelChanged), Strings("#level"))->
		setMask("status_bg.png")->
		hide();
		
	chat = naviMgr->createNavi("chat", "http://navi.agelessanime.com/chat/index.html", NaviPosition(BottomLeft, 40, -150), 512, 256)->
		bind("messageSent", NaviDelegate(this, &NaviDemo::messageSent), Strings("nick")("message"))->
		setMask("navichat_bg.png")->
		hide();

	equip = naviMgr->createNavi("equip", "local://equip.html", NaviPosition(Right), 256, 512)->
		bind("itemEquipped", NaviDelegate(this, &NaviDemo::itemEquipped), Strings("name"))->
		setMask("naviequip_bg.png")->
		hide();

	// Instantiate the NaviMouse singleton and create the cursors
	NaviMouse* mouse = new NaviMouse(64, 64);
	mouse->createCursor("defaultCursor", 3, 2)->addFrame(0, "cursor.png");
	mouse->setDefaultCursor("defaultCursor");

	// If you define a "move" cursor, NaviMouse will automatically use that when you move a Navi
	NaviCursor* moveCursor = mouse->createCursor("move", 19, 19);
	moveCursor->addFrame(0, "cursorMove.png");
}

void NaviDemo::turnOn(const NaviData &naviData)
{
	std::string naviName = naviData["name"].str();
	Navi* navi = naviMgr->getNavi(naviName);

	if(naviName == "quit")
	{
		shouldQuit = true;
	}
	else if(navi)
	{
		navi->resetPosition();
		navi->show(true);
		navi->focus();
	}
}

void NaviDemo::turnOff(const NaviData &naviData)
{
	Navi* navi = naviMgr->getNavi(naviData["name"].str());

	if(navi)
		navi->hide(true);
}

void NaviDemo::hpChange(const NaviData &naviData)
{
	static short curHP = 100;

	if(naviData["direction"].str() == "up")
		curHP += (rand() % 30) + 1;
	else
		curHP -= (rand() % 30) + 1;

	limit<short>(curHP, 0, 100);

	status->evaluateJS("setHP(?)", Args(curHP));

	logTemplate("Navi Menubar: Current HP is now ?", Args(curHP));
}

void NaviDemo::messageSent(const NaviData &naviData)
{
	// Set the HTML of the element 'playerName' to our chat nickname
	status->evaluateJS("$(?).setHTML(?)", Args("playerName")(naviData["nick"]));

	logTemplate("Navi Chat: ?", Args(naviData["message"]));
}

void NaviDemo::itemEquipped(const NaviData &naviData)
{
	logTemplate("Navi Equip: Item '?' equipped!", Args(naviData["name"]));
}

void NaviDemo::levelChanged(const NaviData &naviData)
{
	int level = naviData["level"].toInt();

	logTemplate("Navi Status: Level has been changed to: ?, double that is: ?", Args(level)(level*2));
}

void NaviDemo::Update()
{
	naviMgr->Update();
	Root::getSingleton().renderOneFrame();
	Ogre::WindowEventUtilities::messagePump();
	InputManager::getSingletonPtr()->capture();
}

void NaviDemo::parseResources()
{
    ConfigFile cf;
	cf.load("resources.cfg");
    ConfigFile::SectionIterator seci = cf.getSectionIterator();

    String secName, typeName, archName;
    while(seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        ConfigFile::SettingsMultiMap *settings = seci.getNext();
        ConfigFile::SettingsMultiMap::iterator i;
        for(i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;
            ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
        }
    }

	ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void NaviDemo::loadInputSystem()
{
	inputMgr = InputManager::getSingletonPtr();
    inputMgr->initialise(renderWin);
    inputMgr->addMouseListener(this, "NaviDemoMouseListener");
	inputMgr->addKeyListener(this, "NaviDemoKeyListener");
}

bool NaviDemo::mouseMoved(const OIS::MouseEvent &arg)
{
	if(arg.state.Z.rel != 0) naviMgr->injectMouseWheel(arg.state.Z.rel);

	return naviMgr->injectMouseMove(arg.state.X.abs, arg.state.Y.abs);
}

bool NaviDemo::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	return naviMgr->injectMouseDown(id);
}

bool NaviDemo::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	return naviMgr->injectMouseUp(id);
}

bool NaviDemo::keyPressed( const OIS::KeyEvent &arg )
{
	if(naviMgr->isAnyNaviFocused())
		return true;

	return true;
}

bool NaviDemo::keyReleased( const OIS::KeyEvent &arg )
{
	switch(arg.key)
	{
	case OIS::KC_ESCAPE:
		shouldQuit = true;
		break;
	case OIS::KC_F1:
	{
		const RenderTarget::FrameStats& stats = renderWin->getStatistics();
		logTemplate("Current FPS: ?", Args((int)stats.lastFPS));
		logTemplate("Avg FPS: ?", Args((int)stats.avgFPS));
		logTemplate("Best FPS: ?", Args((int)stats.bestFPS));
		logTemplate("Worst FPS: ?", Args((int)stats.worstFPS));
		logTemplate("Batch Count: ?", Args((int)stats.batchCount));
		break;
	}
	case OIS::KC_F2:
		sceneMgr->getRootSceneNode()->flipVisibility(true);
		break;
	case OIS::KC_F3:
		naviMgr->resetAllPositions();
		break;
	}

	return true;
}

void NaviDemo::windowMoved(RenderWindow* rw) {}

void NaviDemo::windowResized(RenderWindow* rw) 
{
	inputMgr->setWindowExtents(rw->getWidth(), rw->getHeight());
}

void NaviDemo::windowClosed(RenderWindow* rw) 
{
	shouldQuit = true;
}

void NaviDemo::windowFocusChange(RenderWindow* rw) {}
