#include "pch.h"
#include "common/Defines.h"
#include "../vdrift/pathmanager.h"
#include "../vdrift/game.h"
#include "../road/Road.h"
#include "OgreGame.h"
#include "common/Gui_Def.h"
#include "common/GraphView.h"
#include "common/Slider.h"
#include "FollowCamera.h"
#include <OIS/OIS.h>
#include "../oisb/OISB.h"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace Ogre;
using namespace MyGUI;


//-----------------------------------------------------------------------------------------------------------
//  Key pressed
//-----------------------------------------------------------------------------------------------------------

// util
bool App::actionIsActive(std::string name, std::string pressed)
{
	std::string actionKey = GetInputName(mOISBsys->lookupAction("General/" + name)->mBindings[0]->mBindables[0].second->getBindableName());
	boost::to_lower(actionKey);
	boost::to_lower(pressed);
	return actionKey == pressed;
}

bool App::keyPressed( const OIS::KeyEvent &arg )
{
	// update all keystates  (needed for all action("..") from oisb)
	if (mOISBsys)
		mOISBsys->process(0.000/*?0*/);
	
	// action key == pressed key
	#define action(s)  actionIsActive(s, mKeyboard->getAsString(arg.key))

	bool tweak = isTweak();

	if (!bAssignKey)
	{
		//  change tweak tabs
		//----------------------------------------------------------------------------------------
		if (mWndTweak->getVisible())
		{
			TabPtr tab = tabTweak;
			if (!shift && tabTweak->getIndexSelected() == 0)
				tab = tabEdCar;  // car edit sections

			if (action("PrevTab")) {  // prev gui subtab
				int num = tab->getItemCount();
				tab->setIndexSelected( (tab->getIndexSelected() - 1 + num) % num );  }
			else if (action("NextTab")) {  // next gui subtab
				int num = tab->getItemCount();
				tab->setIndexSelected( (tab->getIndexSelected() + 1) % num );  }
				
			if (tab == tabEdCar)  // focus ed
			{	pSet->car_ed_tab = tab->getIndexSelected();
				MyGUI::InputManager::getInstance().resetKeyFocusWidget();
				MyGUI::InputManager::getInstance().setKeyFocusWidget(edCar[tab->getIndexSelected()]);  }
		}else
		//  change gui tabs
		if (isFocGui && !pSet->isMain)
		{
			MyGUI::TabPtr tab = 0;  MyGUI::TabControl* sub = 0;
			switch (pSet->inMenu)
			{	case MNU_Replays:  tab = mWndTabsRpl;  break;
				case MNU_Help:     tab = mWndTabsHelp;  break;
				case MNU_Options:  tab = mWndTabsOpts;  sub = vSubTabsOpts[tab->getIndexSelected()];  break;
				default:           tab = mWndTabsGame;  sub = vSubTabsGame[tab->getIndexSelected()];  break;
			}
			if (tab)
			if (shift)
			{	if (action("PrevTab")) {  // prev gui subtab
					if (sub)  {  int num = sub->getItemCount();
						sub->setIndexSelected( (sub->getIndexSelected() - 1 + num) % num );  }	}
				else if (action("NextTab")) {  // next gui subtab
					if (sub)  {  int num = sub->getItemCount();
						sub->setIndexSelected( (sub->getIndexSelected() + 1) % num );  }  }
			}else
			{	int num = tab->getItemCount()-1, i = 0, n = 0;
				if (action("PrevTab"))
				{	i = tab->getIndexSelected();
					do{  if (i==1)  i = num;  else  --i;  ++n;  }
					while (n < num && tab->getButtonWidthAt(i) == 1);
					tab->setIndexSelected(i);  MenuTabChg(tab,i);  return true;
				} else
				if (action("NextTab"))
				{	i = tab->getIndexSelected();
					do{  if (i==num)  i = 1;  else  ++i;  ++n;  }
					while (n < num && tab->getButtonWidthAt(i) == 1);
					tab->setIndexSelected(i);  MenuTabChg(tab,i);  return true;
				}
			}
		}
		else if (!isFocGui && pSet->show_graphs)  // change graphs type
		{
			int& v = (int&)pSet->graphs_type;  int vo = v;
			if (action("PrevTab"))  v = (v-1 + Gh_ALL) % Gh_ALL;
			if (action("NextTab"))	v = (v+1) % Gh_ALL;
			if (vo != v)
			{
				cmbGraphs->setIndexSelected(v);
				comboGraphs(cmbGraphs, v);
				if (v == 4)  iUpdTireGr = 1;  //upd now
			}
		}
		//----------------------------------------------------------------------------------------


		//  gui on/off  or close wnds
		if (action("ShowOptions") && !alt)
		{
			if (mWndNetEnd && mWndNetEnd->getVisible())  {  mWndNetEnd->setVisible(false);  // hide netw end
				return false;	}
			else
			{
				if (mWndChampEnd && mWndChampEnd->getVisible())  mWndChampEnd->setVisible(false);  // hide champs end
				toggleGui(true);  return false;
			}
		}

		//  new game - reload   not in multiplayer
		if (action("RestartGame") && !mClient)
		{
			bPerfTest = ctrl;  // ctrl-F5 start perf test
			if (bPerfTest)
			{	pSet->gui.track = "Test10-FlatPerf";
				pSet->gui.track_user = false;  }
			iPerfTestStage = PT_StartWait;
			NewGame();  return false;
		}

		//  new game - fast (same track & cars)
		if (action("ResetGame") && !mClient)
		{	
			for (int c=0; c < carModels.size(); ++c)
			{
				CarModel* cm = carModels[c];
				if (cm->pCar)  cm->pCar->bResetPos = true;
				cm->First();
				cm->ResetChecks();
				cm->iWonPlace = 0;  cm->iWonPlaceOld = 0;
				cm->iWonMsgTime = 0.f;
			}
			pGame->timer.Reset(-1);
			pGame->timer.pretime = mClient ? 2.0f : pSet->game.pre_time;  // same for all multi players
			carIdWin = 1;  //
			ghost.Clear(); //
		}


		//  Screen shot
		//if (action("Screenshot"))  //isnt working
		const OISB::Action* act = OISB::System::getSingleton().lookupAction("General/Screenshot",false);
		if (act && act->isActive())
		{	mWindow->writeContentsToTimestampedFile(PATHMANAGER::Screenshots() + "/", ".jpg");
			return false;	}
		
		using namespace OIS;


		//  main menu keys
		if (pSet->isMain && isFocGui)
		{
			switch (arg.key)
			{
			case KC_UP:  case KC_NUMPAD8:
				pSet->inMenu = (pSet->inMenu-1 + ciMainBtns) % ciMainBtns;
				toggleGui(false);  return true;

			case KC_DOWN:  case KC_NUMPAD2:
				pSet->inMenu = (pSet->inMenu+1) % ciMainBtns;
				toggleGui(false);  return true;

			case KC_RETURN:
				pSet->isMain = false;
				toggleGui(false);  return true;
			}
		}

		//  esc
		if (arg.key == KC_ESCAPE)
		{
			if (pSet->escquit)
				mShutDown = true;	// quit
			else
				if (mWndChampStage->getVisible())  ///  close champ wnds
					btnChampStageStart(0);
				else
					toggleGui(true);	// gui on/off
			return true;
		}
		
		
		//  shortcut keys for gui access (alt-Q,C,S,G,V,.. )
		if (alt)
			switch (arg.key)
			{
				case KC_Z:  // alt-Z Tweak (alt-shift-Z save&reload)
					TweakToggle();	return true;
					
				case KC_Q:	GuiShortcut(MNU_Single, TAB_Track);	return true;  // Q Track
				case KC_C:	GuiShortcut(MNU_Single, TAB_Car);	return true;  // C Car

				case KC_T:	GuiShortcut(MNU_Single, TAB_Setup);	return true;  // T Car Setup
				case KC_W:	GuiShortcut(MNU_Single, TAB_Game);	return true;  // W Game Setup

				case KC_J:	GuiShortcut(MNU_Tutorial, TAB_Champs);	return true;  // J Tutorials
				case KC_H:	GuiShortcut(MNU_Champ,    TAB_Champs);	return true;  // H Champs
				case KC_L:	GuiShortcut(MNU_Challenge,TAB_Champs);	return true;  // L Challenges

				case KC_U:	GuiShortcut(MNU_Single, TAB_Multi);	return true;	// U Multiplayer
				case KC_R:	GuiShortcut(MNU_Replays, 1);	return true;		// R Replays

				case KC_S:	GuiShortcut(MNU_Options, 1);	return true;  // S Screen
				 case KC_E:	GuiShortcut(MNU_Options, 1,1);	return true;  // E -Effects
				case KC_G:	GuiShortcut(MNU_Options, 2);	return true;  // G Graphics
				 case KC_N:	GuiShortcut(MNU_Options, 2,2);	return true;  // N -Vegetation

				case KC_V:	GuiShortcut(MNU_Options, 3);	return true;  // V View
				 case KC_M:	GuiShortcut(MNU_Options, 3,1);	return true;  // M -Minimap
				 case KC_O:	GuiShortcut(MNU_Options, 3,3);	return true;  // O -Other
				case KC_I:	GuiShortcut(MNU_Options, 4);	return true;  // I Input
				case KC_P:	GuiShortcut(MNU_Options, 5);	return true;  // P Sound
			}

		
		//>--  dev shortcuts, alt-shift numbers, start test track
		if (pSet->dev_keys && alt && shift && !mClient)
		{
			string t;
			switch (arg.key)
			{
				case KC_1: t = "Test1-Flat";  break;
				case KC_2: t = "Test11-Jumps";  break;
				case KC_3: t = "TestC4-ow";  break;
				case KC_4: t = "Test7-FluidsSmall";  break;
				case KC_5: t = "TestC6-temp";  break;
				case KC_6: t = "Test10-FlatPerf";  break;
			}
			if (!t.empty())
			{
				pSet->gui.champ_num = -1;
				pSet->gui.track = t;  bPerfTest = false;
				pSet->gui.track_user = false;
				NewGame();  return true;
			}
		}
			

		///* tire edit */
		if (pSet->graphs_type == Gh_TireEdit && !tweak)
		{
			int& iCL = iEdTire==1 ? iCurLong : (iEdTire==0 ? iCurLat : iCurAlign);
			int iCnt = iEdTire==1 ? 11 : (iEdTire==0 ? 15 : 18);
			switch (arg.key)
			{
				case KC_HOME: case KC_NUMPAD7:  // mode long/lat
				if (ctrl)
					iTireLoad = 1-iTireLoad;
				else
					iEdTire = iEdTire==1 ? 0 : 1;  iUpdTireGr=1;  return true;

				case KC_END: case KC_NUMPAD1:	// mode align
					iEdTire = iEdTire==2 ? 0 : 2;  iUpdTireGr=1;  return true;

				/*case KC_1:*/ case KC_PGUP: case KC_NUMPAD9:    // prev val
					iCL = (iCL-1 +iCnt)%iCnt;  iUpdTireGr=1;  return true;

				/*case KC_2:*/ case KC_PGDOWN: case KC_NUMPAD3:  // next val
					iCL = (iCL+1)%iCnt;  iUpdTireGr=1;  return true;
			}
		}
		
		
		//  not main menus
		//--------------------------------------------------------------------------------------------------------------
		//if (/*&& !pSet->isMain*/)
		if (!tweak)
		{
			Widget* wf = MyGUI::InputManager::getInstance().getKeyFocusWidget();
			bool edFoc = wf && wf->getTypeName() == "EditBox";
			//if (wf)  LogO(wf->getTypeName()+" " +toStr(edFoc));
			switch (arg.key)
			{
				case KC_BACK:
					if (mWndChampStage->getVisible())	// back from champs stage wnd
					{	btnChampStageBack(0);  return true;  }

					if (pSet->isMain)  break;
					if (isFocGui)
					{	if (edFoc)  break;
						pSet->isMain = true;  toggleGui(false);  }
					else
						if (mWndRpl && !isFocGui)	bRplWnd = !bRplWnd;  // replay controls
					return true;

				case KC_P:		// replay play/pause
					if (bRplPlay && !isFocGui)
					{	bRplPause = !bRplPause;  UpdRplPlayBtn();
						return true;  }
					break;
					
				case KC_K:		// replay car ofs
					if (bRplPlay && !isFocGui)	{	--iRplCarOfs;  return true;  }
					break;
				case KC_L:		// replay car ofs
					if (bRplPlay && !isFocGui)	{	++iRplCarOfs;  return true;  }
					break;
					
				case KC_F:		// focus on find edit
					if (ctrl && edFind && (pSet->dev_keys || isFocGui &&
						!pSet->isMain && pSet->inMenu == MNU_Single && mWndTabsGame->getIndexSelected() == TAB_Track))
					{
						if (pSet->dev_keys)
							GuiShortcut(MNU_Single, 1);	// Track tab
						MyGUI::InputManager::getInstance().resetKeyFocusWidget();
						MyGUI::InputManager::getInstance().setKeyFocusWidget(edFind);
						return true;
					}	break;
					

				case KC_F7:		// Times
					if (shift)
					{	WP wp = chOpponents;  ChkEv(show_opponents);  ShowHUD();  }
					else if (!ctrl)
					{	WP wp = chTimes;  ChkEv(show_times);  ShowHUD();  }
					return false;
					
				case KC_F8:		// car debug bars
					if (ctrl)
					{	WP wp = chDbgB;  ChkEv(car_dbgbars);   ShowHUD();  }
					else		// Minimap
					if (!shift)
					{	WP wp = chMinimp;  ChkEv(trackmap);
						for (int c=0; c < hud.size(); ++c)
							if (hud[c].ndMap)  hud[c].ndMap->setVisible(pSet->trackmap);
					}	return false;

				case KC_F9:
					if (ctrl)	// car debug surfaces
					{	WP wp = chDbgS;  ChkEv(car_dbgsurf);  ShowHUD();  }
					else
					if (shift)	// car debug text
					{	WP wp = chDbgT;  ChkEv(car_dbgtxt);  ShowHUD();  }
					else		// graphs
					{	WP wp = chGraphs;  ChkEv(show_graphs);
						for (int i=0; i < graphs.size(); ++i)
							graphs[i]->SetVisible(pSet->show_graphs);
					}
					return true;

				case KC_F11:
					if (shift)	// profiler times
					{	WP wp = chProfTxt;  ChkEv(profilerTxt);  ShowHUD();  }
					else
					if (!ctrl)  // Fps
					{	WP wp = chFps;  ChkEv(show_fps); 
						if (pSet->show_fps)  mFpsOverlay->show();  else  mFpsOverlay->hide();
						return false;
					}	break;

				case KC_F10:	//  blt debug, txt
					if (shift)
					{	WP wp = chBltTxt;  ChkEv(bltProfilerTxt);  return false;  }
					else if (ctrl)
					{	WP wp = chBlt;  ChkEv(bltDebug);  return false;  }
					else		// wireframe
						toggleWireframe();
					return false;

				
				case KC_RETURN:		///  close champ wnds
					if (mWndChampStage->getVisible())
						btnChampStageStart(0);
					else			//  chng trk/car + new game  after up/dn
					if (isFocGui && !pSet->isMain)
						switch (pSet->inMenu)
						{
						case MNU_Replays:	btnRplLoad(0);  break;
						default:
						{	switch (mWndTabsGame->getIndexSelected())
							{
							case TAB_Track:	 changeTrack();	btnNewGame(0);  break;
							case TAB_Car:	 changeCar();	btnNewGame(0);  break;
							case TAB_Multi:	 chatSendMsg();  break;
							case TAB_Champs: btnChampStart(0);  break;
						}	break;
					}	}
					else
					if (mClient && !isFocGui)  // show/hide players net wnd
					{	mWndNetEnd->setVisible(!mWndNetEnd->getVisible());  return true;  }
					break;
			}
		}
	}
	InputBind(arg.key);
	

	//  gui input
	if (mGUI && (isFocGui || tweak))
	{
		MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(arg.key), arg.text);
		return false;
	}

	return true;
}
